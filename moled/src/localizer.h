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

#ifndef LOCALIZER_H_
#define LOCALIZER_H_

#include "moled.h"
#include "../../common/util.h"
#include "../../common/network.h"
#include "../../common/overlap.h"
#include "scan.h"

#include <QXmlDefaultHandler>
#include <QXmlInputSource>
#include <QXmlSimpleReader>

#include <QTcpSocket>
#include <QAbstractSocket>

class Binder;

class SpaceDesc
{
 public:
  SpaceDesc();
  SpaceDesc(QMap<QString,Sig*> *fingerprint);
  ~SpaceDesc();

  QMap<QString,Sig*>* signatures() { return m_sigs; }
  QList<QString> macs() { return m_sigs->keys(); }

 private:
  QMap<QString,Sig*> *m_sigs;

};

class AreaDesc
{
 public:
  AreaDesc();
  ~AreaDesc();

  QMap<QString,SpaceDesc*> *spaces() const { return m_spaces; }
  QList<QString> macs() const { return m_macs->toList(); }
  void insertMac(QString mac) { m_macs->insert(mac); }
  QDateTime lastUpdateTime() const { return m_lastUpdateTime; }
  QDateTime lastAccessTime() const { return m_lastAccessTime; }
  QDateTime lastModifiedTime() const { return m_lastModifiedTime; }
  void setLastUpdateTime() { m_lastUpdateTime = QDateTime::currentDateTime(); }
  void setLastModifiedTime(QDateTime time) { m_lastModifiedTime = time; }
  void accessed() { m_lastModifiedTime = QDateTime::currentDateTime(); }
  int mapVersion() const { return m_mapVersion; }
  void setMapVersion(int version) { m_mapVersion = version; }

  void touch() { m_touch = true; }
  bool isTouched() const { return m_touch; }
  void untouch() { m_touch = false; }

 private:
  QSet<QString> *m_macs;
  QMap<QString,SpaceDesc*> *m_spaces;
  // according to our local clock
  QDateTime m_lastUpdateTime;
  QDateTime m_lastAccessTime;
  // according to the server
  QDateTime m_lastModifiedTime;
  int m_mapVersion;
  bool m_touch;

};

class MapParser : public QXmlDefaultHandler
{
 public:
  bool startDocument() { return true; }
  bool endElement(const QString&, const QString&, const QString&) { return true; }
  bool startElement(const QString&, const QString&, const QString&, const QXmlAttributes&);
  bool endDocument() { return true; }

  AreaDesc* areaDesc() const { return m_areaDesc; }
  QString fqArea() const { return m_fqArea; }

 private:
  AreaDesc *m_areaDesc;
  SpaceDesc *m_currentSpaceDesc;
  QString m_fqArea;
  int m_builderVersion;

};

class LocalizerStats : public QObject
{
  Q_OBJECT

  // send cookie along with this

 public:
  LocalizerStats(QObject *parent);

  void emitStatistics();
  void emittedNewLocation() { ++m_emitNewLocationCount; }
  void receivedScan();

  void statsAsMap(QVariantMap &map);

  // TODO battery usage?
  // TODO scan rate
  // TODO rate of change between spaces
  void addNetworkLatency(int);
  void addNetworkSuccessRate(int);
  void addApPerSigCount(int);
  void addApPerScanCount(int);

  void addOverlapMax(double value);
  void addOverlapDiff(double value);

  void movementDetected() { ++m_movementDetectedCount; }

  void setScanQueueSize(int v) { m_scanQueueSize = v; }
  void setMacsSeenSize(int v) { m_macsSeenSize = v; }

  void setTotalAreaCount(int v) { m_totalAreaCount = v; }
  void setTotalSpaceCount(int v) { m_totalSpaceCount = v; }
  void setPotentialAreaCount(int v) { m_potentialAreaCount = v; }
  void setPotentialSpaceCount(int v) { m_potentialSpaceCount = v; }

 private:
  QTimer *m_logTimer;
  int m_scanQueueSize;
  int m_macsSeenSize;

  int m_totalAreaCount;
  int m_totalSpaceCount;
  int m_potentialAreaCount;
  int m_potentialSpaceCount;

  double m_networkLatency;
  double m_networkSuccessRate;

  double m_apPerSigCount;
  double m_apPerScanCount;

  double m_scanRateMs;
  double m_emitNewLocationSec;
  int m_emitNewLocationCount;

  double m_overlapMax;
  double m_overlapDiff;
  int m_movementDetectedCount;

  QDateTime m_startTime;
  QTime m_lastScanTime;
  QTime m_lastEmitLocation;

  double updateEwma(double current, int value) { return updateEwma(current, (double)value); }
  double updateEwma(double current, double value);

private slots:
  void logStatistics();

};

class Localizer : public QObject
{
  Q_OBJECT

public:
  Localizer(QObject *parent = 0);
  ~Localizer();

  void scanCompleted();
  void touch(QString area_name);
  QString currentEstimate() const { return currentEstimateSpace; }

  void localize(int scanQueueSize);
  QMap<QString,APDesc*> *fingerprint() const { return m_fingerprint; }
  void replaceFingerprint(QMap<QString,APDesc*> *newFP);

  void movementDetected() { m_stats->movementDetected(); }

  void queryCurrentEstimate(QString&, QString&, QString&, QString&,
                            QString&, QString&, double&);

  LocalizerStats* stats() const { return m_stats; }
  void addMonitor(QTcpSocket *socket);
  QByteArray estimateAndStatsAsJson();
  void estimateAsMap(QVariantMap &placeMap);

  void bind(QString fqArea, QString fqSpace);

private:
  bool m_firstAddScan;

  bool m_areaMapReplyInFlight;
  bool m_macReplyInFlight;
  QDir *m_mapRoot;
  Overlap *m_overlap;
  //int m_networkSuccessLevel;
  LocalizerStats *m_stats;
  QSet<QTcpSocket*> m_monitoringSockets;
  double currentEstimateScore;
  //int m_areaFillPeriod;

  QTimer *m_areaCacheFillTimer;
  QTimer *m_mapCacheFillTimer;
  QMap<QString,APDesc*> *m_fingerprint;

  QString currentEstimateSpace;

  QQueue<QNetworkRequest> m_areaMapRequests;
  QNetworkReply *m_areaMapReply;
  QNetworkReply *m_macReply;

  QMap<QString,AreaDesc*> *m_signalMaps;

  double macOverlapCoefficient(const QMap<QString,APDesc*> *macs_a,
                               const QList<QString> &macs_b);

  void enqueueAreaMapRequest(QString areaName, QDateTime lastUpdateTime);
  void issueAreaMapRequest();
  void requestAreaMap(QNetworkRequest request);
  void handleAreaMapResponse();

  QString loudMac();

  void emitNewLocationEstimate(QString estimatedSpaceName,
                               double estimatedSpaceScore);
  void emitLocationEstimate();
  void emitStatistics();

  void emitNewLocalSignature();
  void emitEstimateToMonitors();

  void makeOverlapEstimate(QMap<QString,SpaceDesc*> &);
  void makeOverlapEstimateWithHist(QMap<QString,SpaceDesc*> &, int penalty);

  //Bayes
  void makeBayesEstimate(QMap<QString,SpaceDesc*> &);
  void makeBayesEstimateWithHist(QMap<QString,SpaceDesc*> &);
  QSet<QString> findSignatureApMacs(const QMap<QString,SpaceDesc*> &);
  QSet<QString> findNovelApMacs(const QMap<QString, SpaceDesc*> &);
  double probabilityEstimate(int rssi, const Sig&);
  double probabilityEstimateWithHistogram(int rssi, const Sig&);
  double probabilityXLessValue(double value, double mean, double std);
  double erfcc(double x);

private slots:
  void fillAreaCache();
  void fillMapCache();

  void macToAreasResponse();
  void handleAreaMapResponseAndReissue();
  bool parseMap(const QByteArray &mapAsByteArray, const QDateTime &lastModified);
  void saveMap(QString path, const QByteArray &mapAsByteArray);
  void unlinkMap(QString path);
  void loadMaps();

  void handleLocationEstimateRequest();

};

extern QString currentEstimateSpace;
extern QString unknownSpace;

#endif /* LOCALIZER_H_ */
