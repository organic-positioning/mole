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

bool MapParser::startDocument() {
  return true;
}

bool MapParser::endDocument() {
  return true;
}

bool MapParser::endElement (const QString&, const QString&, 
			    const QString &/*name*/) {
  return true;
}

bool MapParser::startElement (const QString&, const QString&,
			      const QString &name,
			      const QXmlAttributes &attrs) {

  if (name == "area") {
    area_desc = new AreaDesc();
    QString country, region, city, area;

    for (int i = 0; i < attrs.count(); i++) {
      if (attrs.localName(i) == "country") {
	country = attrs.value (i);
      } else if (attrs.localName(i) == "region") {
	region = attrs.value (i);
      } else if (attrs.localName(i) == "city") {
	city = attrs.value (i);
      } else if (attrs.localName(i) == "area") {
	area = attrs.value (i);
      } else if (attrs.localName(i) == "map_version") {
	area_desc->set_map_version (attrs.value (i).toInt());
      } else if (attrs.localName(i) == "builder_version") {
	builder_version = attrs.value (i).toInt();
      }
    }

    fq_area.append (country);
    fq_area.append ('/');
    fq_area.append (region);
    fq_area.append ('/');
    fq_area.append (city);
    fq_area.append ('/');
    fq_area.append (area);

    qDebug() << "parsing fq_area" << fq_area;


  } else if (name == "spaces") {

    current_space_desc = new SpaceDesc();
    QString space_name = fq_area;
    space_name.append ('/');

    // might add others in the future...
    for (int i = 0; i < attrs.count(); i++) {
      if (attrs.localName(i) == "name") {
	space_name.append (attrs.value(i));
      }
    }
    area_desc->getSpaces()->insert (space_name, current_space_desc);

    qDebug() << "parsing space_name" << space_name;

  } else if (name == "mac") {

    double avg = 0, stddev = 0, weight = 0;
    QString bssid;
    QString histogram;

    for (int i = 0; i < attrs.count(); i++) {
      if (attrs.localName(i) == "name") {
	bssid = attrs.value(i);
      } else if (attrs.localName(i) == "avg") {

	//QString val = attrs.value(i);
	//qDebug () << "val "<< val;
	//qDebug () << "val "<< val.toDouble();

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
      if (stddev <= 0. || stddev > 100.0) {
	qDebug () << bssid 
		  << " avg=" << avg
		  << " stddev=" << stddev;
      }
      if (stddev == 0.) {
	qDebug () << "MapParser::startElement setting stddev";
	stddev = 1.0;
      }

      Q_ASSERT (stddev > 0.);
      Q_ASSERT (stddev < 100.);

      Q_ASSERT (avg > 0.);
      Q_ASSERT (avg < 120.);

      Q_ASSERT (weight > 0.);
      Q_ASSERT (weight < 1.);

      current_space_desc->getSignatures()->insert
	(bssid, new Sig (avg, stddev, weight, histogram));
      //current_space_desc->insertMac (bssid);
      area_desc->insertMac (bssid);
    }

  }

  return true;
}


AreaDesc* MapParser::get_area_desc () {
  return area_desc;
}

bool Localizer::parse_map (const QByteArray &map_as_byte_array, const QDateTime &last_modified) {

  bool ok = false;
  QString map_as_xml(map_as_byte_array);

  // Parse the incoming XML
  // TODO Apply any in-memory user binds
  // Update the signal_map (area_name) -> area_desc

  MapParser parser;
  QXmlInputSource source;
  // not that it matters, but it looks like we do not need to
  // convert to a string first
  //qDebug () << "received xml shows blank mac";
  //qDebug () << "received xml " << map_as_xml;
  source.setData (map_as_xml);
  QXmlSimpleReader reader;
  reader.setContentHandler (&parser);
  reader.parse (source);

  if (!parser.get_fq_area().isEmpty()) {

    QString fq_area = parser.get_fq_area();

    if (parser.get_area_desc() != NULL) {
  
      // out with the old
      if (signal_maps->contains (fq_area)) {
	qDebug() << "dropping old map fq_area=" << fq_area;
	AreaDesc *old_map = signal_maps->value(fq_area);

	delete old_map;
      }

      // in with the new
      AreaDesc *new_map = parser.get_area_desc();
      new_map->set_last_modified_time (last_modified);
      signal_maps->insert (fq_area, new_map);
      qDebug() << "inserted new map fq_area=" << fq_area;

      //dirty = true;
      //emit location_data_changed ();
      localize (0);
      ok = true;

    } else {
      qWarning () << "xml parse: no area_desc fq_name=" << fq_area;
    }

  } else {
    qWarning() << "xml parse: no fq_area";
  }

  return ok;

}

void Localizer::save_map (QString path, const QByteArray &map_as_byte_array) {

  QString dir_name = map_root->absolutePath();
  dir_name.append ("/");
  dir_name.append (path);

  QDir map_dir (dir_name);
  if (!map_dir.exists()) {

    bool ret = map_root->mkpath (path);
    if (!ret) {
      qFatal ("Failed to create map directory");
      QCoreApplication::exit (-1);
    }
    qDebug () << "created map dir " << dir_name;

  }

  QString file_name = map_dir.absolutePath();
  file_name.append ("/sig.xml");
  qDebug () << "file_name" << file_name;
  QFile file (file_name);

  if (!file.open (QIODevice::WriteOnly | QIODevice::Truncate)) {
    qFatal ("Could not open file to store map");
    QCoreApplication::exit (-1);
  }

  QDataStream stream (&file);
  stream << map_as_byte_array;
  file.close ();

}

void Localizer::unlink_map (QString path) {

  QString dir_name = map_root->absolutePath();
  dir_name.append ("/");
  dir_name.append (path);

  QDir map_dir (dir_name);

  bool rm_ok = map_dir.remove ("sig.xml");
  if (!rm_ok) {
    qWarning () << "Failed to remove sig.xml from map_dir= " << map_dir;
    return;
  }

  rm_ok = map_root->rmpath (path);

  if (!rm_ok) {
    qWarning () << "Failed to remove path= " << path;
  }

}

void Localizer::load_maps () {

  QDateTime current_time = QDateTime::currentDateTime();
  QDirIterator it (*map_root, QDirIterator::Subdirectories);
  while (it.hasNext()) {
    it.next();
    qDebug() << "it path" << it.filePath();
    qDebug() << "it name" << it.fileName();

    if (it.fileName() == "sig.xml") {
      QFile file (it.filePath());
      if (!file.open (QIODevice::ReadOnly)) {
	qWarning () << "Could not read map file " << it.fileName();
	return;
      }
      QByteArray map_as_byte_array;
      QDataStream stream (&file);
      stream >> map_as_byte_array;
      if (!parse_map (map_as_byte_array, current_time)) {
	qWarning () << "parse_map error " << it.filePath();
      }
      file.close();

    }
    
  }

}

AreaDesc::AreaDesc () {
  last_access_time = QDateTime::currentDateTime();
  last_modified_time = QDateTime::currentDateTime();
  last_update_time = QDateTime::currentDateTime();
  macs = new QSet<QString>;
  spaces = new QMap<QString,SpaceDesc*> ();
  pTouch = false;
}

AreaDesc::~AreaDesc () {
  macs->clear();
  delete macs;
  qDeleteAll(spaces->begin(), spaces->end());
  spaces->clear();
  delete spaces;
}

QDateTime AreaDesc::get_last_access_time() {
  return last_access_time;
}

QDateTime AreaDesc::get_last_modified_time() {
  return last_modified_time;
}

QDateTime AreaDesc::get_last_update_time() {
  return last_update_time;
}

void AreaDesc::set_last_update_time() {
  last_update_time = QDateTime::currentDateTime();
}

void AreaDesc::set_last_modified_time(QDateTime _last_modified_time) {
  last_modified_time = _last_modified_time;
}


void AreaDesc::accessed() {
  last_access_time = QDateTime::currentDateTime();
}

// Not doing for now, because not updating without replacement
//void AreaDesc::updated() {
//  last_update_time = QDateTime::currentDateTime();
//}
/*
int AreaDesc::get_map_version() {
  return map_version;
}

QMap<QString,SpaceDesc*>* AreaDesc::get_spaces() {
  return spaces;
}

QSet<QString>* AreaDesc::get_macs() {
  return macs;
}
*/

SpaceDesc::SpaceDesc () {
  //macs = new QSet<QString> ();
  sigs = new QMap<QString,Sig*> ();
}

SpaceDesc::SpaceDesc (QMap<QString,Sig*> *fingerprint) {
  //macs = new QSet<QString> ();
  sigs = new QMap<QString,Sig*> ();

  // copy the existing user's sig into this space's
  // constant (non-dynamic) sig
  QMapIterator<QString,Sig*> i (*fingerprint);
  while (i.hasNext()) {
    i.next();
    Sig* sig = new Sig (i.value());
    sigs->insert (i.key(), sig);
  }

}

SpaceDesc::~SpaceDesc () {
  qDeleteAll(sigs->begin(), sigs->end());
  sigs->clear();
  delete sigs;
}
