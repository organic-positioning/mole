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

#include <QNetworkReply>
#include <QNetworkRequest>

#include <qjson/serializer.h>

QString currentEstimateSpace;
QString unknownSpace = "??";


const int AREA_FILL_PERIOD_SHORT = 100;
const int AREA_FILL_PERIOD = 60000;
const int MAP_FILL_PERIOD_SHORT = 1000;
const int MAP_FILL_PERIOD_SOON = 15000;
const int MAP_FILL_PERIOD = 60000;

Localizer::Localizer(QObject *parent)
  : QObject(parent)
  , m_firstAddScan(true)
  , m_areaMapReplyInFlight(false)
  , m_overlap(new Overlap())
  , m_stats(new LocalizerStats(this))
  , m_fingerprint(new QMap<QString,APDesc*>())
  , m_signalMaps(new QMap<QString,AreaDesc*>())
{
  // map dir created/checked in init_mole_app
  QString mapDirName = rootDir.absolutePath();
  mapDirName.append ("/map");
  m_mapRoot = new QDir(mapDirName);

#ifdef USE_MOLE_DBUS
  QDBusConnection::sessionBus().registerObject("/", this);
  QDBusConnection::sessionBus().connect(QString(), QString(), "com.nokia.moled", "GetLocationEstimate", this, SLOT(handleLocationEstimateRequest()));
#endif

  m_areaCacheFillTimer = new QTimer(this);
  connect(m_areaCacheFillTimer, SIGNAL(timeout()), this, SLOT(fillAreaCache()));

  m_mapCacheFillTimer = new QTimer(this);
  connect(m_mapCacheFillTimer, SIGNAL(timeout()), this, SLOT(fillMapCache()));
  m_mapCacheFillTimer->start(MAP_FILL_PERIOD);

  loadMaps();

}

Localizer::~Localizer()
{
  qDebug() << "deleting localizer";

  delete m_areaCacheFillTimer;
  delete m_mapCacheFillTimer;

  // signal_maps
  qDeleteAll(m_signalMaps->begin(), m_signalMaps->end());
  m_signalMaps->clear();
  delete m_signalMaps;

  // fingerprints
  qDeleteAll(m_fingerprint->begin(), m_fingerprint->end());
  m_fingerprint->clear();
  delete m_fingerprint;

  delete m_overlap;
  delete m_stats;
  delete m_mapRoot;

  qDebug () << "finished deleting localizer";

}

void Localizer::replaceFingerprint(QMap<QString,APDesc*> *newFP)
{
  qDeleteAll(m_fingerprint->begin(), m_fingerprint->end());
  m_fingerprint->clear();
  m_fingerprint = newFP;
}

void Localizer::handleLocationEstimateRequest()
{
  qDebug() << "location estimate request";
  emitLocationEstimate();
  m_stats->emitStatistics();
}

#ifdef USE_MOLE_DBUS
void Localizer::emitLocationEstimate()
{
  QDBusMessage msg = QDBusMessage::createSignal("/", "com.nokia.moled", "LocationEstimate");
  msg << currentEstimateSpace;
  msg << networkConfigurationManager->isOnline();

  QDBusConnection::sessionBus().send(msg);
}
#else
void Localizer::emitLocationEstimate()
{
}
#endif

void Localizer::localize(int scanQueueSize)
{
  m_stats->addApPerScanCount(m_fingerprint->size());
  m_stats->receivedScan();

  // now that we have a scan, start the first network timer
  if (m_firstAddScan) {
    m_areaCacheFillTimer->stop();
    m_areaCacheFillTimer->start(AREA_FILL_PERIOD_SHORT);
    m_firstAddScan = false;
  }

  if (m_fingerprint->isEmpty()) {
    qDebug() << "no known macs to localize on";
    return;
  }

  // first come up with a short list of potential areas
  // based on mac overlap
  const double minAreaMacOverlapCoefficient = 0.01;
  const double minSpaceMacOverlapCoefficient = 0.01;

  QList<QString> potentialAreas;
  QMapIterator<QString,AreaDesc*> i (*m_signalMaps);
  double maxC = 0;
  QString maxCArea;

  int validAreas = 0;
  while (i.hasNext()) {
    i.next();
    if (i.value()) {
      ++validAreas;
      double c = macOverlapCoefficient(m_fingerprint, i.value()->macs());
      if (c > maxC) {
        maxC = c;
        maxCArea = i.key();
      }
      qDebug() << "area " << i.key() << " c=" << c;
      if (c > minAreaMacOverlapCoefficient) {
        potentialAreas.push_back(i.key());
        i.value()->accessed();
      }
    }
  }

  if (scanQueueSize > 0)
    m_stats->setScanQueueSize(scanQueueSize);

  m_stats->setMacsSeenSize(m_fingerprint->size());
  m_stats->setTotalAreaCount(validAreas);

  if (verbose) {
    qDebug() << "areas top "<< maxCArea << " c=" << maxC
             << " count=" << potentialAreas.size()
             << " valid=" << validAreas
             << " nomap=" << (m_signalMaps->size() - validAreas);
  }

  if (potentialAreas.isEmpty()) {
    qDebug() << "eliminated all areas";
    return;
  }

  m_stats->setPotentialAreaCount(potentialAreas.size());

  // next narrow down to a subset of spaces
  // from these areas
  QMap<QString,SpaceDesc*> potentialSpaces;
  QListIterator<QString> j (potentialAreas);
  maxC = 0.;
  QString maxCSpace;
  int totalSpaceCount = 0;

  while (j.hasNext()) {
    QString areaName = j.next();
    AreaDesc *area = m_signalMaps->value(areaName);
    QMap<QString,SpaceDesc*> *spaces = area->spaces();
    QMapIterator<QString,SpaceDesc*> k (*spaces);
    area->touch();

    while (k.hasNext()) {
      k.next();
      if (k.value()) {
        double c = macOverlapCoefficient(m_fingerprint, k.value()->macs());
        if (c > maxC) {
          maxC = c;
          maxCSpace = k.key();
          maxCArea = areaName;
        }
        ++totalSpaceCount;

        if (verbose) {
          qDebug() << "potential space " << k.key() << " c=" << c
                   << " ok=" << (c > minSpaceMacOverlapCoefficient);
        }
        if (c > minSpaceMacOverlapCoefficient)
          potentialSpaces.insert(k.key(), k.value());

      }
    }
  }

  int potentialSpacesSize = potentialSpaces.size();

  qDebug() << "spaceCount=" << potentialSpacesSize
           << "pct=" << potentialSpacesSize/(double)totalSpaceCount
           << "scanCount=" << scanQueueSize;

  m_stats->setTotalSpaceCount(totalSpaceCount);
  m_stats->setPotentialSpaceCount(potentialSpacesSize);

  makeOverlapEstimateWithHist(potentialSpaces, 4);

}

void Localizer::makeOverlapEstimateWithHist(QMap<QString,SpaceDesc*> &ps, int penalty)
{
  QString maxSpace;
  double maxScore = -5.;

  QMapIterator<QString,SpaceDesc*> it (ps);

  while (it.hasNext()) {
    it.next();
    double score = m_overlap->compareHistOverlap((QMap<QString,Sig*>*)m_fingerprint,
                                                it.value()->signatures(), penalty);

    qDebug() << "overlap compute: space="<< it.key() << " score="<< score;

    if (score > maxScore) {
      maxScore = score;
      maxSpace = it.key();
    }
  }

  if (penalty == 4) {
    if (!maxSpace.isEmpty())
      emitNewLocationEstimate(maxSpace, maxScore);

    m_stats->addOverlapMax(maxScore);
    m_stats->emitStatistics();
  }

  qDebug() <<"=== WEGO ESTIMATE USING HISTOGRAM (KERNEL) ===";
  qDebug() <<"Wego location estimate using kernel histogram: "
           << "penalty " << penalty
           << maxSpace << maxScore;

}

// TODO add coverage estimate, as int? suggested space?
void Localizer::emitNewLocationEstimate(QString estimatedSpaceName, double estimatedSpaceScore)
{
  qDebug() << "emit_new_location_estimate " << estimatedSpaceName
           << " score=" << estimatedSpaceScore;

  currentEstimateScore = estimatedSpaceScore;
  if (currentEstimateSpace != estimatedSpaceName) {
    currentEstimateSpace = estimatedSpaceName;
    m_stats->emittedNewLocation();
    emitLocationEstimate();
    emitEstimateToMonitors();
  }

}

void Localizer::queryCurrentEstimate(QString &country, QString &region,
                                     QString &city, QString &area,
                                     QString &space, QString &tags, double &score)
{
  QStringList list = currentEstimateSpace.split("/");

  if (currentEstimateSpace == unknownSpace || list.size() != 5) {
    if (currentEstimateSpace != unknownSpace) {
      qWarning() << "queryCurrentEstimate list " << list.size()
                 << "est" << currentEstimateSpace;
    }

    country = unknownSpace;
    region = unknownSpace;
    city = unknownSpace;
    area = unknownSpace;
    space = unknownSpace;
    tags = "";
    score = -1;
  } else {
    country = list.at(0);
    region = list.at(1);
    city = list.at(2);
    area = list.at(3);
    space = list.at(4);
    // tags not implemented yet
    tags = "";
    score = currentEstimateScore;
  }
}

void Localizer::fillAreaCache()
{
  m_areaCacheFillTimer->stop();
  m_areaCacheFillTimer->start(AREA_FILL_PERIOD);

  if (!networkConfigurationManager->isOnline()) {
    qDebug() << "aborting fill_area_cache because offline";
    return;
  }

  QString lMac = loudMac();

  if (!lMac.isEmpty()) {

    QNetworkRequest request;
    QString urlStr = mapServerURL;
    QUrl url(urlStr.append("/getAreas"));

    url.addQueryItem("mac", lMac);
    request.setUrl(url);

    qDebug() << "req " << url;

    setNetworkRequestHeaders(request);

    if (!m_macReplyInFlight) {
      m_macReply = networkAccessManager->get(request);
      qDebug() << "mac_reply creation " << m_macReply;
      connect(m_macReply, SIGNAL(finished()), SLOT(macToAreasResponse()));
      m_macReplyInFlight = true;
    } else {
      qDebug() << "mac_reply_in_flight: dropping request" << url;
    }
  }
}

void Localizer::macToAreasResponse()
{
  qDebug() << "mac_to_areas_response";
  qDebug() << "reply" << m_macReply;

  m_macReplyInFlight = false;
  m_macReply->deleteLater();

  int elapsed = findReplyLatencyMsec(m_macReply);
  m_stats->addNetworkLatency(elapsed);

  if (m_macReply->error() != QNetworkReply::NoError) {
    qWarning() << "mac_to_areas_response request failed "
               << m_macReply->errorString()
               << " url " << m_macReply->url();
    m_stats->addNetworkSuccessRate(0);
    return;
  }
  m_stats->addNetworkSuccessRate(1);
  qDebug() << "mac_to_areas_response request succeeded";

  // process response one line at a time
  // each line is the path to an area
  const int bufferSize = 256;
  char buffer[bufferSize];
  memset(buffer, 0, bufferSize);
  bool newAreaFound = false;

  int length = m_macReply->readLine(buffer, bufferSize);
  while (length != -1) {
    QString areaName(buffer);
    areaName = areaName.trimmed();
    if (areaName.length() > 1 && !m_signalMaps->contains(areaName)) {
      // create empty slot in signal map for this space
      m_signalMaps->insert(areaName, 0);
      newAreaFound = true;
      qDebug() << "getArea found new area_name=" << areaName;
    } else {
      if (verbose) {
        qDebug() << "getArea found existing area_name=" << areaName;
      }

      // make sure we have an up-to-date map for the place
      // where probably are
      AreaDesc *area = m_signalMaps->value(areaName);
      if (area) {
        qDebug() << "touching area" << areaName;
        area->touch();
      } else {
        qDebug() << "NOT touching area" << areaName;
      }
    }
    memset(buffer, 0, bufferSize);
    length = m_macReply->readLine(buffer, bufferSize);
  }

  if (newAreaFound) {
    m_mapCacheFillTimer->stop();
    m_mapCacheFillTimer->start(MAP_FILL_PERIOD_SHORT);
  }

}

// request this area name soon -- called by binder
void Localizer::touch(QString areaName)
{
  AreaDesc *area = m_signalMaps->value(areaName);
  if (area) {
    qDebug() << "touching area" << areaName;
    area->touch();
  } else {
    qDebug() << "touch did not find area" << areaName;
    m_signalMaps->insert(areaName, 0);
  }

  if (networkConfigurationManager->isOnline()) {
    m_mapCacheFillTimer->stop();
    m_mapCacheFillTimer->start(MAP_FILL_PERIOD_SOON);
  }
}

void Localizer::fillMapCache()
{
  qDebug() << "fill_map_cache period " << MAP_FILL_PERIOD;
  m_mapCacheFillTimer->stop();
  m_mapCacheFillTimer->start(MAP_FILL_PERIOD);

  if (!networkConfigurationManager->isOnline()) {
    qDebug() << "aborting fill_map_cache because offline";
    return;
  }

  m_areaMapRequests.clear();

  // 4 hours
  const int EXPIRE_AREA_DESC_SECS = -60*60*4;

  // It stinks to just poll the server like this...
  // Instead we should send it a list of our areas
  // and it can reply if we need to change any

  QDateTime expireStamp(QDateTime::currentDateTime().addSecs(EXPIRE_AREA_DESC_SECS));

  QMutableMapIterator<QString,AreaDesc*> i (*m_signalMaps);

  while (i.hasNext()) {
    i.next();
    AreaDesc *area = i.value();
    if (!area || area->isTouched()) {
      QDateTime lastUpdateTime;
      if (!area) {
        qDebug() << "last_update: area is null";
      } else {
        qDebug() << "last_update " << area->lastUpdateTime()
                 << "touch=" << area->isTouched();
        lastUpdateTime = area->lastUpdateTime();
        area->untouch();
      }


      int version = -1;
      if (!area)
        version = area->mapVersion();

      enqueueAreaMapRequest(i.key(), lastUpdateTime);

    } else if (expireStamp > area->lastAccessTime()) {
      qDebug() << "area expired= " << i.key();
      i.remove();
      delete area;
    }
  }

  if (!m_areaMapRequests.isEmpty())
    issueAreaMapRequest();

}

void Localizer::enqueueAreaMapRequest(QString areaName, QDateTime lastUpdateTime)
{
    QNetworkRequest request;
    QString areaUrl(staticServerURL);
    areaUrl.append("/map/");
    areaUrl.append (areaName);
    areaUrl.append ("/sig.xml");
    areaUrl = areaUrl.trimmed ();

    qDebug() << "enqueueAreaMap" << areaUrl;
    request.setUrl(areaUrl);
    setNetworkRequestHeaders(request);

    if (!lastUpdateTime.isNull()) {
      request.setRawHeader("If-Modified-Since", lastUpdateTime.toString().toAscii());
      qDebug() << "if-modified-since" << lastUpdateTime;
    }

    m_areaMapRequests.enqueue(request);
}

void Localizer::issueAreaMapRequest()
{
  qDebug() << "issueAreaMapRequest size" << m_areaMapRequests.size();
  if (!m_areaMapRequests.isEmpty()) {
    QNetworkRequest request = m_areaMapRequests.dequeue();
    requestAreaMap(request);
  }
}

void Localizer::requestAreaMap(QNetworkRequest request)
{
  qDebug() << "requestAreaMap inFlight" << m_areaMapReplyInFlight;

  if (!m_areaMapReplyInFlight) {
    m_areaMapReply = networkAccessManager->get(request);
    qDebug() << "area_map_reply creation " << m_areaMapReply;
    connect(m_areaMapReply, SIGNAL(finished()), SLOT(handleAreaMapResponseAndReissue()));
    m_areaMapReplyInFlight = true;
  } else {
    // happens if we have no internet access
    // and the previous request hasn't timed out yet
    qWarning () << "area_map_reply in flight: dropping request" << request.url();
  }

}

void Localizer::handleAreaMapResponseAndReissue()
{
  handleAreaMapResponse();
  issueAreaMapRequest();
}

void Localizer::handleAreaMapResponse()
{
  qDebug() << "area_map_response";
  qDebug() << "area_map_reply" << m_areaMapReply;

  m_areaMapReply->deleteLater();
  m_areaMapReplyInFlight = false;

  // handle 304 not modified case
  QVariant httpStatus = m_areaMapReply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
  if (!httpStatus.isNull()) {
    int status = httpStatus.toInt();
    if (status == 304) {
      qDebug() << "area_map_response: not modified";
      m_stats->addNetworkSuccessRate(1);
      return;
    }
  }

  QString path = m_areaMapReply->url().path();

  path.remove(QRegExp("^.*/map/"));

  // chop off the sig.xml at the end
  path.remove(path.length() - 8, path.length() - 1);

  if (m_areaMapReply->error() != QNetworkReply::NoError) {
    qWarning() << "area_map_response request failed "
               << m_areaMapReply->errorString()
               << " url " << m_areaMapReply->url();

    if (m_areaMapReply->error() == QNetworkReply::ContentNotFoundError) {
      // we delete the in-memory space.
      // Note that this will wipe out an in-memory bind, however this is the correct behavior.
      // This only happens if the remote fp server exists but does not have the file.
      int count = m_signalMaps->remove(path);
      if (count != 1) {
        qWarning() << "area_map_response request failed "
                   << " unexpected path count in signal map "
                   << " path= " << path
                   << " count= " << count;
      } else {
        unlinkMap(path);
      }
      m_stats->addNetworkSuccessRate (1);
    } else {
      m_stats->addNetworkSuccessRate (0);
    }

    return;
  }

  m_stats->addNetworkSuccessRate(1);

  // pre-empt any parsing if we've already got the most recent
  // This could be made more effecient with a 'head'
  // at cost of two RTs
  QDateTime lastModified = (m_areaMapReply->header(QNetworkRequest::LastModifiedHeader)).toDateTime();

  // Can be null if we've only inserted the key and not the value
  if (m_signalMaps->contains(path) && m_signalMaps->value(path)) {
    qDebug() << "testing age of new map vs old";
    AreaDesc *existingAreaDesc = m_signalMaps->value(path);
    if (existingAreaDesc->lastModifiedTime() == lastModified) {
      // note when we last checked this area desc
      existingAreaDesc->setLastUpdateTime();
      qDebug() << "skipping map update because unchanged";
      return;
    }
  }

  // TODO sanity check on the response
  QByteArray mapAsByteArray = m_areaMapReply->readAll();

  saveMap(path, mapAsByteArray);

  if (!parseMap(mapAsByteArray, lastModified))
    qWarning() << "parseMap error " << path;

}


// Pick a "loud" mac at random.
// If no loud macs exist, just pick any one.
// Idea is to make selection of areas non-deterministic.
QString Localizer::loudMac()
{
  if (m_fingerprint->isEmpty()) {
    return "";
  }

  const int minRssi = -80;
  QList<QString> loudMacs;

  // subset of observed macs, which are loud
  QMapIterator<QString,APDesc*> i (*m_fingerprint);
  while (i.hasNext()) {
    i.next();
    if (i.value()->loudest() > minRssi) {
      loudMacs.push_back(i.key());
    }
  }


  if (!loudMacs.isEmpty()) {
    int loudMacIndex = randInt(0, loudMacs.size() - 1);
    return loudMacs[loudMacIndex];
  } else {
    int loudMacIndex = randInt(0, m_fingerprint->size() - 1);

    QMapIterator<QString,APDesc*> i (*m_fingerprint);
    int index = 0;
    while (i.hasNext()) {
      i.next();
      if (index == loudMacIndex)
        return i.key();

      ++index;
    }
  }

  qFatal("loud mac not reached");
  return "";
}

double Localizer::macOverlapCoefficient(const QMap<QString,APDesc*> *macsA, const QList<QString> &macsB)
{
  int cIntersection = 0;
  int cUnion = macsA->size();

  QListIterator<QString> i (macsB);
  while (i.hasNext()) {
    QString mac = i.next();
    if (macsA->contains(mac)) {
      ++cIntersection;
    } else {
      ++cUnion;
    }
  }

  double c = (((double) cIntersection / (double) cUnion) +
             ((double) cIntersection / (double) macsA->size()) +
             ((double) cIntersection / (double) macsB.size())) / 3.;

  return c;
}

void Localizer::bind(QString fqArea, QString fqSpace)
{
  qDebug() << "in-memory bind" << fqSpace;

  AreaDesc *areaDesc;
  if (!m_signalMaps->contains(fqArea)) {
    areaDesc = new AreaDesc();
    m_signalMaps->insert(fqArea, areaDesc);
    qDebug() << "bind added new area" << fqArea;
  }
  areaDesc = m_signalMaps->value(fqArea);

  SpaceDesc *spaceDesc = new SpaceDesc((QMap<QString,Sig*> *) m_fingerprint);

  // set the area's macs
  QMapIterator<QString,APDesc*> i (*m_fingerprint);
  while (i.hasNext()) {
    i.next();
    areaDesc->insertMac(i.key());
  }

  QMap<QString,SpaceDesc*> *spaces = areaDesc->spaces();
  if (spaces->contains(fqSpace)) {
    SpaceDesc *oldSpaceDesc = spaces->value(fqSpace);
    delete oldSpaceDesc;
    qDebug () << "bind replaced space" << fqSpace << "in area" << fqArea;
  } else {
    qDebug () << "bind added new space" << fqSpace << "in area" << fqArea;
  }

  spaces->insert(fqSpace, spaceDesc);
  qDebug() << "fingerprint area count" << m_fingerprint->size();
}

void Localizer::addMonitor(QTcpSocket *socket)
{
  qDebug() << "Localizer::addMonitor";

  if (!m_monitoringSockets.contains(socket)) {
    m_monitoringSockets.insert(socket);
  } else {
    qWarning("Localizer::addMonitor socket already present");
  }

}

// See LocalServer::handleStats and handleQuery
void Localizer::emitEstimateToMonitors()
{
  if (m_monitoringSockets.isEmpty())
    return;

  qDebug() << "Localizer::emitEstimateToMonitors";

  QByteArray reply = estimateAndStatsAsJson();
  QMutableSetIterator<QTcpSocket*> it (m_monitoringSockets);

  while (it.hasNext()) {
    QTcpSocket *socket = it.next();
    if (!socket || !(socket->state() == QAbstractSocket::ConnectedState)) {
      it.remove();
    } else {
      qDebug() << "writing estimate to socket";
      socket->write(reply);
    }
  }
}

QByteArray Localizer::estimateAndStatsAsJson()
{
  QVariantMap map;
  QVariantMap placeMap;
  QVariantMap statsMap;

  m_stats->statsAsMap(statsMap);
  estimateAsMap(placeMap);

  map.insert("estimate", placeMap);
  map.insert("stats", statsMap);

  QJson::Serializer serializer;
  const QByteArray json = serializer.serialize(map);

  return json;
}

void Localizer::estimateAsMap(QVariantMap &placeMap)
{
  QString country, region, city, area, space, tags;
  double score;
  queryCurrentEstimate(country, region, city, area, space, tags, score);
  placeMap.insert("country", country);
  placeMap.insert("region", region);
  placeMap.insert("city", city);
  placeMap.insert("area", area);
  placeMap.insert("space", space);
  placeMap.insert("tags", tags);
  placeMap.insert("score", score);
}
