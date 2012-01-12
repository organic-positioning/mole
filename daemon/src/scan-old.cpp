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

#include "scan.h"

/*
const double avg_stddev = 1.0;

int id_counter = 0;
int scan_count = 0;

AP_Reading::AP_Reading (QString _bssid, QString _ssid, quint32 _frequency, quint32 _level) :
  bssid (_bssid.toLower()), ssid (_ssid), frequency (_frequency), level (_level) {

  qDebug () << "bssid" << bssid << "ssid" << ssid << "freq" << frequency << "level" << level;
  
}


AP_Reading::AP_Reading (AP_Reading *reading) :
  bssid (reading->bssid), ssid (reading->ssid), frequency (reading->frequency), level (reading->level) {

}

AP_Scan::AP_Scan (AP_Scan *scan) : readings (new QList<AP_Reading*> ()),
				   stamp (new QDateTime (*scan->stamp)) {
  //stamp = new QDateTime (*scan->stamp);
  //readings = new QList<AP_Reading*> ();
  id = id_counter++;
  scan_count++;
  qDebug () << "SCAN copy create " << id  << " out " << scan_count;
  for (int i = 0; i < scan->readings->size(); i++) {
    AP_Reading *reading = new AP_Reading (scan->readings->at(i));
    readings->append (reading);
  }
  readings_map = NULL;
}

AP_Scan::AP_Scan (QList<AP_Reading*> *_readings, QDateTime *_stamp) :
  readings (_readings), stamp (_stamp) {
  id = id_counter++;
  scan_count++;
  qDebug () << "SCAN readings create " << id  << " out " << scan_count;
  readings_map = NULL;
}

// used by binder to test for uniqueness
bool AP_Scan::is_same_scan (AP_Scan *scan) {
  if (readings_map == NULL) {
    readings_map = new QMap<QString,quint32> ();
    for (int i = 0; i < readings->size(); i++) {
      readings_map->insert (readings->at(i)->bssid,readings->at(i)->level);
      //qDebug () << "map " << readings->at(i)->bssid 
      //<< " " << readings->at(i)->level;
    }
  }

  for (int i = 0; i < scan->readings->size(); i++) {
    if (!readings_map->contains (scan->readings->at(i)->bssid)) {
      //qDebug () << "!same " << scan->readings->at(i)->bssid;
      return false;
    }
    if (readings_map->value(scan->readings->at(i)->bssid) != 
	scan->readings->at(i)->level) {
      //qDebug () << "!same " << scan->readings->at(i)->bssid
      //<< " " << readings_map->value(scan->readings->at(i)->bssid)
      //<< " " << scan->readings->at(i)->level;
      return false;
    }
  }
  return true;
}

*/

QDebug operator<<(QDebug dbg, const Reading &reading) {
  dbg.nospace() <<  reading.bssid << " " << reading.ssid << " " << reading.frequency << " " << reading.level;

  return dbg.space();
}

QDebug operator<<(QDebug dbg, const Scan &scan) {
  dbg.nospace() << "c=" << scan.readings->size();
  for (int i = 0; i < scan.readings->size(); i++) {
    AP_Reading *reading = new AP_Reading (scan.readings->at(i));
    dbg.nospace() << " [" << *reading << "]";
    //dbg.nospace() << "[" << "r" << "] ";
  }
  return dbg.space();
}

/*
AP_Scan::~AP_Scan () {
  qDebug () << "SCAN deleting scan " << id << " out " << scan_count;
  scan_count--;
  for (int i = 0; i < readings->size(); i++) {
    delete readings->at(i);
  }
  delete readings;
  delete stamp;
  if (readings_map != NULL) {
    delete readings_map;
  }
}

MacDesc::MacDesc () {
  clear ();
}

void MacDesc::clear () {
    max = 100;

}



int MacDesc::get_count() {
  return rssi_list.size();
}

void MacDesc::set_weight(double _weight) {
  Q_ASSERT (_weight <= 1.0);
  Q_ASSERT (_weight >= 0.0);
  weight = _weight;
}


void MacDesc::recalculate_stats () {

  if (rssi_list.size() == 0) {
    clear ();
    return;
  }

  if (rssi_list.size() == 1) {
    max = rssi_list[0];
    mean = rssi_list[0];
    stddev = avg_stddev;
    return;
  }

  // first compute mean
  mean = 0.;
  for (int i = 0; i < rssi_list.size(); i++) {
    mean += rssi_list[i];
  }
  mean = mean / rssi_list.size();
    
  // next compute stddev
  double sq_sum = 0.;
  for (int i = 0; i < rssi_list.size(); i++) {
    double deviation = rssi_list[i] - mean;
    sq_sum += deviation * deviation;
  }
  double var = sq_sum / (rssi_list.size()-1);
  stddev = sqrt (var);

  // spread out or narrow down the stddev for macs with few readings
  double factor = (double) rssi_list.size();
  stddev = ( (factor - 1) * stddev + avg_stddev ) / factor;

  // debugging
  /*
  QString values;
  values.append ("[ ");
  for (int i = 0; i < rssi_list.size(); i++) {  
    values.append (QString::number(rssi_list[i]));
    values.append (" ");
  }
  values.append ("]");

  */

//   qDebug () << "mean= " << mean << " stddev= " << stddev
// 	    << " count= " << rssi_list.size();

  Q_ASSERT (stddev > 0);

  //<< values;


}

void MacDesc::add_reading (int rssi) {
  rssi_list.append (rssi);
  histogram()->add(rssi);
  recalculate_stats ();
}

void MacDesc::drop_reading (int rssi) {
  QMutableListIterator<int> i (rssi_list);
  bool removed = false;
  while (i.hasNext() && !removed) {
    int val = i.next();
    if (val == rssi) {
      i.remove();
      removed = true;
    }
  }
  if (!removed) {
    qFatal ("drop reading bug");
  }
  recalculate_stats ();
}

// This scan is being added to the localizer.
// put max rssi and count as value for each mac (=key)
void AP_Scan::add_active_macs (QMap<QString,MacDesc*> *active_macs) {

  QListIterator<AP_Reading *> i (*readings);
  while (i.hasNext()) {
    AP_Reading *reading = i.next();
    if (!active_macs->contains (reading->bssid)) {
      active_macs->insert (reading->bssid, new MacDesc());
    }
    active_macs->value(reading->bssid)->add_reading (reading->level);
  }

  recalculate_weights (active_macs);

}

void AP_Scan::recalculate_weights (QMap<QString,MacDesc*> *active_macs) {

  QMapIterator<QString,MacDesc*> i (*active_macs);
  int ap_scan_count = 0;
  while (i.hasNext()) {
    i.next();
    ap_scan_count += i.value()->get_count();
  }

  if (ap_scan_count == 0) {
    qWarning () << "No scans found in scan";
    return;
  }

  i.toFront();
  while (i.hasNext()) {
    i.next();
    double weight = i.value()->get_count() / (double) ap_scan_count;
    i.value()->set_weight (weight);
  }
  

}

// This scan is being removed from the localizer.
void AP_Scan::remove_inactive_macs (QMap<QString,MacDesc*> *active_macs) {

  QListIterator<AP_Reading *> i (*readings);
  while (i.hasNext()) {
    AP_Reading *reading = i.next();
    if (active_macs->contains (reading->bssid)) {
      MacDesc *mac_desc = active_macs->value (reading->bssid);
      mac_desc->drop_reading (reading->level);

      // we don't see this mac anymore
      if (mac_desc->get_count() == 0) {
	active_macs->remove(reading->bssid);
	delete mac_desc;
      }
    } else {

      qWarning() << "active_macs did not contain " << reading->bssid;

    }

  }
  recalculate_weights (active_macs);

}

int AP_Scan::size () {
  return readings->size();
}

