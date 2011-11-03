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

#include "scanQueue.h"

#include "localizer.h"
#include "scan.h"

// Implements a circular queue of scans
// with a fixed number of readings per scan.

// The various scanners (maemo, network manager, symbian)
// add their raw readings here (via addReading)
// and then tell it when they have completed each scan
// (via scanCompleted)

const QRegExp LocallyAdministeredMAC ("^.[2367abef]:");
const QRegExp MacRegExp ("^[0-9a-f][0-9a-f]:[0-9a-f][0-9a-f]:[0-9a-f][0-9a-f]:[0-9a-f][0-9a-f]:[0-9a-f][0-9a-f]:[0-9a-f][0-9a-f]$");

ScanQueue::ScanQueue(QObject *parent, Localizer *_localizer, int _maxActiveQueueLength, bool _recordScans)
  : QObject(parent)
  , maxActiveQueueLength(_maxActiveQueueLength)
  , m_localizer(_localizer)
  , m_currentScan(0)
  , m_currentReading(0)
  , m_activeScanCount(0)
  , m_recordScans(_recordScans)
  , m_seenMacsSize(0)
  , m_responseRateTotal(0)
  , m_movementDetected(false)
{
  qDebug() << "ScanQueue maxActiveQueueLength" << maxActiveQueueLength;

  // init scan queue
  clear(-1);

}

bool ScanQueue::addReading(QString mac, QString ssid, qint16 frequency, qint8 strength)
{
  // virtual AP detection elimination

  /*
  LEAVING OFF

  simplest thing to do is to set to zero last digit
  and only accept the first one that we've seen
  and include the zeroed value in the sig

  have two piles:
  nonVirtualAPs, virtualAPs
  if in either pile, return answer
  take last digit and make it 0

  keep using original mac (without 0)
  */
  mac = mac.toLower();
  // TODO convert any - to :

  if (m_seenMacs.contains(mac)) {
    qDebug() << "skipping duplicate mac" << mac;
  } else if (mac.contains(LocallyAdministeredMAC)) {
    qDebug() << "dropping locally administered MAC" << mac;
  } else if (!mac.contains(MacRegExp)) {
    qDebug() << "skipping non MAC" << mac;
  } else {
    if (m_currentReading < MAX_SCANQUEUE_READINGS) {
      // stash this reading in the current scan
      // but do not apply it to the fingerprint yet.
      APDesc* ap = getAP(mac, ssid, frequency);

      m_scans[m_currentScan].readings[m_currentReading].set(ap, strength);

      ++m_currentReading;

      // if we have just started a new scan, set the time
      if (m_seenMacs.isEmpty())
        m_scans[m_currentScan].timestamp = QDateTime::currentDateTime();

      m_seenMacs.insert(mac);
      return true;
    } else {
      qWarning("too many readings in this scan");
    }
  }
  return false;
}

bool ScanQueue::scanCompleted()
{
  qDebug() << "sQ scanCompleted" << m_currentScan;

  m_currentReading = 0;

  if (m_seenMacs.isEmpty()) {
    qDebug() << "scanQueue: found no readings";
    return false;
  }

  int previousSeenMacsSize = m_seenMacsSize;
  m_seenMacsSize = m_seenMacs.size();

  m_seenMacs.clear();

  qDebug() << "scanSize previous" << previousSeenMacsSize << "current" << m_seenMacsSize;


  // Check for duplicate scans.
  // Duplicate readings appear to "appear" in the same order
  // so we do not need to use a hash
  // (we can just check the same index)

  if (previousSeenMacsSize == m_seenMacsSize) {
    int previousScan = m_currentScan - 1;
    if (previousScan < 0)
      previousScan = MAX_SCANQUEUE_SCANS -1;

    if (m_scans[previousScan].state == ACTIVE) {
      bool duplicateScan = true;
      for (int i = 0; duplicateScan && i < MAX_SCANQUEUE_READINGS; ++i) {
        APDesc* apC = m_scans[m_currentScan].readings[i].ap;
        APDesc* apP = m_scans[previousScan].readings[i].ap;
        if (apC && apP) {
          if (apC->mac != apP->mac || m_scans[m_currentScan].readings[i].strength
              != m_scans[previousScan].readings[i].strength)
            duplicateScan = false;
        }
      }
      if (duplicateScan) {
        qDebug() << "rejecting duplicate scan";
        return false;
      }
    }
  }

  if (m_recordScans) {
    recordCurrentScan ();
  }

  // apply the current readings to the fingerprint
  for (int i = 0; i < MAX_SCANQUEUE_READINGS; ++i) {
    APDesc* ap = m_scans[m_currentScan].readings[i].ap;
    if (ap) {
      ap->incrementUse();
      ap->addSignalStrength(m_scans[m_currentScan].readings[i].strength);
      ++m_responseRateTotal;
      m_dirtyAPs.insert(ap);
    }
  }

  if (m_movementDetected) {
    truncate();
    m_movementDetected = false;
  }

  // TODO check for extra macs vs prior scans;

  // Use 'state' to prevent partial scans from being bound
  m_scans[m_currentScan].state = ACTIVE;
  ++m_activeScanCount;


  // Here, we change any necessary values in the user's signature
  // We keep a set of dirty (changed) APs
  // and walk through them after we have added or substracted
  // any raw RSSI histogram readings.

  // Drop the oldest recent scan from the user's signature
  // if we are only keeping a limited number of active scans
  // and not using the motion detector
  if (maxActiveQueueLength > 0) {
    qDebug() << "starting expired";
    int expiringScan = m_currentScan - maxActiveQueueLength;
    if (expiringScan < 0)
      expiringScan = MAX_SCANQUEUE_SCANS + expiringScan;

    if (m_scans[expiringScan].state == ACTIVE) {
      for (int i = 0; i < MAX_SCANQUEUE_READINGS; ++i) {
        APDesc* ap = m_scans[expiringScan].readings[i].ap;
        if (ap) {
          ap->removeSignalStrength(m_scans[expiringScan].readings[i].strength);
          --m_responseRateTotal;
          if (ap->isEmpty()) {
            m_localizer->fingerprint()->remove(ap->mac);
          } else {
            m_dirtyAPs.insert(ap);
          }
        }
      }
      m_scans[expiringScan].state = INACTIVE;
      --m_activeScanCount;
      Q_ASSERT(m_activeScanCount > 0);
    }
    qDebug() << "finished expired";
  }

  // Finished processing this scan.
  // Get ready for the next set of readings.

  ++m_currentScan;
  if (m_currentScan >= MAX_SCANQUEUE_SCANS)
    m_currentScan = 0;

  // clean out any unused APs
  for (int i = 0; i < MAX_SCANQUEUE_READINGS; ++i) {
    APDesc* ap = m_scans[m_currentScan].readings[i].ap;
    if (ap) {
      ap->decrementUse();
      if (ap->useCount() <= 0) {
        // note that the ap might not be in the sig if it was
        // expired by maxActiveQueueLength
        QString apString = ap->mac;
        if (m_localizer->fingerprint()->contains(apString))
          m_localizer->fingerprint()->remove(apString);

        if (m_dirtyAPs.contains(ap))
          m_dirtyAPs.remove(ap);

        delete ap;
      } else if (m_scans[m_currentScan].state == ACTIVE) {
        ap->removeSignalStrength(m_scans[m_currentScan].readings[i].strength);
        --m_responseRateTotal;
        m_dirtyAPs.insert(ap);
      }
      m_scans[m_currentScan].readings[i].ap = 0;
    }
  }
  if (m_scans[m_currentScan].state == ACTIVE)
    --m_activeScanCount;
  Q_ASSERT(m_activeScanCount > 0);

  m_scans[m_currentScan].state = INCOMPLETE;

  // reset the normalized histogram for all changed histograms
  QSetIterator <APDesc*> dirtyIt (m_dirtyAPs);
  while (dirtyIt.hasNext()) {
    APDesc* ap = dirtyIt.next();
    ap->normalizeHistogram();
  }
  m_dirtyAPs.clear();

  // rebalance the weights for all APs
  QMapIterator <QString,APDesc*> apIt (*(m_localizer->fingerprint()));
  while (apIt.hasNext()) {
    apIt.next();
    APDesc* apDesc = apIt.value();
    apDesc->setWeight(m_responseRateTotal);
  }

  // debugging activeScanCount
  int activeScanCountTest = 0;
  for (int i = 0; i < MAX_SCANQUEUE_SCANS; ++i) {
    if (m_scans[i].state == ACTIVE)
      ++activeScanCountTest;
  }
  Q_ASSERT(m_activeScanCount == activeScanCountTest);
  Q_ASSERT(m_activeScanCount > 0);

  Q_ASSERT(m_responseRateTotal > 0);
  Q_ASSERT(m_responseRateTotal <= MAX_SCANQUEUE_READINGS*MAX_SCANQUEUE_SCANS);

  // We do not need to notify the binder at all.
  // We do tell the localizer however.
  m_localizer->localize(m_activeScanCount);

  return true;

}

void ScanQueue::serialize(QDateTime oldestValidScan, QVariantList &scanList)
{
  // walk from the oldest scan to the newest
  // skipping over any scans that are too old

  int scanIndex = m_currentScan;
  int scanCount = 0;
  int readingCount = 0;

  for (int i = 0; i < MAX_SCANQUEUE_SCANS; ++i) {
    ++scanIndex;
    if (scanIndex >= MAX_SCANQUEUE_SCANS)
      scanIndex = 0;

    if ((m_scans[scanIndex].state == ACTIVE ||
         m_scans[scanIndex].state == INACTIVE) &&
	(oldestValidScan.isNull() || m_scans[scanIndex].timestamp > oldestValidScan)) {
      QVariantList readingsList;
      for (int j = 0; j < MAX_SCANQUEUE_READINGS; ++j) {
        APDesc* ap = m_scans[scanIndex].readings[j].ap;
        if (ap) {
          QVariantMap readingMap;
          readingMap.insert ("bssid", ap->mac);
          readingMap.insert ("ssid", ap->ssid);
          readingMap.insert ("frequency", ap->frequency);
          readingMap.insert ("level", m_scans[scanIndex].readings[j].strength);
          readingsList << readingMap;
          ++readingCount;
        }
      }

      if (!readingsList.isEmpty()) {
        QVariantMap scanMap;
        scanMap.insert("stamp", m_scans[scanIndex].timestamp.toTime_t());
        scanMap.insert("readings", readingsList);
        scanList << scanMap;
        ++scanCount;
      }
    }
  }

  qDebug() << "scanQueue serialize scanCount " << scanCount << "readingCount" << readingCount;

}

APDesc* ScanQueue::getAP(QString mac, QString ssid, qint16 frequency)
{
  QString apString = mac;
  if (m_localizer->fingerprint()->contains(apString)) {
    return m_localizer->fingerprint()->value(apString);
  }
  APDesc* ap = new APDesc(mac, ssid, frequency);
  m_localizer->fingerprint()->insert(apString, ap);
  return ap;
}

void ScanQueue::handleMotionChange(Motion motion)
{
  qDebug() << "localizer got speed estimate" << motion;

  if (motion == MOVING) {
    m_movementDetected = true;
  }
  m_localizer->handleMotionChange(motion);

}

void ScanQueue::hibernate(bool /*goToSleep*/) {

}

// scale back everything except for the current scan
void ScanQueue::truncate()
{
  qDebug() << "sQ truncate start " << m_currentScan;

  // Create a new fingerprint object
  QMap<QString,APDesc*> *newFP = new QMap<QString,APDesc*> ();

  m_responseRateTotal = 0;
  m_activeScanCount = 0;
  m_dirtyAPs.clear();

  // Copy just the current scan into the new fingerprint
  // (requires the outgoing fingerprint for the ap metadata)
  // Change the scan's pointers to the new fingerprint at the same time.
  for (int i = 0; i < MAX_SCANQUEUE_READINGS; ++i) {
    APDesc* oldAP = m_scans[m_currentScan].readings[i].ap;
    if (oldAP) {
      QString apString = oldAP->mac;
      APDesc *newAP = new APDesc(oldAP->mac, oldAP->ssid, oldAP->frequency);
      newAP->incrementUse();
      newAP->addSignalStrength(m_scans[m_currentScan].readings[i].strength);
      ++m_responseRateTotal;
      m_dirtyAPs.insert(newAP);
      newFP->insert(apString, newAP);
      m_scans[m_currentScan].readings[i].ap = newAP;
    }
  }
  qDebug() << "sQ truncate before clear responseRateTotal" << m_responseRateTotal;

  // Mark all of the other scans as inactive
  clear(m_currentScan);

  qDebug() << "sQ truncate after clear responseRateTotal" << m_responseRateTotal;

  // Delete the outgoing fingerprint and
  // swap in the newly created fingerprint
  m_localizer->replaceFingerprint(newFP);

  qDebug() << "sQ truncate end " << m_currentScan << "responseRateTotal" << m_responseRateTotal;

}

void ScanQueue::clear(int ignoreScan)
{
  for (int i = 0; i < MAX_SCANQUEUE_SCANS; ++i) {
    if (i != ignoreScan) {
      m_scans[i].state = INCOMPLETE;
      for (int j = 0; j < MAX_SCANQUEUE_READINGS; ++j)
        m_scans[i].readings[j].ap = 0;
    }
  }
}

// Was going to save all this to a sqlite db.
// This would have been tidy and more compact, but probably overkill
void ScanQueue::recordCurrentScan() {

  QString scan;

  for (int i = 0; i < MAX_SCANQUEUE_READINGS; ++i) {
    APDesc* ap = m_scans[m_currentScan].readings[i].ap;
    if (ap) {
      scan.append (" ");
      scan.append (ap->mac);
      scan.append (" ");
      qint8 rssi = m_scans[m_currentScan].readings[i].strength;
      QString r = QString::number(rssi);
      scan.append (r);
      scan.append (" ");
    }
  }

  qWarning() << "SCAN" << scan;

}

QDebug operator<<(QDebug dbg, const Reading &reading)
{
  dbg.nospace() << "[";
  if (reading.ap) {
    dbg.nospace() << *(reading.ap);
    dbg.nospace() << ",rssi=" << reading.strength;
  }
  dbg.nospace() << "]";
  return dbg.space();
}

QDebug operator<<(QDebug dbg, const Scan &scan)
{
  dbg.nospace() << "[";
  switch (scan.state) {
  case INCOMPLETE:
    dbg.nospace() << "INCOMPLETE";
    break;
  case ACTIVE:
    dbg.nospace() << "ACTIVE";
    break;
  case INACTIVE:
    dbg.nospace() << "INACTIVE";
  }

  for (int i = 0; i < MAX_SCANQUEUE_READINGS; ++i) {
    dbg.nospace() << scan.readings[i];
  }
  dbg.nospace() << scan.timestamp;
  dbg.nospace() << "]";
  return dbg.space();
}

QDebug operator<<(QDebug dbg, const ScanQueue &s)
{
  dbg.nospace() << "[";
  for (int i = 0; i < MAX_SCANQUEUE_SCANS; ++i) {
    dbg.nospace() << s.m_scans[i];
    if (i == s.m_currentScan) {
      dbg.nospace() << " *";
    }
    dbg.nospace() << "\n";
  }
  dbg.nospace() << ", rr="<< s.m_responseRateTotal;
  dbg.nospace() << "]";
  return dbg.space();
}

