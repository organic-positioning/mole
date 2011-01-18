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

#ifndef SCAN_H_
#define SCAN_H_

#include <QtCore>

#include "../common/sig.h"

//Q_DECLARE_METATYPE(Sig)

class MacDesc : public Sig {
 public:
  MacDesc ();
  
  void add_reading (int rssi);
  void drop_reading (int rssi);
  int get_count();

 public:
  int max;
  //double mean_rssi;
  //double stddev_rssi;
  //double weight;

  void size();
  void set_weight(double _weight);

 private:
  void recalculate_stats ();
  void clear ();
  QList<int> rssi_list;

};

class AP_Reading {

 public:

  AP_Reading (QString _bssid, QString _ssid, quint32 _frequency, quint32 _level);
  AP_Reading (AP_Reading *reading);
  const QString bssid;
  const QString ssid;
  const quint32 frequency;
  const quint32 level;

};

class AP_Scan {
 public:

  AP_Scan (QList<AP_Reading*> *_readings, QDateTime *_stamp);
  AP_Scan (AP_Scan *scan);
  ~AP_Scan ();

  bool is_same_scan (AP_Scan *scan);

  void add_active_macs (QMap<QString,MacDesc*> *active_macs);
  void remove_inactive_macs (QMap<QString,MacDesc*> *active_macs);

  QList<AP_Reading*> *readings;
  QDateTime *stamp;

  QMap<QString,quint32> *readings_map;

  int size();

 private:
  int id;
  void recalculate_weights (QMap<QString,MacDesc*> *active_macs);

};

QDebug operator<<(QDebug dbg, const AP_Reading &reading);
QDebug operator<<(QDebug dbg, const AP_Scan &scan);

#endif
