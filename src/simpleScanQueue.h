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

#ifndef SIMPLESCANQUEUE_H_
#define SIMPLESCANQUEUE_H_

#include <QtCore>
#include "scanner.h"

const int MAX_SCANQUEUE_READINGS = 50;
const int MAX_SCANQUEUE_SCANS = 10;

class Reading
{
  friend bool operator== (Reading &s1, Reading &s2);

 public:
  Reading();
  void serialize(QVariantMap &map);
  void set(QString _mac, QString _ssid, qint16 _frequency, qint8 _strength);
  void clear() { m_mac.clear(); m_ssid.clear(); m_frequency = 0; m_strength = 0; }

 private:
  QString m_mac;
  QString m_ssid;
  qint16 m_frequency;
  qint8 m_strength;
};

class Scan
{
 public:
  Scan();
  void addReading(QString mac, QString ssid, qint16 frequency, qint8 strength);
  void serialize(QVariantMap &map);
  void clear();

  friend bool operator== (Scan &s1, Scan &s2);

 private:
  int m_currentReading;
  QDateTime m_timestamp;
  Reading m_readings[MAX_SCANQUEUE_READINGS];
  void stamp() { m_timestamp = QDateTime::currentDateTime(); }
};

class SimpleScanQueue : public QObject
{
  Q_OBJECT

    public:
  SimpleScanQueue(QObject *parent = 0);
  void serialize(QVariantMap &map);

  public slots:
  void addReading(QString mac, QString ssid, qint16 frequency, qint8 strength);
  void scanCompleted();

  signals:
  void scanQueueCompleted();

 private:
  qint8 m_currentScan;
  //qint8 m_currentReading;
  //qint8 m_activeScanCount;

  QSet<QString> m_seenMacs;

  Scan m_scans[MAX_SCANQUEUE_SCANS];

};

#endif /* SIMPLESCANQUEUE_H_ */
