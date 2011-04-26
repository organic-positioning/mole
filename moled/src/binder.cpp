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
#include <QSystemDeviceInfo>

#include "binder.h"

QTM_USE_NAMESPACE

Binder::Binder(QObject *parent, Localizer *_localizer) : 
  QObject(parent), localizer (_localizer) {

  //QDBusConnection::sessionBus().connect(QString(), QString(), "com.nokia.moled", "MotionEstimate", this, SLOT(handle_speed_estimate(int)));
  if (!rootDir.exists ("binds")) {
    bool ret = rootDir.mkdir ("binds");

    qDebug () << "root " << rootDir.absolutePath();

    if (!ret) {
      qFatal ("Failed to create binds directory. Exiting...");
      QCoreApplication::exit (-1);
    }
    qDebug () << "created binds dir";
  }

  bind_dir_name = rootDir.absolutePath();
  bind_dir_name.append ("/binds");

  binds_dir = new QDir (bind_dir_name);

  in_flight = false;
  //last_reset_stamp = QDateTime::currentDateTime();

  /*
    QSystemNetworkInfo info (parent);
    local_wlan_mac = info.macAddress (QSystemNetworkInfo::WlanMode);
  */

  // on 4.7 seems to trigger this: QDBusObjectPath: invalid path ""
  // but gives right results...
  QSystemDeviceInfo *device = new QSystemDeviceInfo (parent);
    //qDebug () << "imei  " << device.imei();
  qDebug () << "product name  " << device->productName().simplified();
  device_desc.append (device->productName().simplified());
  device_desc.append ("/");
  qDebug () << "model " << device->model ().simplified();
  device_desc.append (device->model().simplified());
  device_desc.append ("/");
  qDebug () << "manu  " << device->manufacturer().simplified();
  device_desc.append (device->manufacturer().simplified());
    //qDebug () << "sim   " << device->simStatus();

  delete device;

  QDBusConnection::sessionBus().registerObject("/", this);
  QDBusConnection::sessionBus().connect(QString(), QString(), "com.nokia.moled", "Bind", this, SLOT(handle_bind_request(QString,QString,QString,QString,QString,QString)));

  clean_scan_list_timer = new QTimer(this);
  connect(clean_scan_list_timer, SIGNAL(timeout()), 
	  this, SLOT(clean_scan_list()));

  // every minute
  clean_scan_list_timer->start(60*1000);

  xmit_bind_timer = new QTimer(this);
  connect(xmit_bind_timer, SIGNAL(timeout()), 
	  this, SLOT(xmit_bind()));
  xmit_bind_timer->start (10000);

  scan_list = new QList<AP_Scan*>;

  //bind_network_manager = new QNetworkAccessManager (this);
  //connect(bind_network_manager, SIGNAL(finished(QNetworkReply*)),
  //SLOT(handle_bind_response(QNetworkReply*)));


}

Binder::~Binder () {
  qDebug () << "deleting binder";
  delete clean_scan_list_timer;
  delete xmit_bind_timer;
  //delete bind_network_manager;
  QListIterator<AP_Scan *> i (*scan_list);
  while (i.hasNext()) {
    AP_Scan *scan = i.next();
    delete scan;
  }
  delete scan_list;
  delete binds_dir;

}

/*
void Binder::set_location_estimate
(QString _estimated_space_name, double _estimated_space_score) {
  estimated_space_name = _estimated_space_name;
  estimated_space_score = _estimated_space_score;
}
*/

void Binder::handle_bind_request
(QString country, QString region, QString city, QString area, 
 QString space_name, QString tags) {

  QVariantMap bind_map;

  QString full_space_name;
  full_space_name.append (country);
  full_space_name.append ("/");
  full_space_name.append (region);
  full_space_name.append ("/");
  full_space_name.append (city);
  full_space_name.append ("/");
  full_space_name.append (area);
  QString area_name = full_space_name;
  full_space_name.append ("/");
  full_space_name.append (space_name);

  QVariantMap location_map;
  location_map.insert ("full_space_name", full_space_name);

  QVariantMap est_location_map;
  QString estimated_space_name = localizer->get_location_estimate ();
  qDebug () << "bind est_space_name=" << estimated_space_name;
  est_location_map.insert ("full_space_name", estimated_space_name);

  bind_map.insert ("location", location_map);
  bind_map.insert ("est_location", est_location_map);

  // note: sends *seconds* not ms
  bind_map.insert ("bind_stamp", QDateTime::currentDateTime().toTime_t());
  bind_map.insert ("device_model", device_desc);
  bind_map.insert ("wifi_model", wifi_desc);

  QVariantList bind_scans_list;
  QListIterator<AP_Scan *> i (*scan_list);
  while (i.hasNext()) {
    AP_Scan *scan = i.next();

    QVariantList readings_list;
    QListIterator<AP_Reading *> j (*(scan->readings));
    while (j.hasNext()) {
      AP_Reading *reading = j.next();
      QVariantMap reading_map;
      reading_map.insert ("bssid", reading->bssid);
      reading_map.insert ("ssid", reading->ssid);
      reading_map.insert ("frequency", reading->frequency);
      reading_map.insert ("level", reading->level);
      readings_list << reading_map;
    }

    QVariantMap ap_scan_map;
    ap_scan_map.insert ("stamp", scan->stamp->toTime_t());
    ap_scan_map.insert ("readings", readings_list);

    bind_scans_list << ap_scan_map;
  }

  qDebug () << "bind scan_list size" << scan_list->size();


  bind_map.insert ("ap_scans", bind_scans_list);

  bind_map.insert ("tags", tags);
  bind_map.insert ("description", "none");

  // create a Serializer instance
  QJson::Serializer serializer;
  const QByteArray serialized = serializer.serialize(bind_map);

  //qDebug() << serialized;

  // write it to a file
  QString file_name = binds_dir->absolutePath();
  file_name.append ("/");
  QString bind_file_name = "bind-";
  bind_file_name.append (get_random_cookie(10));
  bind_file_name.append (".txt");
  file_name.append (bind_file_name);
  qDebug () << "file_name" << file_name;
  QFile file (file_name);

  if (!file.open (QIODevice::WriteOnly | QIODevice::Truncate)) {
    qFatal ("Could not open file to store bind data");
    QCoreApplication::exit (-1);
  }

  // remember this area name so we can tell
  // the localizer to do a refresh when
  // we hear the bind was successful
  bfn2area.insert (bind_file_name, area_name);

  QDataStream stream (&file);
  stream << serialized;
  file.close ();


  // delete up to last minute
  clean_scan_list (60);

  xmit_bind_timer->start (2500);

}
 
void Binder::xmit_bind () {

  // push the timer back a little
  xmit_bind_timer->start (10000);

  // only send one bind at a time
  if (in_flight) {
    qDebug () << "waiting to xmit_bind until current is done";
    return;
  }

  //bool bind_file_exists = false;
  // see if there is a bind to send

  // We seem to need to do this on maemo for the
  // dir listing to be refreshed properly...
  QDir tmp_binds_dir (bind_dir_name);
  QStringList filters;
  filters << "bind-*.txt";
  tmp_binds_dir.setNameFilters(filters);

  QStringList file_list = tmp_binds_dir.entryList ();

  if (file_list.length() == 0) {
    // if not, switch off the timer
    qDebug () << "no binds to transmit";
    xmit_bind_timer->stop ();
    return;
  }

  QString bind_file_name = file_list.at(0);
  qDebug () << "bfn " << bind_file_name;

  QFile file (tmp_binds_dir.filePath(bind_file_name));

  qDebug () << "file " << file.fileName();
  QByteArray serialized;


  // if so, send it
  if (!file.open (QIODevice::ReadOnly)) {
    qWarning () << "Could not read bind file: " << file.fileName();
    xmit_bind_timer->stop ();
    return;
  }

  QDataStream stream (&file);
  stream >> serialized;

  file.close();

  QString url_str = mapServerURL;
  QUrl url(url_str.append("/bind"));
  QNetworkRequest request;
  request.setUrl(url);
  set_network_request_headers (request);

  request.setRawHeader ("Bind", bind_file_name.toAscii());

  reply = networkAccessManager->post (request, serialized);
  connect (reply, SIGNAL(finished()), 
	   SLOT (handle_bind_response()));


  in_flight = true;

  qDebug () << "transmitted bind";



}

// response from MOLE server about our bind request
void Binder::handle_bind_response () {

  in_flight = false;

  if (reply->error() != QNetworkReply::NoError) {
    qWarning() << "handle_bind_response request failed "
	       << reply->errorString()
      	       << " url " << reply->url();
    xmit_bind_timer->stop ();

  } else {

    qDebug() << "handle_bind_response request succeeded";

    // delete the file that we just transmitted
    if (reply->request().hasRawHeader ("Bind")) {
      QVariant bind_file_var = reply->request().rawHeader ("Bind");
      QString bind_file_name = bind_file_var.toString();

      qDebug () << "bfn " << bind_file_name;
      QFile file (binds_dir->filePath(bind_file_name));
      qDebug () << "file " << file.fileName();

      //QFile file (bind_file_name);
      if (file.exists()) {
	file.remove();
	qDebug () << "deleted sent bind file " << file.fileName();
      } else {
	qWarning () << "sent bind file does not exist " << file.fileName();
      }

      QString area_name = bfn2area.take (bind_file_name);
      qDebug () << "handle_bind_response area_name" << area_name;
      if (!area_name.isEmpty()) {
	localizer->touch (area_name);
      }

    }

  }
  reply->deleteLater();

}

void Binder::add_scan (AP_Scan *new_scan) {
  scan_list->push_back (new_scan);
  qDebug () << "binder add_scan list size=" << scan_list->size();
}

void Binder::clean_scan_list (int expire_secs) {
  qDebug () << "binder clean_scan_list"
	    << " expire_secs " << expire_secs;

  QDateTime last_reset_stamp = QDateTime
    (QDateTime::currentDateTime().addSecs(-1 * expire_secs).toUTC());

  int old_size = scan_list->size();

  QList<AP_Scan *> *new_scan_list = new QList<AP_Scan *>;
  QListIterator<AP_Scan *> i (*scan_list);
  while (i.hasNext()) {
    AP_Scan *scan = i.next();
    if (*scan->stamp > last_reset_stamp) {
      new_scan_list->push_back (scan);
    } else {
      delete scan;
    }
  }

  qDebug () << "clean_scan_list was: " << old_size 
	    << " now: " << new_scan_list->size();

  delete scan_list;
  scan_list = new_scan_list;

}

void Binder::handle_speed_estimate(int motion) {
  qDebug () << "binder handle_speed_estimate" << motion;


  if (motion == MOVING) {
    int old_size = scan_list->size();
    QListIterator<AP_Scan *> i (*scan_list);
    while (i.hasNext()) {
      AP_Scan *scan = i.next();
      delete scan;
    }
    qDebug () << "handle_speed_estimate was: " << old_size 
	      << "now: " << scan_list->size();
    delete scan_list;
    scan_list = new QList<AP_Scan *>;
  }
}


void Binder::set_wifi_desc (QString _wifi_desc) {
  wifi_desc = _wifi_desc;
}


// void Binder::set_device_desc (QString _device_desc) {
//   device_desc = _device_desc;
// }


