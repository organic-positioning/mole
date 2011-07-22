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

#include "binder.h"
#include "scanQueue.h"

#include <NetworkManager/NetworkManager.h>


int SCAN_INTERVAL_MSEC = 2500;
const int MIN_SCAN_INTERVAL_MSEC = 10000;
const int MAX_SCAN_INTERVAL_MSEC = 120000;

Scanner::Scanner(QObject *parent, ScanQueue *_scanQueue, Binder *_binder)
  : QObject(parent)
  , m_scanQueue(_scanQueue)
  , m_binder(_binder)
  , m_haveSetDriver(false)
  , m_wifi(0)
{
  connect(&m_timer, SIGNAL(timeout()), this, SLOT(scanAccessPoints()));
  m_timer.start(SCAN_INTERVAL_MSEC);
}

Scanner::~Scanner()
{
}

void Scanner::scanAccessPoints()
{
  m_timer.stop();

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
      m_scanQueue->addReading(ap.property("HwAddress").toString(),
                              ap.property("Ssid").toString(),
                              (qint16)(ap.property("Frequency").toUInt()),
                              (qint8)(ap.property("Strength").toUInt()));
      ++readingCount;
    } else {
      qWarning() << " got invalid AP " << path.path();
    }
  }

  if (readingCount > 0) {
    if (m_scanQueue->scanCompleted()) {
      SCAN_INTERVAL_MSEC -= 1000;
    } else {
      SCAN_INTERVAL_MSEC += 1000;
    }

    if (SCAN_INTERVAL_MSEC < MIN_SCAN_INTERVAL_MSEC) {
      SCAN_INTERVAL_MSEC = MIN_SCAN_INTERVAL_MSEC;
    } else if (SCAN_INTERVAL_MSEC > MAX_SCAN_INTERVAL_MSEC) {
      SCAN_INTERVAL_MSEC = MAX_SCAN_INTERVAL_MSEC;
    }
  } else {
    if (m_wifi)
      delete m_wifi;
    initWiFi();
  }

  m_timer.start(SCAN_INTERVAL_MSEC);

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
          m_binder->setWifiDesc(device.property("Driver").toString());
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
}

