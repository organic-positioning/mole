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

#include "localizer.h"

const qreal ALPHA = 0.20;

// periodically put a line in the log file
void LocalizerStats::logStatistics()
{
  qWarning() << "stats: "
             << "net_ok " << m_networkSuccessRate
             << "churn " << m_emitNewLocationSec
             << "scan_ms " << scanRate() //m_scanRateMs
             << "scans " << m_scanQueueSize
             << "macs " << m_macsSeenSize;
}

#ifdef USE_MOLE_DBUS
void LocalizerStats::emitStatistics()
{
  //qDebug () << "LocalizerStats::emitStatistics";
  int uptime = m_startTime.secsTo(QDateTime::currentDateTime());
  if (m_emitNewLocationCount > 0) {
    m_emitNewLocationSec = (double) uptime / (double) m_emitNewLocationCount;
  } else {
    m_emitNewLocationSec = 0;
  }

  qDebug() << "emit_statistics "
           << "network_success_rate " << m_networkSuccessRate
           << "emit_new_location_sec " << m_emitNewLocationSec
           << "scan_rate_ms " << scanRate(); // m_scanRateMs;

  QString empty = "";

  //QVariantMap rankedSpaces;


  QDBusMessage statsMsg = QDBusMessage::createSignal("/", "com.nokia.moled", "LocationStats");
  statsMsg << empty

    // convert to GMT for network recording
          << m_startTime

    // integers
          << m_scanQueueSize
          << m_macsSeenSize

          << m_totalAreaCount
          << m_totalSpaceCount
          << m_potentialAreaCount
          << m_potentialSpaceCount

          << m_movementDetectedCount

	 << scanRate() // m_scanRateMs

    // doubles
         << m_emitNewLocationSec

         << m_networkLatency
         << m_networkSuccessRate

         << m_overlapMax
         << m_overlapDiff

    // ranked spaces

	   << rankEntries;

  QDBusConnection::systemBus().send(statsMsg);
}
#else
void LocalizerStats::emitStatistics()
{
}
#endif

void LocalizerStats::clearRankEntries() {
  m_confidence = 0.;
  rankEntries.clear();
  rankScores.clear();
}

void LocalizerStats::addRankEntry(QString space, double score) {
  rankEntries.insert(space, score);
  rankScores.append(score);
}

double LocalizerStats::getConfidence() {
  if (m_confidence != 0.) {
    return m_confidence;
  }
  if (rankScores.size() < 2) {
    return 0.;
  }
  qSort(rankScores.begin(), rankScores.end(), qGreater<double>());
  m_confidence = rankScores.at(0) - rankScores.at(1);
  qDebug () << "getConfidence" << rankScores.at(0) << rankScores.at(1) << "conf" << m_confidence;
  return m_confidence;
}


void LocalizerStats::statsAsMap(QVariantMap &map)
{
  map.insert("LocalizerQueueSize", m_scanQueueSize);
  map.insert("MacsSeenSize", m_macsSeenSize);
  map.insert("TotalAreaCount", m_totalAreaCount);
  map.insert("PotentialSpaceCount", m_potentialSpaceCount);
  map.insert("ScanRate", scanRate());
  map.insert("NetworkSuccessRate", m_networkSuccessRate);
  map.insert("NetworkLatency", m_networkLatency);
  map.insert("OverlapMax", m_overlapMax);
  map.insert("OverlapDiff", m_overlapDiff);
  map.insert("Churn", (int)(round(m_emitNewLocationSec)));
}

LocalizerStats::LocalizerStats(QObject *parent)
  : QObject(parent)
  , m_scanQueueSize(0)
  , m_macsSeenSize(0)
  , m_totalAreaCount(0)
  , m_totalSpaceCount(0)
  , m_potentialAreaCount(0)
  , m_potentialSpaceCount(0)
  , m_networkLatency(0)
  , m_networkSuccessRate(0.8)
  , m_apPerSigCount(0)
  , m_apPerScanCount(0)
    //  , m_scanRateMs(0)
  , m_emitNewLocationSec(0)
  , m_emitNewLocationCount(0)
  , m_overlapMax(0.)
  , m_overlapDiff(0.)
  , m_movementDetectedCount(0)
  , m_confidence(0)
{
  m_startTime = QDateTime::currentDateTime();
  m_lastEmitLocation = QTime::currentTime();
  m_lastScanTime.start(); //= QTime::currentTime();

  m_logTimer = new QTimer(this);
  connect(m_logTimer, SIGNAL(timeout()), SLOT(logStatistics()));
  m_logTimer->start(30000);
}

void LocalizerStats::addNetworkLatency(int value)
{
  m_networkLatency = updateEwma(m_networkLatency, value);
}

void LocalizerStats::addNetworkSuccessRate(int value)
{
  m_networkSuccessRate = updateEwma(m_networkSuccessRate, value);
  qDebug() << "net success " << m_networkSuccessRate;
}

void LocalizerStats::receivedScan()
{
  qDebug() << "received_scan "
           << "elapsed " << m_lastScanTime.elapsed();
  //m_scanRateMs = m_lastScanTime.elapsed();
  // TODO make this the average of the last e.g. ten minutes
  //m_lastScanTime = QTime::currentTime();
  m_lastScanTime.restart();
}

int LocalizerStats::scanRate() 
{
  return ((int)(round(m_lastScanTime.elapsed()/1000)));
}

void LocalizerStats::addApPerSigCount(int count)
{
  m_apPerSigCount = updateEwma(m_apPerSigCount, count);
}

void LocalizerStats::addApPerScanCount(int count)
{
  m_apPerScanCount = updateEwma(m_apPerScanCount, count);
}

void LocalizerStats::addOverlapDiff(double value)
{
  m_overlapDiff = updateEwma(m_overlapDiff, value);
}

void LocalizerStats::addOverlapMax(double value)
{
  qDebug() << "overlap "
           << "max " << m_overlapMax
           << "value " << value;
  m_overlapMax = updateEwma(m_overlapMax, value);
}

double LocalizerStats::updateEwma(double current, double value)
{
  return (ALPHA * (double)value + ((1.0 - ALPHA) * current));
}
