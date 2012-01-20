/*
 * Mole - Mobile Organic Localisation Engine
 * Copyright 2010-2012 Nokia Corporation.
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
#include "binder.h"

#include "mole.h"
#include "settings_access.h"
#include "network.h"
#include "source.h"
#include "localizer.h"
#include "scanQueue.h"
#include "dbus.h"

#include <QNetworkReply>
#include <QNetworkRequest>
//#include <qtm12/QtLocation/QSystemDeviceInfo>
#include <QSystemDeviceInfo>

#include <qjson/serializer.h>

QTM_USE_NAMESPACE

Binder::Binder(QObject *parent, Localizer *_localizer, ScanQueue *_scanQueue)
  : QObject(parent)
  , m_localizer(_localizer)
  , m_scanQueue(_scanQueue)
{
  // This keeps scans disjoint: it prevents a scan from being assigned
  // to more than one bind.
  m_oldestValidScan = QDateTime::currentDateTime();

  if (!rootDir.exists("binds")) {
    bool ret = rootDir.mkdir("binds");

    qDebug() << "root " << rootDir.absolutePath();

    if (!ret) {
      qFatal("Failed to create binds directory. Exiting...");
      QCoreApplication::exit(-1);
    }
    qDebug() << "created binds dir";
  }

  m_bindDirName = rootDir.absolutePath();
  m_bindDirName.append("/binds");

  m_bindsDir = new QDir(m_bindDirName);

  m_deviceDesc = getDeviceInfo();

#ifdef USE_MOLE_DBUS
  qDebug () << "Binder listening on D-Bus";
  QDBusConnection::systemBus().registerObject("/", this);
  QDBusConnection::systemBus().connect
    (QString(), QString(), 
     "com.nokia.moled", "Bind", this, SLOT
     (handleBindRequest (QString,QString,QString,QString,QString,
			 int, QString,QString)));
  /*
  QDBusConnection::sessionBus().registerObject("/", this);
  QDBusConnection::sessionBus().connect
    (QString(), QString(), 
     "com.nokia.moled", "Bind", this, SLOT
     (handleBindRequest (QString,QString,QString,QString,QString,
			 int, QString,QString)));
  */
#warning binder using d-bus
#else
#warning binder not using d-bus

#endif

  connect(&m_xmitBindTimer, SIGNAL(timeout()), this, SLOT(xmitBind()));

  if (networkConfigurationManager->isOnline()) {
    qDebug () << "started bind timer m_xmitBindTimer";
    m_xmitBindTimer.start(10000);
  }

  connect(networkConfigurationManager, SIGNAL(onlineStateChanged(bool)), this, SLOT(onlineStateChanged(bool)));

}

Binder::~Binder()
{
  qDebug () << "deleting binder";
  delete m_bindsDir;
}

QString Binder::handleBindRequest(QString source, QString country, 
				  QString region,
                                  QString city, QString area,
                                  int floor, QString space, QString tags)
{
  QVariantMap bindMap;
  const int MAX_NAME_LENGTH = 80;

  source = source.toLower();
  qDebug () << "source " << source;
  if (source == "add" || source == "fix" || source == "validate" || 
      source == "remove" || source == "auto") {
    // nop
  } else {
    return "Error: invalid bind source";
  }

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

  if (source != "validate" && space.isEmpty()) {
    return "Error: space name is manditory";
  }

  // relative bind?
  if (country.isEmpty() || region.isEmpty() || city.isEmpty() || area.isEmpty()) {
    if (country.isEmpty() && region.isEmpty() && city.isEmpty() && area.isEmpty()) {
      qDebug() << "relative bind";
      QString spaceIgnored;
      QString tagsIgnored;
      double scoreIgnored;

      m_localizer->queryCurrentEstimate(country, region, city, area, spaceIgnored, tagsIgnored, floor, scoreIgnored);
      if (country == unknownSpace) {
        return "Error: cannot perform relative bind because there is no current estimate";
      }
      // we are willing to fill in the whole thing if we are validating
      if (source == "validate") {
	space = spaceIgnored;
      }

    } else {
      return "Error: missing parts of full space name but not relative bind";
    }
  }

  QString fullSpaceName;
  fullSpaceName.append(country);
  fullSpaceName.append("/");
  fullSpaceName.append(region);
  fullSpaceName.append("/");
  fullSpaceName.append(city);
  fullSpaceName.append("/");
  fullSpaceName.append(area);
  fullSpaceName.append("/");
  fullSpaceName.append(QString::number(floor));
  QString areaName = fullSpaceName;
  fullSpaceName.append("/");
  fullSpaceName.append(space);

  QVariantMap locationMap;
  locationMap.insert("full_space_name", fullSpaceName);

  QVariantMap estLocationMap;
  QString estimatedSpaceName = m_localizer->currentEstimate();
  qDebug() << "bind est_space_name=" << estimatedSpaceName;
  estLocationMap.insert("full_space_name", estimatedSpaceName);

  bindMap.insert("location", locationMap);
  bindMap.insert("est_location", estLocationMap);

  // note: sends *seconds* not ms
  bindMap.insert("bind_stamp", QDateTime::currentDateTime().toTime_t());
  bindMap.insert("device_model", m_deviceDesc);
  bindMap.insert("wifi_model", m_wifiDesc);

  if (source == "remove") {
    bool found = m_localizer->removeSpace(areaName, fullSpaceName);
    if (!found) {
      return "Error: cannot remove unknown space";
    }
  } else {
    // do not send scans when removing
    QVariantList bindScansList;
    m_scanQueue->serialize(m_oldestValidScan, bindScansList);
    m_oldestValidScan = QDateTime::currentDateTime();
    bindMap.insert("ap_scans", bindScansList);
  }

  bindMap.insert("tags", tags);
  bindMap.insert("description", "none");
  bindMap.insert("source", source);

  // create a Serializer instance
  QJson::Serializer serializer;
  const QByteArray serialized = serializer.serialize(bindMap);

  // write it to a file
  QString fileName = m_bindsDir->absolutePath();
  fileName.append ("/");
  QString bindFileName = "bind-";
  bindFileName.append(getRandomCookie(10));
  bindFileName.append(".txt");
  fileName.append(bindFileName);
  qDebug() << "file_name" << fileName;
  QFile file(fileName);

  if (!file.open (QIODevice::WriteOnly | QIODevice::Truncate)) {
    qFatal("Could not open file to store bind data");
    QCoreApplication::exit(-1);
  }

  // remember this area name so we can tell
  // the localizer to do a refresh when
  // we hear the bind was successful
  m_bfn2area.insert(bindFileName, areaName);

  QDataStream stream (&file);
  stream << serialized;
  file.close();

  // apply bind to local in-memory cache
  if (source == "fix" || source == "add") {
    m_localizer->bind(areaName, fullSpaceName);
  }

  m_xmitBindTimer.start(2500);

  return "OK";

}

void Binder::xmitBind()
{
  // push the timer back a little
  m_xmitBindTimer.start(10000);

  // only send one bind at a time
  //if (m_inFlight) {
  //qDebug() << "waiting to xmit_bind until current is done";
  //return;
  //}

  // We seem to need to do this on maemo for the
  // dir listing to be refreshed properly...
  QDir tmpBindsDir (m_bindDirName);
  QStringList filters;
  filters << "bind-*.txt";
  tmpBindsDir.setNameFilters(filters);

  QStringList fileList = tmpBindsDir.entryList();

  if (fileList.length() == 0) {
    // if not, switch off the timer
    qDebug() << "no binds to transmit";
    m_xmitBindTimer.stop();
    return;
  }

  QString bindFileName = fileList.at(0);
  qDebug() << "bind file name " << bindFileName;

  QFile file(tmpBindsDir.filePath(bindFileName));

  qDebug() << "file " << file.fileName();
  QByteArray serialized;

  // if so, send it
  if (!file.open(QIODevice::ReadOnly)) {
    qWarning() << "Could not read bind file: " << file.fileName();
    m_xmitBindTimer.stop();
    return;
  }

  QDataStream stream (&file);
  stream >> serialized;

  file.close();

  QString urlStr = mapServerURL;
  QUrl url(urlStr.append("/bind"));
  QNetworkRequest request;
  request.setUrl(url);
  setNetworkRequestHeaders(request);

  request.setRawHeader("Bind", bindFileName.toAscii());

  QNetworkReply *reply = networkAccessManager->post(request, serialized);
  connect(reply, SIGNAL(finished()), SLOT (handleBindResponse()));

  qDebug() << "transmitted bind";

}

// response from MOLE server about our bind request
void Binder::handleBindResponse()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

  if (reply->error() != QNetworkReply::NoError) {
    qWarning() << "handle_bind_response request failed "
               << reply->errorString()
               << " url " << reply->url();
    m_xmitBindTimer.stop();
  } else {
    qDebug() << "handle_bind_response request succeeded";

    // delete the file that we just transmitted
    if (reply->request().hasRawHeader("Bind")) {
      QVariant bindFileVar = reply->request().rawHeader("Bind");
      QString bindFileName = bindFileVar.toString();

      qDebug() << "bind file name " << bindFileName;
      QFile file(m_bindsDir->filePath(bindFileName));
      qDebug() << "file " << file.fileName();

      if (file.exists()) {
        file.remove();
        qDebug() << "deleted sent bind file " << file.fileName();
      } else {
        qWarning() << "sent bind file does not exist " << file.fileName();
      }

      QString areaName = m_bfn2area.take(bindFileName);
      qDebug() << "handle_bind_response area_name" << areaName;
      if (!areaName.isEmpty())
        m_localizer->touch(areaName);
    }
  }
  reply->deleteLater();

}

void Binder::onlineStateChanged(bool online)
{
  qDebug() << "binder onlineStateChanged" << online;

  m_xmitBindTimer.stop();
  if (online)
    m_xmitBindTimer.start(10000);
}

void Binder::setWiFiDesc(QString _wifi_desc)
{
  qDebug() << "Binder::set_wifi_desc " << _wifi_desc;
  m_wifiDesc = _wifi_desc;
}

