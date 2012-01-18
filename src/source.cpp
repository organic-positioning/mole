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

#include "source.h"

#include <QSettings>
#include <QSystemDeviceInfo>
QTM_USE_NAMESPACE

//////////////////////////////////////////////////////////////////////////////
// return our uuid, and store it in settings if it wasn't already there
QString getUUID() {
  QSettings settings;
  if (!settings.contains("uuid") ||
      settings.value("uuid").toString().size() < 35) {
    QString uuid;
    for (int i = 0; i < 4; i++) {
      int nextRand = qrand();
      uuid.append(QString::number(nextRand, 16));
      if (i < 3) {
        uuid.append("-");
      }
      qDebug() << "uuid" << uuid;
    }
    settings.setValue("uuid", uuid);
  }
  return settings.value("uuid").toString();
}

//////////////////////////////////////////////////////////////////////////////
// return a description of the device
QString getDeviceInfo() {
  QSettings settings;
  if (!settings.contains("deviceinfo")) {
    QSystemDeviceInfo device;
    qDebug() << "product name  " << device.productName().simplified();
    QString deviceInfo;
    deviceInfo.append(device.productName().simplified());
    deviceInfo.append("/");
    qDebug() << "model " << device.model().simplified();
    deviceInfo.append(device.model().simplified());
    deviceInfo.append("/");
    qDebug() << "manufacturer  " << device.manufacturer().simplified();
    deviceInfo.append(device.manufacturer().simplified());
    settings.setValue("deviceinfo", deviceInfo);
  }
  return settings.value("deviceinfo").toString();
}

void serializeSource(QVariantMap &map) {
  map["uuid"] = getUUID();
  map["device"] = getDeviceInfo();
  map["version"] = MOLE_VERSION;
}
