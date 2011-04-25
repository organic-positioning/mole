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

//#include <QtCore>
//#include <QNetworkReply>
//#include <QNetworkRequest>
//#include <QUrl>
//#include <QNetworkAccessManager>

#include <QXmlDefaultHandler>
#include <QXmlInputSource>
#include <QXmlSimpleReader>

#include "moled.h"
#include "../../common/util.h"
#include "../../common/network.h"
#include "../../common/overlap.h"
#include "scan.h"

//#include "binder.h"
class Binder;

class SpaceDesc {

 public:
  SpaceDesc ();
  ~SpaceDesc ();

  QSet<QString> *get_macs() { return macs; }
  QMap<QString,Sig*>* get_sigs() { return sigs; }
  //void update (QSet<QString> *macs);

 private:
  QSet<QString> *macs;
  QMap<QString,Sig*> *sigs;

};

class AreaDesc {

 public:
  AreaDesc ();
  ~AreaDesc ();

  QMap<QString,SpaceDesc*> *get_spaces ();

  QSet<QString> *get_macs();
  QDateTime get_last_update_time();
  QDateTime get_last_access_time();
  QDateTime get_last_modified_time();
  void set_last_update_time ();
  void set_last_modified_time (QDateTime last);
  void accessed();
  int get_map_version();
  void set_map_version (int version) { map_version = version; }

  void touch () { pTouch = true; }
  bool isTouched () { return pTouch; }
  void untouch () { pTouch = false; }

 private:
  QSet<QString> *macs;
  QMap<QString,SpaceDesc*> *spaces;
  // according to our local clock
  QDateTime last_update_time;
  QDateTime last_access_time;
  // according to the server
  QDateTime last_modified_time;
  int map_version;
  bool pTouch;

};

class MapParser : public QXmlDefaultHandler {
public:
  bool startDocument();
  bool endElement (const QString&, const QString&, const QString &name);

  bool startElement (const QString&, const QString&, const QString &name,
		     const QXmlAttributes &attributes);

  bool endDocument();

  AreaDesc* get_area_desc ();

  //int get_map_version () { return map_version; }

  QString get_fq_area () { return fq_area; }

 private:
  AreaDesc *area_desc;
  SpaceDesc *current_space_desc;
  QString fq_area;
  //int map_version;
  int builder_version;
  //void reset ();

};

class LocalizerStats : public QObject{
  Q_OBJECT

  // send cookie along with this

 public:

  LocalizerStats (QObject *parent);

  void emit_statistics ();
  void emitted_new_location ();
  void received_scan ();

  // TODO battery usage?
  // TODO scan rate
  // TODO rate of change between spaces
  void add_network_latency(int);
  void add_network_success_rate(int);
  void add_ap_per_sig_count(int);
  void add_ap_per_scan_count(int);

  void add_overlap_max(double value);
  void add_overlap_diff(double value);

  void movement_detected ();

  // void add_total_area_count(int);
  //void add_total_space_count(int);
  //void add_potential_area_count(int);
  //void add_potential_space_count(int);
  //void add_emit_new_location_sec(int);

  void set_scan_queue_size (int v) { scan_queue_size = v; }
  void set_macs_seen_size (int v) { macs_seen_size = v; }

  void set_total_area_count (int v) { total_area_count = v; }
  void set_total_space_count (int v) { total_space_count = v; }
  void set_potential_area_count (int v) { potential_area_count = v; }
  void set_potential_space_count (int v) { potential_space_count = v; }

 private:

  QTimer *log_timer;
  int scan_queue_size;
  int macs_seen_size;

  int total_area_count;
  int total_space_count;
  int potential_area_count;
  int potential_space_count;

  double network_latency;
  double network_success_rate;

  double ap_per_sig_count;
  double ap_per_scan_count;

  double scan_rate_ms;
  double emit_new_location_sec;
  int emit_new_location_count;

  double overlap_max;
  double overlap_diff;
  int movement_detected_count;

  QDateTime start_time;
  QTime last_scan_time;
  QTime last_emit_location;

  double update_ewma(double current, int value);
  double update_ewma(double current, double value);

private slots:
  void log_statistics ();

};

class Localizer : public QObject {
  Q_OBJECT

public:
  Localizer(QObject *parent = 0);
  ~Localizer ();
  //Localizer(QObject *parent = 0, Network *network = 0);

  //void add_scan (AP_Scan *scan);

  void add_scan (AP_Scan *scan);
  void touch (QString area_name);
  QString get_location_estimate () { return max_space; }

  //public slots:


  //void handle_quit ();

private:

  bool first_add_scan;
  bool clear_queue;
  bool online;

  bool area_map_reply_in_flight;
  bool mac_reply_in_flight;

  QDir *map_root;

  Overlap *overlap;
  QTime last_localized_time;

  int network_success_level;
  LocalizerStats *stats;
  //QTime emit_new_location_estimate_time;


  double current_estimated_space_score;

  QQueue<AP_Scan *> *scan_queue;

  int area_fill_period;
  int map_fill_period;

  QTimer *area_cache_fill_timer;
  QTimer *map_cache_fill_timer;
  //QTimer *localize_timer;
  // long time window
  QMap<QString,MacDesc*> *active_macs;
  // short time window (maybe just most recent scan)
  //QMap<QString,MacDesc*> *recent_active_macs;

  QString max_space;

  QNetworkReply *area_map_reply;
  QNetworkReply *mac_reply;


  QMap<QString,AreaDesc*> *signal_maps;

  double mac_overlap_coefficient
    (QMap<QString,MacDesc*> *macs_a, QSet<QString> *macs_b);


  void request_area_map (QString area_name, QDateTime last_update_time);

  QString get_loud_mac ();


  void emit_new_location_estimate 
    (QString estimated_space_name, double estimated_space_score);
  void emit_location_estimate ();
  void emit_statistics ();

  void emit_new_local_signature ();

  void clear_active_macs (QMap<QString,MacDesc*> *macs);

  void make_overlap_estimate 
    (QMap<QString,SpaceDesc*> &potential_spaces);

  //void check_network_state ();

signals:
  void location_data_changed ();

private slots:
  void localize ();

  void fill_area_cache();
  void fill_map_cache();

  //void mac_to_areas_response(QNetworkReply*);
  void mac_to_areas_response();
  //void area_map_response(QNetworkReply*);
  void area_map_response();
  bool parse_map (const QByteArray &map_as_byte_array,
		  const QDateTime &last_modified);
  void save_map (QString path, const QByteArray &map_as_byte_array);
  void unlink_map (QString path);
  void load_maps();

  QString handle_signature_request();
  void handle_location_estimate_request();
  void handle_speed_estimate(int motion);

  //signals:
  //void parsed_area (QString fq_area);
  //void added_scan ();
  //void new_location_estimate (QString fq_space_name, double score);

};

extern QString current_estimated_space_name;
extern QString unknown_space_name;

#endif /* LOCALIZER_H_ */
