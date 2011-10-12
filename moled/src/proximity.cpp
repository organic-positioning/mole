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

#include "proximity.h"

Proximity::Proximity(QObject *parent, ScanQueue *_scanQueue)
  : QObject(parent)
  , m_scanQueue(_scanQueue)
  , m_active(false)
  , m_name(QString())
{

  if (settings->contains("proximity_active")) {
    m_active = settings->value("proximity_active").toBool();
  }
  if (settings->contains("proximity_name")) {
    m_name = settings->value("proximity_name").toString();
  }

#ifdef USE_MOLE_DBUS
  qDebug () << "P: listening on D-Bus";
  QDBusConnection::systemBus().registerObject("/", this);
  QDBusConnection::systemBus().connect
    (QString(), QString(), 
     "com.nokia.moled", "ProximitySettings", this, SLOT
     (handleProximitySettings (bool,QString)));
#endif

  connect(&m_updateTimer, SIGNAL(timeout()), this, SLOT(update()));

  setTimer();

}

void Proximity::update() {
  if (!networkConfigurationManager->isOnline()) {
    qDebug () << "P: no update because offline";
    return;
  }

  QVariantMap map;
  map.insert("name", m_name);
  QVariantList bindScansList;
  m_scanQueue->serialize(QDateTime(), bindScansList);
  map.insert("ap_scans", bindScansList);
  QJson::Serializer serializer;
  const QByteArray json = serializer.serialize(map);

  QString urlStr = mapServerURL;
  QUrl url(urlStr.append("/proximity"));
  QNetworkRequest request;
  request.setUrl(url);
  setNetworkRequestHeaders(request);

  QNetworkReply *reply = networkAccessManager->post(request, json);
  connect(reply, SIGNAL(finished()), SLOT (handleUpdateResponse()));
  qDebug() << "P: sent update";

}

void Proximity::handleUpdateResponse()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
  reply->deleteLater();

  if (reply->error() != QNetworkReply::NoError) {
    qWarning() << "P: handleUpdateResponse request failed "
               << reply->errorString()
               << " url " << reply->url();
  } else {

    QByteArray rawJson = reply->readAll();
    QJson::Parser parser;
    bool ok;
    QVariantMap response = parser.parse(rawJson, &ok).toMap();

    if (!ok) {
      qWarning() << "P: failed to parse json" << rawJson;
      return;
    }

    QMap<QString,double> proxMap;
    for (QVariantMap::Iterator it = response.begin(); 
	 it != response.end(); ++it) {
      QString name = it.key();
      //double score = QString::number(it.value().toReal());
      double score = it.value().toDouble();
      proxMap.insert (name, score);
      qDebug () << "P nearby: " << name << score;
    }

#ifdef USE_MOLE_DBUS
    QDBusMessage proxMsg = 
      QDBusMessage::createSignal("/", "com.nokia.moled", "Proximity");
    proxMsg << rawJson;
    QDBusConnection::systemBus().send(proxMsg);
#endif

  }

}

void Proximity::handleProximitySettings (bool activate, QString name) {
  m_active = activate;
  settings->setValue("proximity_active", activate);

  m_name = name;
  settings->setValue("proximity_name", name);

  setTimer ();
}

void Proximity::setTimer () {
  m_updateTimer.stop ();
  if (m_active) {
    m_updateTimer.start (10000);
  }
}
