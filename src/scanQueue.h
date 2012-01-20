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

#ifndef SCANQUEUE_H_
#define SCANQUEUE_H_

#include <QtCore>
#include "motion.h"

const int MAX_SCANQUEUE_READINGS = 50;
const int MAX_SCANQUEUE_SCANS = 60;

class APDesc;
class Localizer;

class Reading
{
 friend QDebug operator<<(QDebug dbg, const Reading &reading);

 public:
  APDesc *ap;
  qint8 strength;
  void set(APDesc *_ap, qint8 _strength) { ap = _ap; strength = _strength; }
};

// Scans start off as incomplete.
// As they are being filled with readings, they are still incomplete.
// When they are incomplete, they are not used by the localizer
// or the binder.
// When they are filled with readings (signalled by scanCompleted)
// they become active, which means they will be used for localization
// and for binding.  In this state, the readings will be mirrored by
// each APs signal strength histogram.  I.e., when a scan becomes
// active, its values are applied to the histogram, and when it
// becomes inactive, they are removed.
// Inactive scans are used by the binder, sent as part of the place's
// signature.

enum ScanState { INCOMPLETE, ACTIVE, INACTIVE };

class Scan
{
 friend QDebug operator<<(QDebug dbg, const Scan &scan);

 public:
  ScanState state;
  QDateTime timestamp;
  Reading readings[MAX_SCANQUEUE_READINGS];
};

class ScanQueue : public QObject
{
 friend QDebug operator<<(QDebug dbg, const ScanQueue &scanQueue);
  Q_OBJECT

 public:
  ScanQueue(QObject *parent = 0, Localizer *localizer = 0, int maxActiveQueueLength = 0, 
	    bool recordScans = false);

  void serialize(QDateTime oldestValidScan, QVariantList &scanList);
  void hibernate(bool goToSleep);
  bool hibernating() { return m_hibernating; }

  const int maxActiveQueueLength;

 public slots:
  bool addReading(QString mac, QString ssid, qint16 frequency, qint8 strength);
  bool scanCompleted();
  void handleMotionChange(Motion motion);

 private:
  Localizer *m_localizer;
  qint8 m_currentScan;
  qint8 m_currentReading;
  qint8 m_activeScanCount;
  bool m_recordScans;
  qint8 m_seenMacsSize;
  int m_responseRateTotal;
  bool m_movementDetected;
  bool m_hibernating;

  QSet<QString> m_seenMacs;
  QSet<APDesc*> m_dirtyAPs;

  Scan m_scans[MAX_SCANQUEUE_SCANS];

  APDesc* getAP(QString mac, QString ssid, qint16 frequency);

  void recordCurrentScan();

  void truncate();
  void clear(int ignoreScan);

};

#endif /* SCANQUEUE_H_ */
