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

#include "scanner.h"
#include "simpleScanQueue.h"

SimpleScanQueue::SimpleScanQueue(QObject *parent)
  : QObject(parent)
  , m_currentScan(0)
{
  qWarning() << "Creating simpleScanQueue for" 
	     << MAX_SCANQUEUE_SCANS << "scans and"
	     << MAX_SCANQUEUE_READINGS << "readings";
    
}

Scan::Scan() : m_currentReading(0) {
  m_timestamp = QDateTime();
}

void Scan::addReading(QString mac, QString ssid, qint16 frequency, qint8 strength) {
  //qDebug() << Q_FUNC_INFO << " reading=" << m_currentReading;
  if (m_currentReading < MAX_SCANQUEUE_READINGS) {
    m_readings[m_currentReading].set (mac, ssid, frequency, strength);
    ++m_currentReading;
  } else {
    qWarning("too many readings in this scan");
  }
}

void Scan::clear() {
  for (int i = 0; i < MAX_SCANQUEUE_READINGS; i++) {
    m_readings[i].clear();
  }
  m_currentReading = 0;
  m_timestamp = QDateTime();
}

void SimpleScanQueue::addReading(QString mac, QString ssid, qint16 frequency, qint8 strength)
{
  //qDebug() << Q_FUNC_INFO << " scan=" << m_currentScan;
  mac = mac.toLower();
  // TODO convert any - to :

  if (m_seenMacs.contains(mac)) {
    qDebug() << "skipping duplicate mac" << mac;
  } else if (mac.contains(LocallyAdministeredMAC)) {
    qDebug() << "dropping locally administered MAC" << mac;
  } else if (!mac.contains(MacRegExp)) {
    qDebug() << "skipping non MAC" << mac;
  } else {
    m_scans[m_currentScan].addReading(mac, ssid, frequency, strength);
    m_seenMacs.insert(mac);
  }
}

void SimpleScanQueue::scanCompleted()
{
  qDebug() << Q_FUNC_INFO << m_currentScan;

  if (m_seenMacs.isEmpty()) {
    qDebug() << "scanQueue: found no readings";
    return;
  }

  m_seenMacs.clear();

  // Reject duplicate scans.

  if (m_currentScan > 0) {
    if (m_scans[m_currentScan] == m_scans[m_currentScan-1]) {
      qDebug() << "rejecting duplicate scan";
      m_scans[m_currentScan].clear();
      return;
    }
  }

  // mark this scan as valid
  m_scans[m_currentScan].stamp();
  m_currentScan++;
  if (m_currentScan >= MAX_SCANQUEUE_SCANS) {
    m_currentScan = 0;
  }
  // reset the data structure for the next incoming scan
  m_scans[m_currentScan].clear();
  emit scanQueueCompleted();
}

void SimpleScanQueue::handleMotionChange(Motion motion) {
  qDebug() << Q_FUNC_INFO << motion;

  if (motion == MOVING) {
    for (int i = 0; i < MAX_SCANQUEUE_SCANS; i++) {
      if (i != m_currentScan) {
	m_scans[i].clear();
      }
    }
  }
}


bool operator== (Reading &s1, Reading &s2) {
  if (s1.m_mac == s2.m_mac &&
      s1.m_ssid == s2.m_ssid &&
      s1.m_frequency == s2.m_frequency &&
      s1.m_strength == s2.m_strength) {
    return true;
  }
  return false;
}


bool operator== (Scan &s1, Scan &s2) {
  if (s1.m_currentReading != s2.m_currentReading)
    return false;
  for (int i = 0; i < s1.m_currentReading; ++i) {
    if (! (s1.m_readings[i] == s2.m_readings[i])) {
      return false;
    }
  }
  return true;
}

Reading::Reading() : m_mac(), m_ssid(), m_frequency(0), m_strength(0) {

}

void Reading::set(QString _mac, QString _ssid, qint16 _frequency, qint8 _strength) {
    m_mac = _mac;
    m_ssid = _ssid;
    m_frequency = _frequency;
    m_strength = _strength;
  }


void Reading::serialize(QVariantMap &map) {
  map.insert ("bssid", m_mac);
  map.insert ("ssid", m_ssid);
  map.insert ("frequency", m_frequency);
  map.insert ("level", m_strength);
}

void Scan::serialize(QVariantMap &map) {
  if (m_currentReading == 0) { 
    return;
  }
  QVariantList readingsList;
  for (int j = 0; j < m_currentReading; ++j) {
    QVariantMap readingMap;
    m_readings[j].serialize (readingMap);
    readingsList << readingMap;
  }
  //map.insert("stamp", m_timestamp.toTime_t());
  map.insert("readings", readingsList);
}

void SimpleScanQueue::serialize(QVariantMap &map) {
  QVariantList list;
  /*
  for (int i = 0; i < m_currentScan; ++i) {
    QVariantMap map;
    m_scans[i].serialize (map);    
    list << map;
  }
  */

  for (int i = m_currentScan+1; i != m_currentScan; ++i) {
    if (i == MAX_SCANQUEUE_SCANS) {
      i = 0;
    }
    if (m_scans[i].isValid()) {
      QVariantMap map;
      m_scans[i].serialize (map);    
      list << map;
    }
  }


  map["scans"] = list;
}


