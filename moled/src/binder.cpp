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

Binder::Binder(QObject *parent, Localizer *_localizer, ScanQueue *_scanQueue) : 
  QObject(parent), localizer (_localizer), scanQueue (_scanQueue) {

  // This keeps scans disjoint: it prevents a scan from being assigned
  // to more than one bind.
  oldestValidScan = QDateTime::currentDateTime();

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

  // TODO use something from QtMobility to access the local device/driver name
  // This is not available as of 4.7 and Mobility 1.2

  // on 4.7 seems to trigger this: QDBusObjectPath: invalid path ""
  // but gives right results...
  QSystemDeviceInfo *device = new QSystemDeviceInfo (parent);
  qDebug () << "product name  " << device->productName().simplified();
  device_desc.append (device->productName().simplified());
  device_desc.append ("/");
  qDebug () << "model " << device->model ().simplified();
  device_desc.append (device->model().simplified());
  device_desc.append ("/");
  qDebug () << "manu  " << device->manufacturer().simplified();
  device_desc.append (device->manufacturer().simplified());
  delete device;

#ifdef USE_MOLE_DBUS
  QDBusConnection::sessionBus().registerObject("/", this);
  QDBusConnection::sessionBus().connect(QString(), QString(), "com.nokia.moled", "Bind", this, SLOT(handleBindRequest(QString,QString,QString,QString,QString,QString)));
#endif

  connect(&xmit_bind_timer, SIGNAL(timeout()), 
	  this, SLOT(xmit_bind()));
  xmit_bind_timer.start (10000);

}

Binder::~Binder () {
  qDebug () << "deleting binder";
  delete binds_dir;

}

/*
void Binder::set_location_estimate
(QString _estimated_space_name, double _estimated_space_score) {
  estimated_space_name = _estimated_space_name;
  estimated_space_score = _estimated_space_score;
}
*/

QString Binder::handleBindRequest
(QString country, QString region, QString city, QString area, 
 QString space, QString tags) {

  QVariantMap bind_map;

  const int MAX_NAME_LENGTH = 80;
  // TODO validate params here
  if (country.contains("/") || region.contains("/") || city.contains("/") ||
      area.contains("/") || space.contains("/")) {
    return "Error: place names cannot contains slashes";
  }
  if (country.length() > MAX_NAME_LENGTH) {
    return "Error: country name too long";
  }
  if (region.length() > MAX_NAME_LENGTH) {
    return "Error: region name too long";
  }
  if (city.length() > MAX_NAME_LENGTH) {
    return "Error: city name too long";
  }
  if (area.length() > MAX_NAME_LENGTH) {
    return "Error: area name too long";
  }
  if (space.length() > MAX_NAME_LENGTH) {
    return "Error: space name name too long";
  }
  if (tags.length() > MAX_NAME_LENGTH) {
    return "Error: tags too long";
  }

  if (space.isEmpty()) {
    return "Error: space name is manditory";
  }

  // relative bind?
  if (country.isEmpty() || region.isEmpty() || 
      city.isEmpty() || area.isEmpty()) {
    if (country.isEmpty() && region.isEmpty() && 
	city.isEmpty() && area.isEmpty()) {
      qDebug () << "relative bind";
      QString spaceIgnored;
      QString tagsIgnored;
      double scoreIgnored;
      localizer->queryCurrentEstimate 
	(country, region, city, area, 
	 spaceIgnored, tagsIgnored, scoreIgnored);
      if (country == unknownSpace) {
	return "Error: cannot perform relative bind because there is no current estimate";
      }
    } else {
      return "Error: missing parts of full space name but not relative bind";
    }
  }

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
  full_space_name.append (space);

  QVariantMap location_map;
  location_map.insert ("full_space_name", full_space_name);

  QVariantMap est_location_map;
  QString estimated_space_name = localizer->getCurrentEstimate ();
  qDebug () << "bind est_space_name=" << estimated_space_name;
  est_location_map.insert ("full_space_name", estimated_space_name);

  bind_map.insert ("location", location_map);
  bind_map.insert ("est_location", est_location_map);

  // note: sends *seconds* not ms
  bind_map.insert ("bind_stamp", QDateTime::currentDateTime().toTime_t());
  bind_map.insert ("device_model", device_desc);
  bind_map.insert ("wifi_model", wifi_desc);

  QVariantList bind_scans_list;
  scanQueue->serialize (oldestValidScan, bind_scans_list);
  
  oldestValidScan = QDateTime::currentDateTime();

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

  localizer->bind (area_name, full_space_name);

  xmit_bind_timer.start (2500);

  return "OK";

}
 
void Binder::xmit_bind () {

  // push the timer back a little
  xmit_bind_timer.start (10000);

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
    xmit_bind_timer.stop ();
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
    xmit_bind_timer.stop ();
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
    xmit_bind_timer.stop ();

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

/*
void Binder::handle_speed_estimate(int motion) {
  qDebug () << "binder handle_speed_estimate" << motion;

  if (motion == MOVING) {
    oldestValidScan = QDateTime::currentDateTime();
    qDebug () << "handle_speed_estimate setting oldestValidScan";
  }
}
*/

void Binder::set_wifi_desc (QString _wifi_desc) {
  qDebug () << "Binder::set_wifi_desc " << _wifi_desc;
  wifi_desc = _wifi_desc;
}


// void Binder::set_device_desc (QString _device_desc) {
//   device_desc = _device_desc;
// }


