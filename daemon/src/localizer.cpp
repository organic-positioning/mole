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
const QString unknownSpace = "??";


const int AREA_FILL_PERIOD_SHORT = 100;
const int AREA_FILL_PERIOD = 60000;
const int MAP_FILL_PERIOD_SHORT = 1000;
const int MAP_FILL_PERIOD_SOON = 10000;
const int MAP_FILL_PERIOD = 60000;
const int BEST_PENALTY = 4;

Localizer::Localizer(QObject *parent, bool _runAllAlgorithms)
  : QObject(parent)
  , m_runAllAlgorithms(_runAllAlgorithms)
  , m_firstAddScan(true)
  , m_forceMapCacheUpdate(true)
  , m_hibernating(false)
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
  QDBusConnection::systemBus().registerObject("/", this);
  QDBusConnection::systemBus().connect(QString(), QString(), "com.nokia.moled", "GetLocationEstimate", this, SLOT(emitLocationAndStats()));
#endif

  /*
  m_locationDataSource = 0;
  if (!m_locationDataSource) {
    // qWarning() << "sources" << QGeoPositionInfoSource::  availableSources();
    m_locationDataSource =
      QGeoPositionInfoSource::createDefaultSource(this);
      //QGeoPositionInfoSource::PositioningMethods methods = m_locationDataSource->preferredPositioningMethods();
      //qWarning () << "methods" << methods;

    //m_locationDataSource->setPreferredPositioningMethods(QGeoPositionInfoSource::NonSatellitePositioningMethods);
    if (m_locationDataSource) {
    connect(m_locationDataSource, SIGNAL(positionUpdated(QGeoPositionInfo)),
	    this, SLOT(positionUpdated(QGeoPositionInfo)));
 
    m_locationDataSource->startUpdates();

    QGeoPositionInfo position = m_locationDataSource->lastKnownPosition();
    QGeoCoordinate coordinate = position.coordinate();
    if (coordinate.isValid()) {
    qreal latitude = coordinate.latitude();
    qreal longitude = coordinate.longitude();
    qWarning() << QString("Latitude: %1 Longitude: %2").arg(latitude).arg(longitude);

    QGeoServiceProvider serviceProvider("nokia");
 
    if (serviceProvider.error() == QGeoServiceProvider::NoError) {
      m_geoSearchManager = serviceProvider.searchManager();
    }

    connect(m_geoSearchManager,
                    SIGNAL(finished(QGeoSearchReply*)),
                    this,
                    SLOT(searchResults(QGeoSearchReply*)));

            connect(m_geoSearchManager,
                    SIGNAL(error(QGeoSearchReply*,QGeoSearchReply::Error,QString)),
                    this,
                    SLOT(searchError(QGeoSearchReply*,QGeoSearchReply::Error,QString)));
            QGeoSearchReply* reply = m_geoSearchManager->reverseGeocode (coordinate);
            //connect(reply, SIGNAL(finished()), this, SLOT(handleGeoSearch()));


    } else {
      qWarning () << "coord not valid";
    }
      }
  }
  */

  connect(&m_areaCacheFillTimer, SIGNAL(timeout()), this, SLOT(fillAreaCache()));

  connect(&m_mapCacheFillTimer, SIGNAL(timeout()), this, SLOT(fillMapCache()));
  m_mapCacheFillTimer.start(MAP_FILL_PERIOD);

  loadMaps();

}

/*
void Localizer::handleGeoSearch() {
    QGeoSearchReply *reply = qobject_cast<QGeoSearchReply *>(sender());
    reply->deleteLater();
    if (reply->error() != QGeoSearchReply::NoError) {
        qWarning() << "geo search error" << reply->errorString();
    } else{
        qWarning() << "size" << reply->places().size();
        QList<QGeoPlace> places = reply->places();
        for (int i = 0; i < reply->places().size(); ++i) {
            QGeoPlace place = reply->places().at(i);
            qWarning() << "addr country" << place.address().county();
        }
    }
}


void Localizer::searchError(QGeoSearchReply *reply, QGeoSearchReply::Error error, const QString &errorString)
{
    // ... inform the user that an error has occurred ...
    qWarning("searchError");
    reply->deleteLater();
}

*/

Localizer::~Localizer()
{
  qDebug() << "deleting localizer";

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

/*
void Localizer::positionUpdated(QGeoPositionInfo geoPositionInfo) {
  if (geoPositionInfo.isValid()) {

    m_locationDataSource->stopUpdates();
 
    // Save the position information into a member variable
    m_positionInfo = geoPositionInfo;
 
    // Get the current location as latitude and longitude
    QGeoCoordinate geoCoordinate = geoPositionInfo.coordinate();
    qreal latitude = geoCoordinate.latitude();
    qreal longitude = geoCoordinate.longitude();
    qDebug() << QString("Latitude: %1 Longitude: %2").arg(latitude).arg(longitude);
  }
  qFatal ("positionUpdated");
}
*/

void Localizer::replaceFingerprint(QMap<QString,APDesc*> *newFP)
{
  qDeleteAll(m_fingerprint->begin(), m_fingerprint->end());
  m_fingerprint->clear();
  m_fingerprint = newFP;
}

void Localizer::emitLocationAndStats()
{
  qDebug() << "location estimate request";
  m_stats->emitStatistics();
  emitLocationEstimate();
}


void Localizer::emitLocationEstimate()
{
#ifdef USE_MOLE_DBUS
  QDBusMessage msg = QDBusMessage::createSignal("/", "com.nokia.moled", "LocationEstimate");
  msg << currentEstimateSpace;
  QDBusConnection::systemBus().send(msg);
  qDebug () << "emitLocationEstimate on dbus" << currentEstimateSpace;
#endif
}

void Localizer::localize(const int scanQueueSize)
{
  m_stats->addApPerScanCount(m_fingerprint->size());
  m_stats->receivedScan();

  /*
  // for debugging
  QMapIterator<QString,AreaDesc*> i (*m_signalMaps);
  while (i.hasNext()) {
    i.next();
    QString area = i.key();
    AreaDesc *desc = i.value();
    if (desc != NULL) {
      qDebug () << "new map loop C area " << desc->lastModifiedTime();      
    } else {
      qDebug () << "new map loop C area " << area << " null";
    }
  }
  */

  // now that we have a scan, start the first network timer
  if (m_firstAddScan) {
    m_areaCacheFillTimer.stop();
    m_areaCacheFillTimer.start(AREA_FILL_PERIOD_SHORT);
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
    emitNewLocationEstimate(unknownSpace, -1);
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

  qDebug () << "localizing on" << potentialAreas.size() << "areas";

  while (j.hasNext()) {
    QString areaName = j.next();
    AreaDesc *area = m_signalMaps->value(areaName);
    QMap<QString,SpaceDesc*> *spaces = area->spaces();
    QMapIterator<QString,SpaceDesc*> k (*spaces);
    qDebug () << "about to touch" << areaName;
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

  // note that scanQueueSize is 0 if we have been called by a new signal map download
  qDebug() << "spaceCount=" << potentialSpacesSize
           << "pct=" << potentialSpacesSize/(double)totalSpaceCount
           << "scanCount=" << scanQueueSize;

  m_stats->setTotalSpaceCount(totalSpaceCount);
  m_stats->setPotentialSpaceCount(potentialSpacesSize);

  if (m_runAllAlgorithms) {
    //makeBayesEstimate(potentialSpaces);
    //makeBayesEstimateWithHist(potentialSpaces);
    makeOverlapEstimateWithGaussians(potentialSpaces);
    makeOverlapEstimateWithHist(potentialSpaces, 0);
    makeOverlapEstimateWithHist(potentialSpaces, 1);
  }
  makeOverlapEstimateWithHist(potentialSpaces, BEST_PENALTY);

}

void Localizer::makeOverlapEstimateWithHist(QMap<QString,SpaceDesc*> &ps, int penalty)
{
  QString maxSpace;
  double maxScore = -5.;

  m_stats->clearRankEntries();
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

    if (penalty == BEST_PENALTY) {
      m_stats->addRankEntry(it.key(), score);
    }

  }

  double confidence = 0.;
  if (penalty == BEST_PENALTY) {
    m_stats->addOverlapMax(maxScore);
    if (!maxSpace.isEmpty())
      emitNewLocationEstimate(maxSpace, maxScore);

    confidence = m_stats->getConfidence();
  }

  qDebug() <<"=== MAO ESTIMATE USING HISTOGRAM (KERNEL) ==="
           << "penalty " << penalty
	   << "estimate" << maxSpace << maxScore
	   << "confidence" << confidence;

}

void Localizer::makeOverlapEstimateWithGaussians
(QMap<QString,SpaceDesc*> &potential_spaces) {

  QString maxSpace;
  double maxScore = -5.;
  const double init_overlap_diff = 10.;
  double overlap_diff = init_overlap_diff;
  QMapIterator<QString,SpaceDesc*> i (potential_spaces);

  while (i.hasNext()) {
    i.next();
    double score = m_overlap->compareSigOverlap
      ((QMap<QString,Sig*>*)m_fingerprint, i.value()->signatures());

    qDebug () << "overlap compute: space="<< i.key() << " score="<< score;

    double diff = maxScore - score;
    //qDebug () << "score " << score
    //<< "diff " << diff;

    // Hmm. This is not correct and not computable the way we are doing it.
    // It is also not that interesting a value.

    // Maybe s
    // difference between top and second-from-top
    if (diff < overlap_diff) {
      overlap_diff = diff;
    }

    if (score > maxScore) {
      maxScore = score;
      maxSpace = i.key();
    }

  }

  //if (overlap_diff < init_overlap_diff) {
  //stats->add_overlap_diff (overlap_diff);
  // stats->add_overlap_max (maxScore);
  //}

  //qDebug () << "overlap max: space="<< maxSpace << " score="<< maxScore
  // << " scans_used=" << scan_queue->size();

  qDebug() <<"=== MAO ESTIMATE USING GAUSSIAN ===" << maxSpace << maxScore;
}



// TODO add coverage estimate, as int? suggested space?
void Localizer::emitNewLocationEstimate(QString estimatedSpaceName, double estimatedSpaceScore)
{
  qDebug() << "emitNewLocationEstimate" << estimatedSpaceName
           << "score" << estimatedSpaceScore
	   << "currentEstimateSpace" << currentEstimateSpace;

  currentEstimateScore = estimatedSpaceScore;
  if (currentEstimateSpace != estimatedSpaceName) {
    currentEstimateSpace = estimatedSpaceName;
    m_stats->emittedNewLocation();
    m_stats->emitStatistics();
    emitLocationEstimate();
    emitEstimateToMonitors();
  }
}

bool Localizer::haveValidEstimate() {
  if (currentEstimateSpace == unknownSpace) {
    return false;
  }
  return true;
}

void Localizer::queryCurrentEstimate(QString &country, QString &region,
                                     QString &city, QString &area,
                                     QString &space, QString &tags, 
				     int &floor, double &score)
{
  QStringList list = currentEstimateSpace.split("/");

  if (currentEstimateSpace == unknownSpace || list.size() != 6) {
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
    floor = 0;
  } else {
    country = list.at(0);
    region = list.at(1);
    city = list.at(2);
    area = list.at(3);
    floor = list.at(4).toInt();
    space = list.at(5);
    // tags not implemented yet
    tags = "";
    score = currentEstimateScore;
  }
}

void Localizer::fillAreaCache()
{
  m_areaCacheFillTimer.stop();
  m_areaCacheFillTimer.start(AREA_FILL_PERIOD);

  if (!networkConfigurationManager->isOnline()) {
    qDebug() << "aborting fill_area_cache because offline";
    return;
  }
  if (m_hibernating && haveValidEstimate()) {
    qDebug() << "aborting fill_area_cache because hibernating";
    return;
  }

  QString lMacA, lMacB;
  loudMac(lMacA, lMacB);

  if (!lMacA.isEmpty() || !lMacB.isEmpty()) {

    QNetworkRequest request;
    QString urlStr = mapServerURL;
    QUrl url(urlStr.append("/getAreas"));

    url.addQueryItem("mac", lMacA);
    url.addQueryItem("mac2", lMacB);
    request.setUrl(url);

    qDebug() << "req " << url;

    setNetworkRequestHeaders(request);
    QNetworkReply *reply = networkAccessManager->get(request);
    connect(reply, SIGNAL(finished()), SLOT(macToAreasResponse()));
  }
}

void Localizer::macToAreasResponse()
{
  qDebug() << "mac_to_areas_response";
  QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
  qDebug() << "reply" << reply;
  reply->deleteLater();

  int elapsed = findReplyLatencyMsec(reply);
  m_stats->addNetworkLatency(elapsed);

  if (reply->error() != QNetworkReply::NoError) {
    qWarning() << "mac_to_areas_response request failed "
               << reply->errorString()
               << " url " << reply->url();
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

  int length = reply->readLine(buffer, bufferSize);
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
    length = reply->readLine(buffer, bufferSize);
  }

  if (newAreaFound || m_forceMapCacheUpdate) {
    m_mapCacheFillTimer.stop();
    m_mapCacheFillTimer.start(MAP_FILL_PERIOD_SHORT);
    m_forceMapCacheUpdate = false;
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

  // here we do want to potentially delay the fetch, to give the server time
  // to do its work updating the signal map
  if (networkConfigurationManager->isOnline()) {
    m_mapCacheFillTimer.stop();
    m_mapCacheFillTimer.start(MAP_FILL_PERIOD_SOON);
  }
}

void Localizer::fillMapCache()
{
  qDebug() << "fill_map_cache period " << MAP_FILL_PERIOD;
  m_mapCacheFillTimer.stop();
  m_mapCacheFillTimer.start(MAP_FILL_PERIOD);

  if (!networkConfigurationManager->isOnline()) {
    qDebug() << "aborting fill_map_cache because offline";
    return;
  }
  if (m_hibernating && haveValidEstimate()) {
    qDebug() << "aborting fill_map_cache because hibernating";
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
      QDateTime lastModifiedTime;
      if (!area) {
        qDebug() << "last_modified: area is null";
      } else {
        qDebug() << "last_update " << area->lastModifiedTime()
                 << "touch=" << area->isTouched();
        lastModifiedTime = area->lastModifiedTime();
        area->untouch();
      }


      int version = -1;
      if (area) {
	qDebug () << "setting version from area";
        version = area->mapVersion();
      }

      enqueueAreaMapRequest(i.key(), lastModifiedTime);

    } else if (expireStamp > area->lastAccessTime()) {
      qDebug() << "area expired= " << i.key();
      i.remove();
      delete area;
    }
  }

  issueAreaMapRequest();

}

void Localizer::enqueueAreaMapRequest(QString areaName, QDateTime lastModifiedTime)
{
  qDebug() << "enqueueAreaMapRequest " << areaName << lastModifiedTime;
    QNetworkRequest request;
    QString areaUrl(staticServerURL);
    areaUrl.append("/map/");
    areaUrl.append (areaName);
    areaUrl.append ("/sig.xml");
    areaUrl = areaUrl.trimmed ();

    qDebug() << "enqueueAreaMap" << areaUrl;
    request.setUrl(areaUrl);
    setNetworkRequestHeaders(request);

    if (!lastModifiedTime.isNull() && lastModifiedTime.isValid()) {
      request.setRawHeader("If-Modified-Since", lastModifiedTime.toString().toAscii());
      qDebug() << "if-modified-since" << lastModifiedTime;
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
  qDebug() << "requestAreaMap";

  QNetworkReply *reply = networkAccessManager->get(request);
  qDebug() << "area_map_reply creation " << reply->url().path();
  connect(reply, SIGNAL(finished()), SLOT(handleAreaMapResponseAndReissue()));
}

void Localizer::handleAreaMapResponseAndReissue()
{
  handleAreaMapResponse();
  issueAreaMapRequest();
}

void Localizer::handleAreaMapResponse()
{
  qDebug() << "handleAreaMapResponse";

  /*
  // for debugging
  QMapIterator<QString,AreaDesc*> i (*m_signalMaps);
  while (i.hasNext()) {
    i.next();
    QString area = i.key();
    AreaDesc *desc = i.value();
    if (desc != NULL) {
      qDebug () << "new map loop A area " << desc->lastModifiedTime();      
    } else {
      qDebug () << "new map loop A area " << area << " null";
    }
  }
  */
  QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

  reply->deleteLater();
  QString path = reply->url().path();

  // handle 304 not modified case
  QVariant httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
  if (!httpStatus.isNull()) {
    int status = httpStatus.toInt();
    if (status == 304) {
      qDebug() << "area_map_response: not modified path" << path;
      m_stats->addNetworkSuccessRate(1);
      return;
    }
  }

  path.remove(QRegExp("^.*/map/"));

  // chop off the sig.xml at the end
  path.remove(path.length() - 8, path.length() - 1);

  if (reply->error() != QNetworkReply::NoError) {
    qDebug() << "area_map_response request failed "
	     << reply->errorString()
	     << "errorNo" << reply->error()
	     << " url " << reply->url();

    // AWS returns 202 when it has been deleted
    if (reply->error() == QNetworkReply::ContentNotFoundError ||
	reply->error() == QNetworkReply::ContentOperationNotPermittedError ||
	reply->error() == QNetworkReply::ContentAccessDenied) {
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
  QDateTime lastModified = (reply->header(QNetworkRequest::LastModifiedHeader)).toDateTime();


  // Can be null if we've only inserted the key and not the value
  if (m_signalMaps->contains(path) && m_signalMaps->value(path)) {
    AreaDesc *existingAreaDesc = m_signalMaps->value(path);
    qDebug() << "testing age of new map vs old "
	     << "existing " << existingAreaDesc->lastModifiedTime() 
	     << "new " << lastModified;
    if (existingAreaDesc->lastModifiedTime() == lastModified) {
      qDebug() << "skipping map update because unchanged";
      return;
    }
  }

  // TODO sanity check on the response
  QByteArray mapAsByteArray = reply->readAll();

  saveMap(path, mapAsByteArray);

  if (!parseMap(mapAsByteArray, lastModified))
    qWarning() << "parseMap error " << path;

  /*
  // for debugging
  QMapIterator<QString,AreaDesc*> i (*m_signalMaps);
  while (i.hasNext()) {
    i.next();
    QString area = i.key();
    AreaDesc *desc = i.value();
    if (desc != NULL) {
      qDebug () << "new map loop B area " << desc->lastModifiedTime();      
    } else {
      qDebug () << "new map loop B area " << area << " null";
    }
  }
  */

}


// Pick a "loud" mac at random.
// If no loud macs exist, just pick any one.
// Idea is to make selection of areas non-deterministic.

// The power of two choices...
void Localizer::loudMac(QString &loudMacA, QString &loudMacB)
{
  if (m_fingerprint->isEmpty()) {
    return;
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
    loudMacA = loudMacs[loudMacIndex];
    loudMacIndex = randInt(0, loudMacs.size() - 1);
    loudMacB = loudMacs[loudMacIndex];
  } else {
    int loudMacIndexA = randInt(0, m_fingerprint->size() - 1);
    int loudMacIndexB = randInt(0, m_fingerprint->size() - 1);

    QMapIterator<QString,APDesc*> i (*m_fingerprint);
    int index = 0;
    while (i.hasNext()) {
      i.next();
      if (index == loudMacIndexA)
        loudMacA = i.key();
      if (index == loudMacIndexB)
        loudMacB = i.key();
      ++index;
    }
  }

}

void Localizer::handleMotionChange(Motion currentMotion) { 
    m_stats->setCurrentMotion(currentMotion);
    if (currentMotion == MOVING) {
      m_stats->clearAfterWalkDetection();
      //currentEstimateSpace = unknownSpace; // TODO is this really what we want?
    }
    m_stats->emitStatistics();
}

void Localizer::handleHibernate(bool goToSleep)
{
  qDebug () << "Localizer handleHibernate" << goToSleep;
  m_hibernating = goToSleep;
  m_stats->handleHibernate(goToSleep);
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

// returns true if space existed
bool Localizer::removeSpace(QString fqArea, QString fqSpace)
{
  qDebug() << "in-memory remove" << fqSpace;

  if (!m_signalMaps->contains(fqArea)) {
    qDebug () << "removeSpace did not find area" << fqArea;
    return false;
  }
  AreaDesc *areaDesc = m_signalMaps->value(fqArea);

  QMap<QString,SpaceDesc*> *spaces = areaDesc->spaces();
  if (!spaces->contains(fqSpace)) {
    qDebug () << "removeSpace did not find space" << fqArea;
    return false;
  }

  spaces->remove(fqSpace);
  if (spaces->size() == 0) {
    qDebug () << "no spaces left in this area" << fqArea;
    m_signalMaps->remove(fqArea);
  }
  return true;
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
  int floor;
  queryCurrentEstimate(country, region, city, area, space, tags, floor, score);
  placeMap.insert("country", country);
  placeMap.insert("region", region);
  placeMap.insert("city", city);
  placeMap.insert("area", area);
  placeMap.insert("space", space);
  placeMap.insert("tags", tags);
  placeMap.insert("floor", floor);
  placeMap.insert("score", score);
}

void Localizer::serializeSignature (QVariantMap &map) {
  QVariantMap subsigs;

  QMapIterator<QString,APDesc*> i (*m_fingerprint);
  while (i.hasNext()) {
    i.next();
    Sig *sig = (Sig*) (i.value());
    QVariantMap sigMap;
    sig->serialize (sigMap);
    subsigs[i.key()] = sigMap;
  }

  map["macsigs"] = subsigs;

}

void Localizer::removeMacFromFingerprint(QString mac) {
  if (m_fingerprint->contains(mac)) {
      m_fingerprint->remove(mac);
  }
}

/*
void Localizer::dumpSignalMaps () {
  QMapIterator<QString,AreaDesc*> i (*m_signalMaps);
  while (i.hasNext()) {
    i.next();
    QString area = i.key();
    AreaDesc *desc = i.value();
    
  }
}
*/
