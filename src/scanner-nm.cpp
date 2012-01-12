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

#include "scanner-nm.h"
#include <NetworkManager/NetworkManager.h>

const int SCAN_INTERVAL_MSEC_REGULAR   = 10000;
const int SCAN_INTERVAL_MSEC_HIBERNATE = 60000;

Scanner::Scanner(QObject *parent)
  : QObject(parent)
  , m_scanInterval(SCAN_INTERVAL_MSEC_REGULAR)
  , m_haveSetDriver(false)
  , m_wifi(0)
{
  m_timer.setSingleShot(true);
  connect(&m_timer, SIGNAL(timeout()), this, SLOT(scanAccessPoints()));
}

Scanner::~Scanner()
{
}

void Scanner::handleHibernate(bool goToSleep)
{
  qDebug () << "handleHibernate" << goToSleep;
  m_timer.stop();
  if (goToSleep) {
    m_scanInterval = SCAN_INTERVAL_MSEC_HIBERNATE;
    m_timer.start(m_scanInterval);
  } else {
    m_scanInterval = SCAN_INTERVAL_MSEC_REGULAR;
    m_timer.start(0);
  }
}

void Scanner::scanAccessPoints()
{
  //qDebug() << Q_FUNC_INFO;
  m_timer.start(m_scanInterval);

  if (!m_wifi)
    initWiFi();

  int readingCount = 0;
  QDBusReply<QList<QDBusObjectPath> > reply = m_wifi->call("GetAccessPoints");

  foreach(QDBusObjectPath path, reply.value()) {
    QDBusInterface ap (NM_DBUS_SERVICE,
                       path.path(),
                       NM_DBUS_INTERFACE_ACCESS_POINT,
                       QDBusConnection::systemBus());

    if (ap.isValid()) {
      //qDebug() << Q_FUNC_INFO << "addReading";
      emit addReading(ap.property("HwAddress").toString(),
		      ap.property("Ssid").toString(),
		      (qint16)(ap.property("Frequency").toUInt()),
		      (qint8)(ap.property("Strength").toUInt()));
      ++readingCount;
    } else {
      qWarning() << " got invalid AP " << path.path();
    }
  }

  if (readingCount > 0) {
    emit scanCompleted();
  } else {
    if (m_wifi) {
      delete m_wifi;
      m_wifi = 0;
    }
    initWiFi();
  }
}

void Scanner::initWiFi()
{
  QDBusInterface interface (NM_DBUS_SERVICE,
                            NM_DBUS_PATH,
                            NM_DBUS_INTERFACE,
                            QDBusConnection::systemBus());

  QDBusReply<QList<QDBusObjectPath> > reply = interface.call("GetDevices");

  foreach(QDBusObjectPath path, reply.value()) {
    QDBusInterface device (NM_DBUS_SERVICE,
                           path.path(),
                           NM_DBUS_INTERFACE_DEVICE,
                           QDBusConnection::systemBus());
    if (device.isValid()) {
      if (device.property("DeviceType").toUInt() == DEVICE_TYPE_802_11_WIRELESS) {
        if (!m_haveSetDriver) {
	  QString desc = device.property("Driver").toString();
          emit setWiFiDesc(desc);
          m_haveSetDriver = true;
        }
        // parent?
        m_wifi = new QDBusInterface (NM_DBUS_SERVICE,
                                   path.path(),
                                   NM_DBUS_INTERFACE_DEVICE_WIRELESS,
                                   QDBusConnection::systemBus());
      }
    }
  }

  if (!m_wifi) {
    qFatal ("No WiFi interface found.");
  }

}

bool Scanner::start() {
  qDebug() << Q_FUNC_INFO;
  m_timer.start(1000);
  return true;
}

bool Scanner::stop() {
  // TODO only return true if already running?
  qDebug() << Q_FUNC_INFO;
  m_timer.stop();
  delete m_wifi;
  m_wifi = 0;
  return true;
}

