/*
 * Mole - Mobile Organic Localisation Engine
 * Copyright 2010-2011 Nokia Corporation.
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

QString unknownSpace = "??";

#define area_fill_period_short   100
#define area_fill_period       60000
#define map_fill_period_short   1000
#define map_fill_period_soon   15000
//#define map_fill_period        5000
#define map_fill_period        60000


//bool offline_mode = true;

Localizer::Localizer(QObject *parent) :
  QObject(parent) {

  online = true;
  first_add_scan = true;

  area_map_reply_in_flight = false;
  mac_reply_in_flight = false;

  // contains the queue of currently active scans
  // i.e. scans we are using to localize

  // mapping from macs in scan_queue to stuff about them
  // e.g. their max rssi, count
  fingerprint = new QMap<QString,APDesc*> ();
  //recent_active_macs = new QMap<QString,APDesc*> ();

  overlap = new Overlap ();

  stats = new LocalizerStats (this);
  network_success_level = 10;

  // map dir created/checked in init_mole_app
  QString map_dir_name = rootDir.absolutePath();
  map_dir_name.append ("/map");
  map_root = new QDir (map_dir_name);



  currentEstimateSpace = unknownSpace;

#ifdef USE_MOLE_DBUS
  QDBusConnection::sessionBus().registerObject("/", this);
  QDBusConnection::sessionBus().connect(QString(), QString(), "com.nokia.moled", "GetLocationEstimate", this, SLOT(handle_location_estimate_request()));
#endif

  area_cache_fill_timer = new QTimer(this);
  connect(area_cache_fill_timer, SIGNAL(timeout()), 
	  this, SLOT(fill_area_cache()));

  map_cache_fill_timer = new QTimer(this);
  connect(map_cache_fill_timer, SIGNAL(timeout()), 
	  this, SLOT(fill_map_cache()));
  map_cache_fill_timer->start (map_fill_period);


  signal_maps = new QMap<QString,AreaDesc*>;  

  load_maps();

}

Localizer::~Localizer () {
  qDebug () << "deleting localizer";

  delete area_cache_fill_timer;
  delete map_cache_fill_timer;

  // signal_maps
  qDeleteAll(signal_maps->begin(), signal_maps->end());
  signal_maps->clear();
  delete signal_maps;

  delete overlap;
  delete stats;
  delete map_root;

  qDebug () << "finished deleting localizer";

}

void Localizer::replaceFingerprint(QMap<QString,APDesc*> *newFP) {
  qDeleteAll(fingerprint->begin(), fingerprint->end());
  fingerprint->clear();
  fingerprint = newFP;
}

void Localizer::handle_location_estimate_request() {
  qDebug() << "location estimate request";
  emit_location_estimate ();
  stats->emit_statistics ();
}

#ifdef USE_MOLE_DBUS
void Localizer::emit_location_estimate () {
  QDBusMessage msg = QDBusMessage::createSignal("/", "com.nokia.moled", "LocationEstimate");
  msg << currentEstimateSpace;
  msg << online;

  QDBusConnection::sessionBus().send(msg);
}
#else
void Localizer::emit_location_estimate () { }
#endif

void Localizer::localize (int scanQueueSize) {

  stats->add_ap_per_scan_count (fingerprint->size());
  stats->received_scan ();
  //emit_new_local_signature ();

  // now that we have a scan, start the first network timer
  if (first_add_scan) {
    area_cache_fill_timer->stop ();
    area_cache_fill_timer->start (area_fill_period_short);
    first_add_scan = false;
  }

  //last_localized_time = QTime::currentTime();

  if (fingerprint->size() == 0) {
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

      double c = mac_overlap_coefficient (fingerprint, i.value()->getMacs());
      if (c > max_c) {
	max_c = c;
	max_c_area = i.key();
      }

      qDebug() << "area " << i.key() << " c=" << c;
      if (c > min_area_mac_overlap_coefficient) {
	potential_areas.push_back (i.key());
	i.value()->accessed();
      }
    }
  }

  if (scanQueueSize > 0) {
    stats->set_scan_queue_size (scanQueueSize);
  }
  stats->set_macs_seen_size (fingerprint->size());
  stats->set_total_area_count (valid_areas);


  //  if (verbose) {
    qDebug() << "areas top "<< max_c_area << " c=" << max_c
	     << " count=" << potential_areas.size()
	     << " valid=" << valid_areas 
	     << " nomap=" << (signal_maps->size() - valid_areas);
    //  }


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
    QMap<QString,SpaceDesc*> *spaces = area->getSpaces ();
    QMapIterator<QString,SpaceDesc*> k (*spaces);
    area->touch();

    while (k.hasNext()) {

      k.next();
      
      if (k.value() != NULL) {

	double c = mac_overlap_coefficient (fingerprint, k.value()->getMacs());
	if (c > max_c) {
	  max_c = c;
	  max_c_space = k.key();
	  max_c_area = area_name;
	}

	total_space_count++;

	//if (verbose) {
	  qDebug() << "potential space " << k.key() << " c=" << c
		   << " ok=" << (c > min_space_mac_overlap_coefficient);
	  //}
	if (c > min_space_mac_overlap_coefficient) {
	  potential_spaces.insert (k.key(), k.value());
	}

      }

    }

  }

  qDebug() << "spaceCount=" << potential_spaces.size()
	   << "pct=" << potential_spaces.size()/(double)total_space_count
	   << "scanCount=" << scanQueueSize;

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

  //Wego with gaussian overlapping
  //make_overlap_estimate (potential_spaces);
  //Wego with histogram (kernel estimator) overlapping

  // estimate emitted for 0 below
  //make_overlap_estimate_with_hist (potential_spaces, -1);
  //make_overlap_estimate_with_hist (potential_spaces, 0);
  //make_overlap_estimate_with_hist (potential_spaces, 1);
  //make_overlap_estimate_with_hist (potential_spaces, 2);
  make_overlap_estimate_with_hist (potential_spaces, 4);
  //Bayes with gaussian sensor model
  //make_bayes_estimate (potential_spaces);
  //Bayes with histogram (kernel estimator) model
  //make_bayes_estimate_with_hist (potential_spaces);

}

/*

void Localizer::make_overlap_estimate 
(QMap<QString,SpaceDesc*> &potential_spaces) {

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

  qDebug() <<"=== WEGO ESTIMATE ===";
  qDebug() << "Wego location estimate: " << max_space << max_score;
}

*/

void Localizer::make_overlap_estimate_with_hist(QMap<QString,SpaceDesc*> &ps, int penalty)
{
  QString max_space;
  double max_score = -5.;

  QMapIterator<QString,SpaceDesc*> it (ps);

  while (it.hasNext()) {
    it.next();
    double score = overlap->compareHistOverlap((QMap<QString,Sig*>*)fingerprint,
					       it.value()->getSignatures(), penalty);

    qDebug () << "overlap compute: space="<< it.key() << " score="<< score;

    if (score > max_score) {
      max_score = score;
      max_space = it.key();
    }

  }

  if (penalty == 4) {

    if (!max_space.isEmpty()) {
      emit_new_location_estimate (max_space, max_score);
    }
    stats->add_overlap_max (max_score);
    stats->emit_statistics ();
  }

  qDebug() <<"=== WEGO ESTIMATE USING HISTOGRAM (KERNEL) ===";
  qDebug() <<"Wego location estimate using kernel histogram: " 
	   << "penalty " << penalty 
	   << max_space << max_score;

}


// TODO add coverage estimate, as int? suggested space?
void Localizer::emit_new_location_estimate 
(QString estimated_space_name, double estimated_space_score) {


  qDebug () << "emit_new_location_estimate " << estimated_space_name
	    << " score=" << estimated_space_score;

  currentEstimateScore = estimated_space_score;
  if (currentEstimateSpace != estimated_space_name) {
    currentEstimateSpace = estimated_space_name;
    stats->emitted_new_location ();
    emit_location_estimate ();
    emitEstimateToMonitors ();
  }
}

void Localizer::queryCurrentEstimate
(QString &country, QString &region, QString &city, QString &area, QString &space, QString &tags, double &score) {

  QStringList list = currentEstimateSpace.split("/");
  
  if (currentEstimateSpace == unknownSpace || list.size() != 5) {
    if (currentEstimateSpace != unknownSpace) {
      qWarning () << "queryCurrentEstimate list " << list.size() 
		  << "est" << currentEstimateSpace;
    }

    country = unknownSpace;
    region = unknownSpace;
    city = unknownSpace;
    area = unknownSpace;
    space = unknownSpace;
    tags = "";
    score = -1;
  } else {
    country = list.at(0);
    region = list.at(1);
    city = list.at(2);
    area = list.at(3);
    space = list.at(4);
    // tags not implemented yet
    tags = "";
    score = currentEstimateScore;
  }
}

void Localizer::fill_area_cache() {
  area_cache_fill_timer->stop ();
  area_cache_fill_timer->start(area_fill_period);

  //if (offline_mode) { return; }  

  QString loud_mac = get_loud_mac ();

  if (!loud_mac.isEmpty()) {

    QNetworkRequest request;
    QString url_str = mapServerURL;
    QUrl url(url_str.append("/getAreas"));

    url.addQueryItem ("mac", loud_mac);
    request.setUrl(url);

    qDebug () << "req " << url;

    set_network_request_headers (request);

    if (!mac_reply_in_flight) {
      mac_reply = networkAccessManager->get (request);
      qDebug () << "mac_reply creation " << mac_reply;  
      connect (mac_reply, SIGNAL(finished()), 
	       SLOT (mac_to_areas_response()));
      mac_reply_in_flight = true;
    } else {
      qDebug () << "mac_reply_in_flight: dropping request" << url;
    }
  }

}

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
      // make sure we have an up-to-date map for the place
      // where probably are

      AreaDesc *area = signal_maps->value(area_name);
      if (area != NULL) {
	qDebug () << "touching area" << area_name;
	area->touch();	
      } else {
	qDebug () << "NOT touching area" << area_name;
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

// request this area name soon -- called by binder
void Localizer::touch (QString area_name) {
  AreaDesc *area = signal_maps->value(area_name);
  if (area != NULL) {
    qDebug () << "touching area" << area_name;
    area->touch();	
  } else {
    qDebug () << "touch did not find area" << area_name;
    signal_maps->insert(area_name, NULL); 
  }
  map_cache_fill_timer->stop ();
  map_cache_fill_timer->start (map_fill_period_soon);
}

void Localizer::fill_map_cache () {

  qDebug () << "fill_map_cache period " << map_fill_period;
  map_cache_fill_timer->stop();
  map_cache_fill_timer->start (map_fill_period);
  areaMapRequests.clear ();

  //if (offline_mode) { return; }

  // 4 hours
  const int EXPIRE_AREA_DESC_SECS = -60*60*4;

  // It stinks to just poll the server like this...
  // Instead we should send it a list of our areas
  // and it can reply if we need to change any

  // 60 seconds
  //const int UPDATE_AREA_DESC_SECS = -60;

  QDateTime expire_stamp
    (QDateTime::currentDateTime().addSecs(EXPIRE_AREA_DESC_SECS));

  //QDateTime update_stamp
  //(QDateTime::currentDateTime().addSecs(UPDATE_AREA_DESC_SECS));


  QMutableMapIterator<QString,AreaDesc*> i (*signal_maps);

  while (i.hasNext()) {
    i.next();

    AreaDesc *area = i.value();

    if (area == NULL || area->isTouched()) {

      //qDebug () << "update_stamp " << update_stamp;

      QDateTime last_update_time; 
      if (area == NULL) {
	qDebug () << "last_update: area is null";
      } else {
	qDebug () << "last_update " << area->get_last_update_time()
		  << "touch=" << area->isTouched();
	last_update_time = area->get_last_update_time();
	area->untouch();
      }


      int version = -1;
      if (area != NULL) {
	version = area->get_map_version();
      }

      enqueueAreaMapRequest (i.key(), last_update_time);

    } else if (expire_stamp > area->get_last_access_time()) {
      qDebug() << "area expired= " << i.key();
      i.remove();
      delete area;
    }
  }

  if (areaMapRequests.size() > 0) {
    issueAreaMapRequest ();
  }
}

void Localizer::enqueueAreaMapRequest (QString area_name, 
				       QDateTime last_update_time) {
    QNetworkRequest request;
    QString area_url(staticServerURL);
    area_url.append("/map/");
    area_url.append (area_name);
    area_url.append ("/sig.xml");
    area_url = area_url.trimmed ();
    /*
      if (version >= 0) {
      area_url.append ("/");
      area_url.append (version);
      }
    */

    qDebug() << "enqueueAreaMap" << area_url;
    request.setUrl(area_url);
    set_network_request_headers (request);

    if (!last_update_time.isNull()) {
      request.setRawHeader 
	("If-Modified-Since", last_update_time.toString().toAscii());
      qDebug () << "if-modified-since" << last_update_time;
    }

    areaMapRequests.enqueue (request);
}

void Localizer::issueAreaMapRequest () {
  qDebug () << "issueAreaMapRequest size" << areaMapRequests.size();
  if (areaMapRequests.size() > 0) {
    QNetworkRequest request = areaMapRequests.dequeue ();
    requestAreaMap (request);
  }
}

void Localizer::requestAreaMap (QNetworkRequest request) {
  qDebug () << "requestAreaMap inFlight" << area_map_reply_in_flight;

  if (!area_map_reply_in_flight) {
    area_map_reply = networkAccessManager->get (request);
    qDebug () << "area_map_reply creation " << area_map_reply;  
    connect (area_map_reply, SIGNAL(finished()), 
	     SLOT (handleAreaMapResponseAndReissue()));
    area_map_reply_in_flight = true;
  } else {
    // happens if we have no internet access
    // and the previous request hasn't timed out yet
    qWarning () << "area_map_reply in flight: dropping request" << request.url();
  }

}

void Localizer::handleAreaMapResponseAndReissue () {
  handleAreaMapResponse ();
  issueAreaMapRequest ();
}

void Localizer::handleAreaMapResponse () {

  qDebug () << "area_map_response";
  qDebug () << "area_map_reply" << area_map_reply;

  area_map_reply->deleteLater ();
  area_map_reply_in_flight = false;

  //qDebug () << "error code" << area_map_reply->error();

  // handle 304 not modified case
  QVariant httpStatus = 
    area_map_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
  if (!httpStatus.isNull()) {
    //qDebug () << "httpStatus " << httpStatus;
    int status = httpStatus.toInt();
    if (status == 304) {
      qDebug () << "area_map_response: not modified";
      online = true;
      stats->add_network_success_rate (1);
      return;
    }
  }

  QString path = area_map_reply->url().path();

  //qDebug() << "pathA="<< path;
  path.remove (QRegExp("^.*/map/"));
  //qDebug() << "pathB="<< path;

  // chop off the sig.xml at the end
  path.remove (path.length()-8,path.length()-1);
  //qDebug() << "pathC="<< path;

  if (area_map_reply->error() != QNetworkReply::NoError) {
    qWarning() << "area_map_response request failed "
	       << area_map_reply->errorString()
      	       << " url " << area_map_reply->url();

    if (area_map_reply->error() == QNetworkReply::ContentNotFoundError) {
      // we delete the in-memory space.
      // Note that this will wipe out an in-memory bind, however this is the correct behavior.
      // This only happens if the remote fp server exists but does not have the file.
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

  //qDebug () << "map_as_byte_array" << map_as_byte_array;

  // write it to a file, for offline positioning if need be
  //write_to_uid_file (maps_dir, "map-", map_as_byte_array);

  save_map (path, map_as_byte_array);

  if (!parse_map (map_as_byte_array, last_modified)) {
    qWarning () << "parse_map error " << path;
  }

  //area_map_reply->deleteLater ();
}


// Pick a "loud" mac at random.
// If no loud macs exist, just pick any one.
// Idea is to make selection of areas non-deterministic.

QString Localizer::get_loud_mac () {

  if (fingerprint->size() == 0) {
    return "";
  }

  const int min_rssi = -80;
  QList<QString> loud_macs;

  // subset of observed macs, which are loud
  QMapIterator<QString,APDesc*> i (*fingerprint);
  while (i.hasNext()) {
    i.next();
    if (i.value()->loudest() > min_rssi) {
      loud_macs.push_back (i.key());
    }
  }


  if (loud_macs.size() > 0) {
    int loud_mac_index = rand_int (0, loud_macs.size()-1);
    return loud_macs[loud_mac_index];

  } else {

    int loud_mac_index = rand_int (0, fingerprint->size()-1);

    QMapIterator<QString,APDesc*> i (*fingerprint);
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
(const QMap<QString,APDesc*> *macs_a, const QList<QString> &macs_b) {

  int c_intersection = 0;
  int c_union = macs_a->size();

  QListIterator<QString> i (macs_b);

  while (i.hasNext()) {
    QString mac_b = i.next();
    if (macs_a->contains(mac_b)) {
      c_intersection++;
    } else {
      c_union++;
    }
  }

  //   double c = (((double) c_intersection / (double) c_union) +
  // 	      ((double) c_intersection / (double) macs_a->size())) / 2.;

  double c = (((double) c_intersection / (double) c_union) +
	      ((double) c_intersection / (double) macs_a->size()) +
	      ((double) c_intersection / (double) macs_b.size())) / 3.;


  // TODO consider adding missing term
  // (double) c_intersection / (double) macs_b->size() / 3

  return c;

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

void Localizer::bind (QString fqArea, QString fqSpace) {
  qDebug () << "in-memory bind" << fqSpace;
  AreaDesc *areaDesc;
  if (!signal_maps->contains(fqArea)) {
    areaDesc = new AreaDesc ();
    signal_maps->insert (fqArea, areaDesc);
    qDebug () << "bind added new area" << fqArea;
  }
  areaDesc = signal_maps->value(fqArea);

  SpaceDesc *spaceDesc = new SpaceDesc ((QMap<QString,Sig*> *) fingerprint);

  // set the area's macs
  QMapIterator<QString,APDesc*> i (*fingerprint);
  while (i.hasNext()) {
    i.next();
    areaDesc->insertMac (i.key());
  }

  QMap<QString,SpaceDesc*> *spaces = areaDesc->getSpaces ();
  if (spaces->contains (fqSpace)) {
    SpaceDesc* oldSpaceDesc = spaces->value (fqSpace);
    delete oldSpaceDesc;
    qDebug () << "bind replaced space" << fqSpace << "in area" << fqArea;
  } else {
    qDebug () << "bind added new space" << fqSpace << "in area" << fqArea;
  }
  spaces->insert (fqSpace, spaceDesc);
  qDebug () << "fingerprint area count" << fingerprint->size();
}

void Localizer::addMonitor (QTcpSocket *socket) {
  qDebug () << "Localizer::addMonitor";
  if (!monitoringSockets.contains(socket)) {
    monitoringSockets.insert (socket);
  } else {
    qWarning ("Localizer::addMonitor socket already present");
  }
}

// See LocalServer::handleStats and handleQuery
void Localizer::emitEstimateToMonitors () {
  if (monitoringSockets.size() == 0) { return; }
  qDebug () << "Localizer::emitEstimateToMonitors";
  QByteArray reply = getEstimateAndStatsAsJson ();
  QMutableSetIterator<QTcpSocket*> it (monitoringSockets);
  while (it.hasNext()) {
    QTcpSocket *socket = it.next();
    if (socket == NULL || !(socket->state() == QAbstractSocket::ConnectedState)) {
      it.remove ();
    } else {
      qDebug () << "writing estimate to socket";
      socket->write(reply);
    }
  }
}

QByteArray Localizer::getEstimateAndStatsAsJson () {
  QVariantMap map;
  QVariantMap placeMap;
  QVariantMap statsMap;
  stats->getStatsAsMap (statsMap);
  getEstimateAsMap (placeMap);
  map.insert("estimate", placeMap);
  map.insert("stats", statsMap);
  QJson::Serializer serializer;
  const QByteArray json = serializer.serialize(map);
  return json;
}

void Localizer::getEstimateAsMap (QVariantMap &placeMap) {
  QString country, region, city, area, space, tags;
  double score;
  queryCurrentEstimate (country, region, city, area, space, tags, score);
  placeMap.insert("country", country);
  placeMap.insert("region", region);
  placeMap.insert("city", city);
  placeMap.insert("area", area);
  placeMap.insert("space", space);
  placeMap.insert("tags", tags);
  placeMap.insert("score", score);
}
