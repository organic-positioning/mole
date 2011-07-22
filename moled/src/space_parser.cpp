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

bool MapParser::startElement(const QString&, const QString&,
                             const QString &name,
                             const QXmlAttributes &attrs)
{
  if (name == "area") {
    m_areaDesc = new AreaDesc();
    QString country;
    QString region;
    QString city;
    QString area;

    for (int i = 0; i < attrs.count(); ++i) {
      if (attrs.localName(i) == "country") {
        country = attrs.value(i);
      } else if (attrs.localName(i) == "region") {
        region = attrs.value(i);
      } else if (attrs.localName(i) == "city") {
        city = attrs.value(i);
      } else if (attrs.localName(i) == "area") {
        area = attrs.value(i);
      } else if (attrs.localName(i) == "map_version") {
        m_areaDesc->setMapVersion(attrs.value(i).toInt());
      } else if (attrs.localName(i) == "builder_version") {
        m_builderVersion = attrs.value(i).toInt();
      }
    }

    m_fqArea.append(country);
    m_fqArea.append('/');
    m_fqArea.append(region);
    m_fqArea.append('/');
    m_fqArea.append(city);
    m_fqArea.append('/');
    m_fqArea.append(area);

    qDebug() << "parsing area" << m_fqArea;


  } else if (name == "spaces") {
    m_currentSpaceDesc = new SpaceDesc();
    QString spaceName = m_fqArea;
    spaceName.append ('/');

    // might add others in the future...
    for (int i = 0; i < attrs.count(); ++i) {
      if (attrs.localName(i) == "name")
        spaceName.append(attrs.value(i));
    }
    m_areaDesc->spaces()->insert(spaceName, m_currentSpaceDesc);

    qDebug() << "parsing space" << spaceName;

  } else if (name == "mac") {
    double avg = 0.;
    double stddev = 0.;
    double weight = 0.;
    QString bssid;
    QString histogram;

    for (int i = 0; i < attrs.count(); ++i) {
      if (attrs.localName(i) == "name") {
        bssid = attrs.value(i);
      } else if (attrs.localName(i) == "avg") {
        avg = attrs.value(i).toDouble();
      } else if (attrs.localName(i) == "stddev") {
        stddev = attrs.value(i).toDouble();
      } else if (attrs.localName(i) == "weight") {
        weight = attrs.value(i).toDouble();
      } else if (attrs.localName(i) == "histogram") {
        histogram = attrs.value(i);
      }
    }

    // TODO workaround for weird case where bssid is empty in xml...
    if (!bssid.isEmpty()) {
      // TODO sanity check that all values are set...
      if (stddev <= 0. || stddev > 100.0)
        qDebug() << bssid << " avg=" << avg << " stddev=" << stddev;

      if (stddev == 0.) {
        qDebug() << "MapParser::startElement setting stddev";
        stddev = 1.0;
      }

      Q_ASSERT (stddev > 0.);
      Q_ASSERT (stddev < 100.);

      Q_ASSERT (avg > 0.);
      Q_ASSERT (avg < 120.);

      Q_ASSERT (weight > 0.);
      Q_ASSERT (weight < 1.);

      m_currentSpaceDesc->signatures()->insert(bssid, new Sig(avg, stddev, weight, histogram));
      m_areaDesc->insertMac(bssid);
    }
  }

  return true;
}

bool Localizer::parseMap(const QByteArray &mapAsByteArray, const QDateTime &lastModified)
{
  bool ok = false;
  QString mapAsXml(mapAsByteArray);

  // Parse the incoming XML
  // TODO Apply any in-memory user binds
  // Update the m_signalMap (areaName) -> areaDesc
  MapParser parser;
  QXmlInputSource source;
  // not that it matters, but it looks like we do not need to
  // convert to a string first
  source.setData(mapAsXml);
  QXmlSimpleReader reader;
  reader.setContentHandler(&parser);
  reader.parse(source);

  if (!parser.fqArea().isEmpty()) {
    QString fqArea = parser.fqArea();
    if (parser.areaDesc()) {
      // out with the old
      if (m_signalMaps->contains(fqArea)) {
        qDebug() << "dropping old map fq_area=" << fqArea;
        AreaDesc *oldMap = m_signalMaps->value(fqArea);
        delete oldMap;
      }

      // in with the new
      AreaDesc *newMap = parser.areaDesc();
      newMap->setLastModifiedTime(lastModified);
      m_signalMaps->insert(fqArea, newMap);
      qDebug() << "inserted new map fq_area=" << fqArea;

      localize(0);
      ok = true;

    } else {
      qWarning() << "xml parse: no area_desc fq_name=" << fqArea;
    }
  } else {
    qWarning() << "xml parse: no fq_area";
  }

  return ok;

}

void Localizer::saveMap(QString path, const QByteArray &mapAsByteArray)
{
  QString dirName = m_mapRoot->absolutePath();
  dirName.append("/");
  dirName.append(path);

  QDir mapDir(dirName);
  if (!mapDir.exists()) {
    bool ret = m_mapRoot->mkpath(path);
    if (!ret) {
      qFatal("Failed to create map directory");
      QCoreApplication::exit(-1);
    }
    qDebug() << "created map dir " << dirName;

  }

  QString fileName = mapDir.absolutePath();
  fileName.append("/sig.xml");
  qDebug() << "file name" << fileName;
  QFile file(fileName);

  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    qFatal("Could not open file to store map");
    QCoreApplication::exit (-1);
  }

  QDataStream stream(&file);
  stream << mapAsByteArray;
  file.close();

}

void Localizer::unlinkMap(QString path)
{
  QString dirName = m_mapRoot->absolutePath();
  dirName.append("/");
  dirName.append(path);

  QDir mapDir(dirName);

  bool rmOk = mapDir.remove("sig.xml");
  if (!rmOk) {
    qWarning() << "Failed to remove sig.xml from " << mapDir;
    return;
  }

  rmOk = m_mapRoot->rmpath(path);

  if (!rmOk) {
    qWarning() << "Failed to remove path= " << path;
  }

}

void Localizer::loadMaps()
{
  QDateTime currentTime = QDateTime::currentDateTime();
  QDirIterator it (*m_mapRoot, QDirIterator::Subdirectories);
  while (it.hasNext()) {
    it.next();
    qDebug() << "it path" << it.filePath();
    qDebug() << "it name" << it.fileName();

    if (it.fileName() == "sig.xml") {
      QFile file (it.filePath());
      if (!file.open (QIODevice::ReadOnly)) {
        qWarning() << "Could not read map file " << it.fileName();
        return;
      }
      QByteArray mapAsByteArray;
      QDataStream stream (&file);
      stream >> mapAsByteArray;
      if (!parseMap(mapAsByteArray, currentTime))
        qWarning() << "parse_map error " << it.filePath();

      file.close();
    }
  }
}

AreaDesc::AreaDesc()
  : m_macs(new QSet<QString>())
  , m_spaces(new QMap<QString,SpaceDesc*>())
  , m_touch(false)
{
  m_lastAccessTime = QDateTime::currentDateTime();
  m_lastModifiedTime = QDateTime::currentDateTime();
  m_lastUpdateTime = QDateTime::currentDateTime();
}

AreaDesc::~AreaDesc()
{
  m_macs->clear();
  delete m_macs;
  qDeleteAll(m_spaces->begin(), m_spaces->end());
  m_spaces->clear();
  delete m_spaces;
}

SpaceDesc::SpaceDesc()
  : m_sigs(new QMap<QString,Sig*> ())
{
}

SpaceDesc::SpaceDesc(QMap<QString,Sig*> *fingerprint)
{
  m_sigs = new QMap<QString,Sig*> ();

  // copy the existing user's sig into this space's
  // constant (non-dynamic) sig
  QMapIterator<QString,Sig*> i (*fingerprint);
  while (i.hasNext()) {
    i.next();
    Sig* sig = new Sig(i.value());
    m_sigs->insert(i.key(), sig);
  }

}

SpaceDesc::~SpaceDesc()
{
  qDeleteAll(m_sigs->begin(), m_sigs->end());
  m_sigs->clear();
  delete m_sigs;
}
