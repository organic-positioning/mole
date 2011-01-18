/*
 * Mole - Mobile Organic Localisation Engine
 * Copyright 2010 Nokia Corporation.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "localizer.h"

QString current_estimated_space_name;
QString unknown_space_name = "??";

#define area_fill_period_short   100
#define area_fill_period       60000
#define map_fill_period_short   1000
#define map_fill_period        60000

#define ALPHA                   0.20

//bool offline_mode = true;

/*
void start_timer (QTimer *timer, int period) {
  timer->start (rand_poisson(period));
}
*/

Localizer::Localizer(QObject *parent, Binder *_binder) :
  QObject(parent), binder (_binder) {

  online = true;
  first_add_scan = true;

  area_map_reply_in_flight = false;
  mac_reply_in_flight = false;

  //area_map_reply = NULL;

  // contains the queue of currently active scans
  // i.e. scans we are using to localize
  scan_queue = new QQueue<AP_Scan*> ();

  // mapping from macs in scan_queue to stuff about them
  // e.g. their max rssi, count
  active_macs = new QMap<QString,MacDesc*> ();
  recent_active_macs = new QMap<QString,MacDesc*> ();

  overlap = new Overlap ();

  stats = new LocalizerStats (this);
  network_success_level = 10;

  // map dir created/checked in init_mole_app
  QString map_dir_name = app_root_dir->absolutePath();
  map_dir_name.append ("/map");
  map_root = new QDir (map_dir_name);



  current_estimated_space_name = unknown_space_name;

  /*
    cache_network_manager = new QNetworkAccessManager (this);
    connect(cache_network_manager, SIGNAL(finished(QNetworkReply*)),
    SLOT(mac_to_areas_response(QNetworkReply*)));

    area_map_network_manager = new QNetworkAccessManager (this);
    connect(area_map_network_manager, SIGNAL(finished(QNetworkReply*)),
    SLOT(area_map_response(QNetworkReply*)));
  */

  area_cache_fill_timer = new QTimer(this);
  connect(area_cache_fill_timer, SIGNAL(timeout()), 
	  this, SLOT(fill_area_cache()));

  map_cache_fill_timer = new QTimer(this);
  connect(map_cache_fill_timer, SIGNAL(timeout()), 
	  this, SLOT(fill_map_cache()));
  // Do not start these timer until we have a scan


  /*
  localize_timer = new QTimer(this);
  connect(localize_timer, SIGNAL(timeout()), 
	  this, SLOT(localize()));
  localize_timer->start(15000);
  */
  connect (this, SIGNAL (location_data_changed()),
	   SLOT (localize()));


  last_localized_time = QTime::currentTime();
  //dirty = false;
  clear_queue = false;
  signal_maps = new QMap<QString,AreaDesc*>;  

  load_maps();

  QDBusConnection::sessionBus().registerObject("/", this);

  // service, path, interface, name, signature, receiver, slot
  //QDBusConnection::sessionBus().connect("com.nokia.moled", "/com/nokia/moled", "com.nokia.moled", "GetSignature", this, SLOT(handle_signature_request()));

  QDBusConnection::sessionBus().connect(QString(), QString(), "com.nokia.moled", "GetLocationEstimate", this, SLOT(handle_location_estimate_request()));

  QDBusConnection::sessionBus().connect(QString(), QString(), "com.nokia.moled", "SpeedEstimate", this, SLOT(handle_speed_estimate(int)));

}

Localizer::~Localizer () {
  qDebug () << "deleting localizer";

  delete area_cache_fill_timer;
  delete map_cache_fill_timer;
  //delete localize_timer;

  qDebug () << "deleting scan_queue";
  
  // scan_queue and ap_scans in it
  while (!scan_queue->isEmpty()) {
    AP_Scan *scan = scan_queue->dequeue ();
    delete scan;
  }
  delete scan_queue;

  qDebug () << "deleting active macs";

  // active_macs and mac_desc
  /*
    QMapIterator<QString,MacDesc*> i (*active_macs);
    while (i.hasNext()) {
    i.next();
    MacDesc *mac_desc = i.value();
    delete mac_desc;
    }
  */
  clear_active_macs (active_macs);
  delete active_macs;

  clear_active_macs (recent_active_macs);
  delete recent_active_macs;

  qDebug () << "deleting scan_queue";
  // signal_maps
  QMapIterator<QString,AreaDesc*> j (*signal_maps);
  while (j.hasNext()) {
    j.next();
    AreaDesc *area_desc = j.value();
    delete area_desc;
  }
  delete signal_maps;

  delete overlap;
  delete stats;
  delete map_root;
  //delete cache_network_manager;
  //delete area_map_network_manager;

  qDebug () << "finished deleting localizer";

}

// Given a new scan from the scanner, record it
// and clean out any old scans
void Localizer::add_scan (AP_Scan *raw_scan) {
  qDebug() << "localizer adding new scan, queue size " << scan_queue->size();

  stats->received_scan ();

  // BEGIN TESTING
  //   QMap<QString,MacDesc*> *active_macs_tmp_a = new QMap<QString,MacDesc*> ();
  //   raw_scan->add_active_macs (active_macs_tmp_a);

  //   QMap<QString,MacDesc*> *active_macs_tmp_b = new QMap<QString,MacDesc*> ();
  //   raw_scan->add_active_macs (active_macs_tmp_b);

  //   double score_now = overlap->compare_sig_overlap ((QMap<QString,Sig*>*)active_macs_tmp_a, (QMap<QString,Sig*>*)active_macs_tmp_b);

  //   qDebug() << "score from same single scan " << score_now;

  //   double score_all = overlap->compare_sig_overlap ((QMap<QString,Sig*>*)active_macs_tmp_a, (QMap<QString,Sig*>*)active_macs);

  //   qDebug() << "score from vs all scans " << score_all;

  //   delete active_macs_tmp_a;
  //   delete active_macs_tmp_b;

  // END TESTING

  // now that we have a scan, start the first network timer
  if (first_add_scan) {
    area_cache_fill_timer->stop ();
    area_cache_fill_timer->start (area_fill_period_short);
    first_add_scan = false;
  }
  
  // 2 minutes
  const int EXPIRE_SECS = -60*2;

  QDateTime expire_stamp = QDateTime
    (QDateTime::currentDateTime().addSecs(EXPIRE_SECS).toUTC());

  // toss any expired scans
  // or toss everything if we have been told to clear the queue
  while (!scan_queue->isEmpty() &&
	 (clear_queue || 
	  *(scan_queue->head()->stamp) < expire_stamp)) {

    AP_Scan *scan = scan_queue->dequeue ();
    qDebug() << "tossing expired scan " << *scan->stamp
	     << " remaining="<< scan_queue->size();
    scan->remove_inactive_macs (active_macs);

    delete scan;
      
  }
  clear_queue = false;


  // make our own local copy of the scan
  AP_Scan *new_scan = new AP_Scan (raw_scan);

  stats->add_ap_per_scan_count (new_scan->size());
  scan_queue->enqueue (new_scan);
  new_scan->add_active_macs (active_macs);
  clear_active_macs (recent_active_macs);
  new_scan->add_active_macs (recent_active_macs);
  emit_new_local_signature ();

  emit location_data_changed ();

  /*
  // TODO for now, just blow away the old lists
  QMapIterator<QString,QList<int>* > k (*mac2rssi_list);
  while (k.hasNext()) {
  k.next();
  QList<int> *old_rssi_list = k.value();
  delete old_rssi_list;
  }


  QMap<QString,QList<int>* > mac2rssi_list;

  QListIterator<AP_Scan*> i (*scan_queue);
  while (i.hasNext()) {
  AP_Scan *ap_scan = i.next();
  QListIterator<AP_Reading*> j (ap_scan->readings);

  while (j.hasNext()) {
  AP_Reading *ap_reading = j.next();

  if (!mac2rssi_list.contains (ap_reading->bssid)) {
  QList<int>* new_rssi_list = new QList<int> ();
  mac2rssi_list.insert (ap_reading->bssid, new_rssi_list);
  }
  QList<int>* rssi_list = mac2rssi_list.value (ap_reading->bssid);
  rssi_list.append (ap_reading->level);
  }
  }
  */


  //dirty = true;

}

void Localizer::handle_speed_estimate(int moving_count) {
  qDebug () << "localizer got speed estimate" << moving_count;

  // TODO some kind of moving rate
  stats->movement_detected ();

  if (moving_count > 0) {
    clear_queue = true;
    qDebug () << "set clear queue";
  }

}


void Localizer::emit_new_local_signature () {

  QDBusMessage msg = QDBusMessage::createSignal("/", "com.nokia.moled", "SignatureChanged");

  // serialize the current signature (OK, not very flexible, I admit)

  QStringList sigs;

  sigs << QString::number (active_macs->size());

  QMapIterator <QString,MacDesc*> i (*active_macs);
  while (i.hasNext()) {
    i.next();
    sigs << i.key();
    sigs << QString::number (i.value()->mean);
    sigs << QString::number (i.value()->stddev);
    sigs << QString::number (i.value()->weight);
  }

  msg << sigs;

  stats->add_ap_per_sig_count (active_macs->size());

  if (verbose) {
    qDebug () << "current sig " << sigs;
  }

  QDBusConnection::sessionBus().send(msg);

}

QString Localizer::handle_signature_request() {
  return "sig request";
}

void Localizer::handle_location_estimate_request() {
  qDebug() << "location estimate request";
  emit_location_estimate ();
  stats->emit_statistics ();
}

void Localizer::localize () {

  /*
  if (!dirty) {
    qDebug() << "localize has no new data, so doing nothing";
    return;
  }
  */

  // do not localize faster than once per 10 seconds
  const int min_localize_interval_sec = 10;
  if (last_localized_time.elapsed() < 1000 * min_localize_interval_sec) {
    qDebug () << "not localizing yet, not enough time elapsed";
    return;
  }
  last_localized_time = QTime::currentTime();
  //dirty = false;

  // add_scan has updated active_macs
  if (active_macs->size() == 0) {
    qDebug() << "no known macs to localize on";
    return;
  }

  // first come up with a short list of potential areas
  // based on mac overlap

  const double min_area_mac_overlap_coefficient = 0.01;
  const double min_space_mac_overlap_coefficient = 0.01;



  QList<QString> potential_areas;
  QMapIterator<QString,AreaDesc*> i (*signal_maps);
  double max_c = 0;
  QString max_c_area;



  int valid_areas = 0;
  while (i.hasNext()) {
    i.next();

    if (i.value() != NULL) {

      valid_areas++;

      double c = mac_overlap_coefficient (active_macs, i.value()->get_macs());
      if (c > max_c) {
	max_c = c;
	max_c_area = i.key();
      }

      //qDebug() << "area " << i.key() << " c=" << c;
      if (c > min_area_mac_overlap_coefficient) {
	potential_areas.push_back (i.key());
	i.value()->accessed();
      }
    }
  }

  stats->set_scan_queue_size (scan_queue->size());
  stats->set_macs_seen_size (active_macs->size());
  stats->set_total_area_count (valid_areas);

  if (verbose) {
    qDebug() << "areas top "<< max_c_area << " c=" << max_c
	     << " count=" << potential_areas.size()
	     << " valid=" << valid_areas 
	     << " nomap=" << (signal_maps->size() - valid_areas);
  }

  if (potential_areas.size() == 0) {
    qDebug() << "eliminated all areas";
    return;
  }

  stats->set_potential_area_count (potential_areas.size());

  // next narrow down to a subset of spaces
  // from these areas

  QMap<QString,SpaceDesc*> potential_spaces;
  QListIterator<QString> j (potential_areas);
  max_c = 0.;
  QString max_c_space;
  int total_space_count = 0;

  while (j.hasNext()) {
    QString area_name = j.next();

    AreaDesc *area = signal_maps->value(area_name);
    QMap<QString,SpaceDesc*> *spaces = area->get_spaces ();
    QMapIterator<QString,SpaceDesc*> k (*spaces);

    while (k.hasNext()) {

      k.next();
      
      if (k.value() != NULL) {

	double c = mac_overlap_coefficient (active_macs, k.value()->get_macs());
	if (c > max_c) {
	  max_c = c;
	  max_c_space = k.key();
	  max_c_area = area_name;
	}

	total_space_count++;

	if (verbose) {
	  qDebug() << "potential space " << k.key() << " c=" << c
		   << " ok=" << (c > min_space_mac_overlap_coefficient);
	}
	if (c > min_space_mac_overlap_coefficient) {
	  potential_spaces.insert (k.key(), k.value());
	}

      }

    }

  }

  qDebug() << "spaces count=" << potential_spaces.size()
	   << " pct=" << potential_spaces.size()/(double)total_space_count;

  // for now, just pick the space with the top c
  //QString t_estimated_space_name = max_c_space;
  //estimated_space_name = max_c_space;
  //estimated_space_score = max_c;

  // TODO blah just send it everytime
  //if (t_estimated_space_name != estimated_space_name ||
  //      (t_estimated_space_name == estimated_space_name && 
  //   )) {
  //estimated_space_name = t_estimated_space_name;

  //}

  stats->set_total_space_count (total_space_count);
  stats->set_potential_space_count (potential_spaces.size());

  make_overlap_estimate (potential_spaces);

}


void Localizer::make_overlap_estimate 
(QMap<QString,SpaceDesc*> &potential_spaces) {

  QString max_space;
  double max_score = -5.;
  const double init_overlap_diff = 10.;
  double overlap_diff = init_overlap_diff;
  QMapIterator<QString,SpaceDesc*> i (potential_spaces);
  while (i.hasNext()) {
    i.next();
    double score = overlap->compare_sig_overlap
      ((QMap<QString,Sig*>*)active_macs, i.value()->get_sigs());


    //double recent_score = overlap->compare_sig_overlap
    //((QMap<QString,Sig*>*)recent_active_macs, i.value()->get_sigs());


    //if (verbose) {
    qDebug () << "overlap compute: space="<< i.key() << " score="<< score;
    //		<< " recent=" << recent_score;

    //}

    double diff = max_score - score;
    //qDebug () << "score " << score 
    //<< "diff " << diff;

    // Hmm.  This is not correct and not computable the way we are doing it.
    // It is also not that interesting a value.
    // Maybe s
    // difference between top and second-from-top
    if (diff < overlap_diff) {
      overlap_diff = diff;
    }

    if (score > max_score) {
      max_score = score;
      max_space = i.key();
    }

  }

  if (overlap_diff < init_overlap_diff) {
    stats->add_overlap_diff (overlap_diff);
    stats->add_overlap_max (max_score);
  }

  qDebug () << "overlap max: space="<< max_space << " score="<< max_score
	    << " scans_used=" << scan_queue->size();

  if (!max_space.isEmpty()) {
    if (binder != NULL) {
      binder->set_location_estimate (max_space, max_score);
    }
    emit_new_location_estimate (max_space, max_score);
  }

  stats->emit_statistics ();
}



// TODO add coverage estimate, as int? suggested space?
void Localizer::emit_new_location_estimate 
(QString estimated_space_name, double estimated_space_score) {


  qDebug () << "emit_new_location_estimate " << estimated_space_name
	    << " score=" << estimated_space_score;

  current_estimated_space_score = estimated_space_score;
  if (current_estimated_space_name != estimated_space_name) {
    current_estimated_space_name = estimated_space_name;
    stats->emitted_new_location ();
    emit_location_estimate ();
  }
}

void Localizer::clear_active_macs (QMap<QString,MacDesc*> *macs) {
  QMapIterator<QString,MacDesc*> i (*macs);
  while (i.hasNext()) {
    i.next();
    MacDesc *mac_desc = i.value();
    delete mac_desc;
  }
  macs->clear();
}

void Localizer::emit_location_estimate () {
  QDBusMessage msg = QDBusMessage::createSignal("/", "com.nokia.moled", "LocationEstimate");
  msg << current_estimated_space_name;
  msg << online;

  QDBusConnection::sessionBus().send(msg);
}

// periodically put a line in the log file
void LocalizerStats::log_statistics () {
  qWarning () << "stats: "
	      << "net_ok " << network_success_rate
	      << "churn " << emit_new_location_sec
	      << "scan_ms " << scan_rate_ms
	      << "scans " << scan_queue_size
	      << "macs " << macs_seen_size;
}

void LocalizerStats::emit_statistics () {

  int uptime = start_time.secsTo (QDateTime::currentDateTime());
  if (emit_new_location_count > 0) {
    emit_new_location_sec = (double) uptime / (double) emit_new_location_count;
  } else {
    emit_new_location_sec = 0;
  }

  qDebug () << "emit_statistics "
	    << "network_success_rate " << network_success_rate
	    << "emit_new_location_sec " << emit_new_location_sec
	    << "scan_rate_ms " << scan_rate_ms;


  QDBusMessage stats_msg = QDBusMessage::createSignal("/", "com.nokia.moled", "LocationStats");
  stats_msg << current_estimated_space_name

    // convert to GMT for network recording
	    << start_time

    // integers
	    << scan_queue_size
	    << macs_seen_size

	    << total_area_count
	    << total_space_count
	    << potential_area_count
	    << potential_space_count

	    << movement_detected_count


    // doubles
	    << scan_rate_ms
	    << emit_new_location_sec

	    << network_latency
	    << network_success_rate

	    << overlap_max
	    << overlap_diff;


  QDBusConnection::sessionBus().send(stats_msg);
}

void Localizer::fill_area_cache() {
  area_cache_fill_timer->stop ();
  area_cache_fill_timer->start(area_fill_period);

  //if (offline_mode) { return; }  

  QString loud_mac = get_loud_mac ();

  if (!loud_mac.isEmpty()) {

    QNetworkRequest request;
    QString url_str = setting_server_url_value;
    QUrl url(url_str.append("/getAreas"));

    url.addQueryItem ("mac", loud_mac);
    request.setUrl(url);

    qDebug () << "req " << url;

    set_network_request_headers (request);

    //cache_network_manager->get (request);

    if (!mac_reply_in_flight) {
      mac_reply = network_access_manager->get (request);
      qDebug () << "mac_reply creation " << mac_reply;  
      connect (mac_reply, SIGNAL(finished()), 
	       SLOT (mac_to_areas_response()));
      mac_reply_in_flight = true;
    } else {
      qDebug () << "mac_reply_in_flight: dropping request" << url;
    }
  }

}

//void Localizer::mac_to_areas_response (QNetworkReply* mac_reply) {
void Localizer::mac_to_areas_response () {

  qDebug () << "mac_to_areas_response";
  qDebug () << "reply" << mac_reply;

  mac_reply_in_flight = false;
  mac_reply->deleteLater();

  int elapsed = find_reply_latency_msec (mac_reply);
  stats->add_network_latency (elapsed);

  if (mac_reply->error() != QNetworkReply::NoError) {
    qWarning() << "mac_to_areas_response request failed "
	       << mac_reply->errorString()
	       << " url " << mac_reply->url();
    //mac_reply->deleteLater();
    stats->add_network_success_rate (0);
    online = false;
    //check_network_state ();
    return;
  }
  online = true;
  //qDebug () << "resp B";
  stats->add_network_success_rate (1);
  //check_network_state ();
  qDebug() << "mac_to_areas_response request succeeded";

  // process response one line at a time
  // each line is the path to an area
  const int buffer_size = 256;
  char buffer [buffer_size];
  memset (buffer, 0, buffer_size);
  bool new_area_found = false;

  int length = mac_reply->readLine(buffer, buffer_size);
  //QByteArray data = mac_reply->readLine(buffer_size);
  //while  (!data.isEmpty()) {
  while  (length != -1) {
    //char buffer2 [buffer_size];
    //memset (buffer2, 0, buffer_size);
    //memcpy (buffer2, buffer, strlen (buffer));
    QString area_name (buffer);
    //QString area_name (data);
    //QString area_name = new QString (buffer);
    //QString area_name = "USA/Massachusetts/Cambridge/4 Cambridge Center";
    area_name = area_name.trimmed();
    

    if (area_name.length() > 1 && !signal_maps->contains(area_name)) {
      // create empty slot in signal map for this space
      signal_maps->insert(area_name, NULL);
      new_area_found = true;
      qDebug() << "getArea found new area_name=" << area_name;
    } else {
      if (verbose) {
	qDebug() << "getArea found existing area_name=" << area_name;
      }
    }
    memset (buffer, 0, buffer_size);
    //memset (buffer2, 0, buffer_size);
    //data = mac_reply->readLine(buffer_size);
    length = mac_reply->readLine(buffer, buffer_size);
  }


  if (new_area_found) {
    map_cache_fill_timer->stop ();
    map_cache_fill_timer->start (map_fill_period_short);
  }

}

void Localizer::fill_map_cache () {

  qDebug () << "fill_map_cache period " << map_fill_period;
  map_cache_fill_timer->stop();
  map_cache_fill_timer->start (map_fill_period);

  //if (offline_mode) { return; }

  // 4 hours
  const int EXPIRE_AREA_DESC_SECS = -60*60*4;

  // It stinks to just poll the server like this...
  // Instead we should send it a list of our areas
  // and it can reply if we need to change any

  // 60 seconds
  const int UPDATE_AREA_DESC_SECS = -60;

  QDateTime expire_stamp
    (QDateTime::currentDateTime().addSecs(EXPIRE_AREA_DESC_SECS));

  QDateTime update_stamp
    (QDateTime::currentDateTime().addSecs(UPDATE_AREA_DESC_SECS));


  QMutableMapIterator<QString,AreaDesc*> i (*signal_maps);

  while (i.hasNext()) {
    i.next();

    AreaDesc *area = i.value();

    if (area == NULL || update_stamp > area->get_last_update_time()) {

      qDebug () << "update_stamp " << update_stamp;

      if (area == NULL) {
	qDebug () << "last_update: area is null";
      } else {
	qDebug () << "last_update " << area->get_last_update_time();
      }

      int version = -1;
      if (area != NULL) {
	version = area->get_map_version();
      }

      request_area_map (i.key(), version);

    } else if (expire_stamp > area->get_last_access_time()) {
      qDebug() << "area expired= " << i.key();
      i.remove();
      delete area;
    }

  }

}

void Localizer::request_area_map (QString area_name, int /*version*/) {

  QNetworkRequest request;

  QString area_url(setting_mapserver_url_value);
  area_url.append("/map");
  area_url = area_url.trimmed ();
  area_url.append ("/");
  area_url.append (area_name);
  area_url.append ("/");
  area_url.append ("sig.xml");
  area_url = area_url.trimmed ();
  /*
    if (version >= 0) {
    area_url.append ("/");
    area_url.append (version);
    }
  */
  qDebug() << "request_area_map " << area_url;

  request.setUrl(area_url);

  set_network_request_headers (request);

  // TODO use "head" to only request if new data
  /*
    if (area_map_reply == NULL) {
    qDebug () << "area_map_reply is NULL";
    } else {
    qDebug () << "area_map_reply is NOT NULL";
    }
  */

  //area_map_network_manager->get (request);

  // TODO move up
  if (!area_map_reply_in_flight) {
    area_map_reply = network_access_manager->get (request);
    qDebug () << "area_map_reply creation " << area_map_reply;  
    connect (area_map_reply, SIGNAL(finished()), 
	     SLOT (area_map_response()));
    area_map_reply_in_flight = true;
  } else {
    qDebug () << "area_map_reply in flight: dropping request" << area_url;
  }

}

//void Localizer::area_map_response (QNetworkReply* area_map_reply) {
void Localizer::area_map_response () {

  qDebug () << "area_map_response";
  qDebug () << "area_map_reply" << area_map_reply;

  area_map_reply->deleteLater ();
  area_map_reply_in_flight = false;
  //qDebug () << "just returning from area_map_response";
  //return;

  QString path = area_map_reply->url().path();
  // ug.  sorry.
  // TODO change URL so that map is always prepended
  //qDebug() << "pathA="<< path;
  path.remove (0,MOLE_MAPSERVER_URL_CHOP);
  //qDebug() << "pathB="<< path;

  // chop off the sig.xml at the end
  path.remove (path.length()-8,path.length()-1);
  //qDebug() << "pathC="<< path;

  if (area_map_reply->error() != QNetworkReply::NoError) {
    qWarning() << "area_map_response request failed "
	       << area_map_reply->errorString()
      	       << " url " << area_map_reply->url();

    if (area_map_reply->error() == QNetworkReply::ContentNotFoundError) {
      int count = signal_maps->remove (path);
      if (count != 1) {
	qWarning () << "area_map_response request failed "
		    << " unexpected path count in signal map "
		    << " path= " << path
		    << " count= " << count;
      } else {
	unlink_map (path);
      }
      online = true;
      stats->add_network_success_rate (1);
    } else {
      online = false;
      stats->add_network_success_rate (0);
    }
    //area_map_reply->deleteLater ();
    //check_network_state ();
    return;
  }

  online = true;
  stats->add_network_success_rate (1);
  //check_network_state ();

  //qDebug() << "area_map_response request succeeded path=" << reply->url().path(); 

  // pre-empt any parsing if we've already got the most recent
  // This could be made more effecient with a 'head' 
  // at cost of two RTs
  QDateTime last_modified =
    (area_map_reply->header(QNetworkRequest::LastModifiedHeader)).toDateTime();
  //qDebug () << "last " << last_modified;

  // Can be null if we've only inserted the key and not the value
  if (signal_maps->contains (path) && signal_maps->value(path) != NULL) {
    qDebug() << "testing age of new map vs old";
    AreaDesc *existing_area_desc = signal_maps->value(path);
    if (existing_area_desc->get_last_modified_time() == last_modified) {
      // note when we last checked this area desc
      existing_area_desc->set_last_update_time();
      qDebug() << "skipping map update because unchanged";
      //area_map_reply->deleteLater ();
      return;
    }
  }

  // TODO sanity check on the response
  QByteArray map_as_byte_array = area_map_reply->readAll();

  // write it to a file, for offline positioning if need be
  //write_to_uid_file (maps_dir, "map-", map_as_byte_array);

  save_map (path, map_as_byte_array);

  if (!parse_map (map_as_byte_array, last_modified)) {
    qWarning () << "parse_map error " << path;
  }

  //area_map_reply->deleteLater ();
}

bool Localizer::parse_map (const QByteArray &map_as_byte_array, const QDateTime &last_modified) {

  bool ok = false;
  QString map_as_xml(map_as_byte_array);

  // Parse the incoming XML
  // TODO Apply any in-memory user binds
  // Update the signal_map (area_name) -> area_desc

  MapParser parser;
  QXmlInputSource source;
  // not that it matters, but it looks like we do not need to
  // convert to a string first
  //qDebug () << "received xml shows blank mac";
  //qDebug () << "received xml " << map_as_xml;
  source.setData (map_as_xml);
  QXmlSimpleReader reader;
  reader.setContentHandler (&parser);
  reader.parse (source);

  if (!parser.get_fq_area().isEmpty()) {

    QString fq_area = parser.get_fq_area();

    if (parser.get_area_desc() != NULL) {
  
      // out with the old
      if (signal_maps->contains (fq_area)) {
	qDebug() << "dropping old map fq_area=" << fq_area;
	AreaDesc *old_map = signal_maps->value(fq_area);

	delete old_map;
      }

      // in with the new
      AreaDesc *new_map = parser.get_area_desc();
      new_map->set_last_modified_time (last_modified);
      signal_maps->insert (fq_area, new_map);
      qDebug() << "inserted new map fq_area=" << fq_area;

      //dirty = true;
      emit location_data_changed ();
      ok = true;

    } else {
      qWarning () << "xml parse: no area_desc fq_name=" << fq_area;
    }

  } else {
    qWarning() << "xml parse: no fq_area";
  }

  return ok;

}


// Pick a "loud" mac at random.
// If no loud macs exist, just pick any one.
// Idea is to make selection of areas non-deterministic.

QString Localizer::get_loud_mac () {

  if (active_macs->size() == 0) {
    return "";
  }

  const int min_rssi = -70;
  QList<QString> loud_macs;

  // subset of observed macs, which are loud
  QMapIterator<QString,MacDesc*> i (*active_macs);
  while (i.hasNext()) {
    i.next();
    if (i.value()->max > min_rssi) {
      loud_macs.push_back (i.key());
    }
  }


  if (loud_macs.size() > 0) {
    int loud_mac_index = rand_int (0, loud_macs.size()-1);
    return loud_macs[loud_mac_index];

  } else {

    int loud_mac_index = rand_int (0, active_macs->size()-1);

    QMapIterator<QString,MacDesc*> i (*active_macs);
    int index = 0;
    while (i.hasNext()) {
      i.next();
      if (index == loud_mac_index) {
	return i.key();
      }
      index++;
    }

  }

  qFatal ("not reached");
  return "";
}

double Localizer::mac_overlap_coefficient
(QMap<QString,MacDesc*> *macs_a, QSet<QString> *macs_b) {

  int c_intersection = 0;
  int c_union = macs_a->size();

  QMapIterator<QString,MacDesc*> i (*macs_a);
  while (i.hasNext()) {
    i.next();
    if (macs_b->contains(i.key())) {
      c_intersection++;
    } else {
      c_union++;
    }
  }

  //   double c = (((double) c_intersection / (double) c_union) +
  // 	      ((double) c_intersection / (double) macs_a->size())) / 2.;

  double c = (((double) c_intersection / (double) c_union) +
	      ((double) c_intersection / (double) macs_a->size()) +
	      ((double) c_intersection / (double) macs_b->size())) / 3.;


  // TODO consider adding missing term
  // (double) c_intersection / (double) macs_b->size() / 3

  return c;

}

void Localizer::save_map (QString path, const QByteArray &map_as_byte_array) {

  QString dir_name = map_root->absolutePath();
  dir_name.append ("/");
  dir_name.append (path);

  QDir map_dir (dir_name);
  if (!map_dir.exists()) {

    bool ret = map_root->mkpath (path);
    if (!ret) {
      qFatal ("Failed to create map directory");
      QCoreApplication::exit (-1);
    }
    qDebug () << "created map dir " << dir_name;

  }

  QString file_name = map_dir.absolutePath();
  file_name.append ("/sig.xml");
  qDebug () << "file_name" << file_name;
  QFile file (file_name);

  if (!file.open (QIODevice::WriteOnly | QIODevice::Truncate)) {
    qFatal ("Could not open file to store map");
    QCoreApplication::exit (-1);
  }

  QDataStream stream (&file);
  stream << map_as_byte_array;
  file.close ();

}

void Localizer::unlink_map (QString path) {

  QString dir_name = map_root->absolutePath();
  dir_name.append ("/");
  dir_name.append (path);

  QDir map_dir (dir_name);

  bool rm_ok = map_dir.remove ("sig.xml");
  if (!rm_ok) {
    qWarning () << "Failed to remove sig.xml from map_dir= " << map_dir;
    return;
  }

  rm_ok = map_root->rmpath (path);

  if (!rm_ok) {
    qWarning () << "Failed to remove path= " << path;
  }

}

void Localizer::load_maps () {

  QDateTime current_time = QDateTime::currentDateTime();
  QDirIterator it (*map_root, QDirIterator::Subdirectories);
  while (it.hasNext()) {
    it.next();
    qDebug() << "it path" << it.filePath();
    qDebug() << "it name" << it.fileName();

    if (it.fileName() == "sig.xml") {
      QFile file (it.filePath());
      if (!file.open (QIODevice::ReadOnly)) {
	qWarning () << "Could not read map file " << it.fileName();
	return;
      }
      QByteArray map_as_byte_array;
      QDataStream stream (&file);
      stream >> map_as_byte_array;
      if (!parse_map (map_as_byte_array, current_time)) {
	qWarning () << "parse_map error " << it.filePath();
      }
      file.close();

    }
    
  }

}

AreaDesc::AreaDesc () {
  last_access_time = QDateTime::currentDateTime();
  last_modified_time = QDateTime::currentDateTime();
  last_update_time = QDateTime::currentDateTime();
  macs = new QSet<QString>;
  spaces = new QMap<QString,SpaceDesc*> ();
}

AreaDesc::~AreaDesc () {
  delete macs;
  QMapIterator<QString,SpaceDesc*> i (*spaces);
  while (i.hasNext()) {
    i.next();
    SpaceDesc *space_desc = i.value();
    delete space_desc;
  }
  delete spaces;
}

QDateTime AreaDesc::get_last_access_time() {
  return last_access_time;
}

QDateTime AreaDesc::get_last_modified_time() {
  return last_modified_time;
}

QDateTime AreaDesc::get_last_update_time() {
  return last_update_time;
}

void AreaDesc::set_last_update_time() {
  last_update_time = QDateTime::currentDateTime();
}

void AreaDesc::set_last_modified_time(QDateTime _last_modified_time) {
  last_modified_time = _last_modified_time;
}


void AreaDesc::accessed() {
  last_access_time = QDateTime::currentDateTime();
}

// Not doing for now, because not updating without replacement
//void AreaDesc::updated() {
//  last_update_time = QDateTime::currentDateTime();
//}

int AreaDesc::get_map_version() {
  return map_version;
}

QMap<QString,SpaceDesc*>* AreaDesc::get_spaces() {
  return spaces;
}

QSet<QString>* AreaDesc::get_macs() {
  return macs;
}

SpaceDesc::SpaceDesc () {
  macs = new QSet<QString> ();
  sigs = new QMap<QString,Sig*> ();
}

SpaceDesc::~SpaceDesc () {
  delete macs;
  QMapIterator<QString,Sig*> i (*sigs);
  while (i.hasNext()) {
    i.next();
    Sig *sig = i.value();
    delete sig;
  }
  delete sigs;  
}


/*
  TODO move this into the localizer -- it is not a stat per se
  It is really an error message.
  Have it sent with the estimate

  void Localizer::check_network_state () {
  int level = 0;
  if (stats->network_success_rate > 0.98) {
  level = 1;
  }
  if (stats->network_success_rate < 0.8) {
  level = -1;
  }

  if (network_success_level != level) {
  network_success_level = level;

  QDBusMessage msg = QDBusMessage::createSignal("/", "com.nokia.moled", "NetworkState");
  msg << level;
  qDebug () << "sent network state " << level;
  QDBusConnection::sessionBus().send(msg);
  }
  }
*/

LocalizerStats::LocalizerStats (QObject *parent) : QObject(parent) {
  start_time = QDateTime::currentDateTime();

  emit_new_location_count = 0;
  last_emit_location = QTime::currentTime();
  last_scan_time = QTime::currentTime();

  scan_queue_size = 0;
  macs_seen_size = 0;

  network_latency = 0;
  network_success_rate = 0.8;
  ap_per_sig_count = 0;
  ap_per_scan_count = 0;
  total_area_count = 0;
  total_space_count = 0;
  potential_area_count = 0;
  potential_space_count = 0;

  // TODO this is not accurate because it only gets updated when there is a change, not when there isn't
  // should be updates per time
  emit_new_location_sec = 0;
  scan_rate_ms = 0;
  movement_detected_count = 0;

  overlap_diff = 0.;
  overlap_max = 0.;

  log_timer = new QTimer (this);
  connect (log_timer, SIGNAL (timeout()),
	   SLOT (log_statistics ()));
  log_timer->start (30000);
}

void LocalizerStats::add_network_latency(int value){
  network_latency = update_ewma (network_latency, value);
}
void LocalizerStats::add_network_success_rate(int value){ 
  network_success_rate = update_ewma (network_success_rate, value);
  qDebug () << "net success " << network_success_rate;
}

void LocalizerStats::emitted_new_location () {
  emit_new_location_count++;

  // TODO do this in a more sensible way:
  // i.e. what is the *recent* update rate
  //emit_new_location_ms = 
  //update_ewma (emit_new_location_ms, last_emit_location.elapsed());
  //last_emit_location = QTime::currentTime();

  // computed in emit_statistics

}

void LocalizerStats::received_scan () {
  qDebug () << "received_scan "
	    << "rate " << scan_rate_ms
	    << "elapsed " << last_scan_time.elapsed();
  scan_rate_ms = last_scan_time.elapsed();
  // TODO make this the average of the last e.g. ten minutes
  //update_ewma (scan_rate_ms, last_scan_time.elapsed());
  last_scan_time = QTime::currentTime();
}

void LocalizerStats::add_ap_per_sig_count(int count) { 
  ap_per_sig_count = update_ewma (ap_per_sig_count, count);
}

void LocalizerStats::add_ap_per_scan_count(int count){ 
  ap_per_scan_count = update_ewma (ap_per_scan_count, count);
}

void LocalizerStats::add_overlap_diff(double value){ 
  overlap_diff = update_ewma (overlap_diff, value);
}

void LocalizerStats::add_overlap_max(double value){ 
  qDebug () << "overlap "
	    << "max " << overlap_max
	    << "value " << value;
  overlap_max = update_ewma (overlap_max, value);
}


void LocalizerStats::movement_detected () {
  movement_detected_count++;
}

/*
  void LocalizerStats::add_total_area_count(int){ }
  void LocalizerStats::add_total_space_count(int){ }
  void LocalizerStats::add_potential_area_count(int){ }
  void LocalizerStats::add_potential_space_count(int){ }
  void LocalizerStats::add_emit_new_location_sec(int){ }
*/

double LocalizerStats::update_ewma(double current, int value){
  return (ALPHA*(double)value + ((1.0-ALPHA)*current));
}

double LocalizerStats::update_ewma(double current, double value){
  return (ALPHA*(double)value + ((1.0-ALPHA)*current));
}


bool MapParser::startDocument() {
  return true;
}

bool MapParser::endDocument() {
  return true;
}

bool MapParser::endElement (const QString&, const QString&, 
			    const QString &/*name*/) {
  return true;
}

bool MapParser::startElement (const QString&, const QString&,
			      const QString &name,
			      const QXmlAttributes &attrs) {

  if (name == "area") {
    area_desc = new AreaDesc();
    QString country, region, city, area;

    for (int i = 0; i < attrs.count(); i++) {
      if (attrs.localName(i) == "country") {
	country = attrs.value (i);
      } else if (attrs.localName(i) == "region") {
	region = attrs.value (i);
      } else if (attrs.localName(i) == "city") {
	city = attrs.value (i);
      } else if (attrs.localName(i) == "area") {
	area = attrs.value (i);
      } else if (attrs.localName(i) == "map_version") {
	area_desc->set_map_version (attrs.value (i).toInt());
      } else if (attrs.localName(i) == "builder_version") {
	builder_version = attrs.value (i).toInt();
      }
    }

    fq_area.append (country);
    fq_area.append ('/');
    fq_area.append (region);
    fq_area.append ('/');
    fq_area.append (city);
    fq_area.append ('/');
    fq_area.append (area);

    qDebug() << "parsing fq_area" << fq_area;


  } else if (name == "spaces") {

    current_space_desc = new SpaceDesc();
    QString space_name = fq_area;
    space_name.append ('/');

    // might add others in the future...
    for (int i = 0; i < attrs.count(); i++) {
      if (attrs.localName(i) == "name") {
	space_name.append (attrs.value(i));
      }
    }
    area_desc->get_spaces()->insert (space_name, current_space_desc);

    qDebug() << "parsing space_name" << space_name;

  } else if (name == "mac") {

    double avg = 0, stddev = 0, weight = 0;
    QString bssid;

    for (int i = 0; i < attrs.count(); i++) {
      if (attrs.localName(i) == "name") {
	bssid = attrs.value(i);
      } else if (attrs.localName(i) == "avg") {

	//QString val = attrs.value(i);
	//qDebug () << "val "<< val;
	//qDebug () << "val "<< val.toDouble();

	avg = attrs.value(i).toDouble();
      } else if (attrs.localName(i) == "stddev") {
	stddev = attrs.value(i).toDouble();
      } else if (attrs.localName(i) == "weight") {
	weight = attrs.value(i).toDouble();
      }

    }

    // TODO workaround for weird case where bssid is empty in xml...
    if (!bssid.isEmpty()) {
      // TODO sanity check that all values are set...
      if (stddev <= 0. || stddev > 100.0) {
	qDebug () << bssid 
		  << " avg=" << avg
		  << " stddev=" << stddev;
      }
      if (stddev == 0.) {
	qDebug () << "MapParser::startElement setting stddev";
	stddev = 1.0;
      }

      Q_ASSERT (stddev > 0.);
      Q_ASSERT (stddev < 100.);

      Q_ASSERT (avg > 0.);
      Q_ASSERT (avg < 120.);

      Q_ASSERT (weight > 0.);
      Q_ASSERT (weight < 1.);

      current_space_desc->get_sigs()->insert (bssid, new Sig (avg, stddev, weight));
      current_space_desc->get_macs()->insert (bssid);
      area_desc->get_macs()->insert (bssid);
    }

  }

  return true;
}


AreaDesc* MapParser::get_area_desc () {
  return area_desc;
}
