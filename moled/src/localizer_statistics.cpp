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

#define ALPHA                   0.20

// periodically put a line in the log file
void LocalizerStats::log_statistics () {
  qWarning () << "stats: "
	      << "net_ok " << network_success_rate
	      << "churn " << emit_new_location_sec
	      << "scan_ms " << scan_rate_ms
	      << "scans " << scan_queue_size
	      << "macs " << macs_seen_size;
}

#ifdef USE_MOLE_DBUS
void LocalizerStats::emit_statistics () {

  int uptime = start_time.secsTo (QDateTime::currentDateTime());
  if (emit_new_location_count > 0) {
    emit_new_location_sec = (double) uptime / (double) emit_new_location_count;
  } else {
    emit_new_location_sec = 0;
  }

  qDebug () << "emit_statistics "
	    << "network_success_rate " << network_success_rate
	    << "emit_new_location_sec " << emit_new_location_sec
	    << "scan_rate_ms " << scan_rate_ms;

  QString empty = "";

  QDBusMessage stats_msg = QDBusMessage::createSignal("/", "com.nokia.moled", "LocationStats");
  stats_msg << empty

    // convert to GMT for network recording
	    << start_time

    // integers
	    << scan_queue_size
	    << macs_seen_size

	    << total_area_count
	    << total_space_count
	    << potential_area_count
	    << potential_space_count

	    << movement_detected_count


    // doubles
	    << scan_rate_ms
	    << emit_new_location_sec

	    << network_latency
	    << network_success_rate

	    << overlap_max
	    << overlap_diff;


  QDBusConnection::sessionBus().send(stats_msg);
}
#else
void LocalizerStats::emit_statistics () { }
#endif

void LocalizerStats::getStatsAsMap (QVariantMap &map) {
  map.insert("LocalizerQueueSize", scan_queue_size);
  map.insert("MacsSeenSize", macs_seen_size);
  map.insert("TotalAreaCount", total_area_count);
  map.insert("PotentialSpaceCount", potential_space_count);
  map.insert("ScanRate", (int)(round(scan_rate_ms/1000)));
  map.insert("NetworkSuccessRate", network_success_rate);
  map.insert("NetworkLatency", network_latency);
  map.insert("OverlapMax", overlap_max);
  map.insert("OverlapDiff", overlap_diff);
  map.insert("Churn", (int)(round(emit_new_location_sec)));
}

LocalizerStats::LocalizerStats (QObject *parent) : QObject(parent) {
  start_time = QDateTime::currentDateTime();

  emit_new_location_count = 0;
  last_emit_location = QTime::currentTime();
  last_scan_time = QTime::currentTime();

  scan_queue_size = 0;
  macs_seen_size = 0;

  network_latency = 0;
  network_success_rate = 0.8;
  ap_per_sig_count = 0;
  ap_per_scan_count = 0;
  total_area_count = 0;
  total_space_count = 0;
  potential_area_count = 0;
  potential_space_count = 0;

  // TODO this is not accurate because it only gets updated when there is a change, not when there isn't
  // should be updates per time
  emit_new_location_sec = 0;
  scan_rate_ms = 0;
  movement_detected_count = 0;

  overlap_diff = 0.;
  overlap_max = 0.;

  log_timer = new QTimer (this);
  connect (log_timer, SIGNAL (timeout()),
	   SLOT (log_statistics ()));
  log_timer->start (30000);
}

void LocalizerStats::add_network_latency(int value){
  network_latency = update_ewma (network_latency, value);
}
void LocalizerStats::add_network_success_rate(int value){ 
  network_success_rate = update_ewma (network_success_rate, value);
  qDebug () << "net success " << network_success_rate;
}

void LocalizerStats::emitted_new_location () {
  emit_new_location_count++;

  // TODO do this in a more sensible way:
  // i.e. what is the *recent* update rate
  //emit_new_location_ms = 
  //update_ewma (emit_new_location_ms, last_emit_location.elapsed());
  //last_emit_location = QTime::currentTime();

  // computed in emit_statistics

}

void LocalizerStats::received_scan () {
  qDebug () << "received_scan "
	    << "rate " << scan_rate_ms
	    << "elapsed " << last_scan_time.elapsed();
  scan_rate_ms = last_scan_time.elapsed();
  // TODO make this the average of the last e.g. ten minutes
  //update_ewma (scan_rate_ms, last_scan_time.elapsed());
  last_scan_time = QTime::currentTime();
}

void LocalizerStats::add_ap_per_sig_count(int count) { 
  ap_per_sig_count = update_ewma (ap_per_sig_count, count);
}

void LocalizerStats::add_ap_per_scan_count(int count){ 
  ap_per_scan_count = update_ewma (ap_per_scan_count, count);
}

void LocalizerStats::add_overlap_diff(double value){ 
  overlap_diff = update_ewma (overlap_diff, value);
}

void LocalizerStats::add_overlap_max(double value){ 
  qDebug () << "overlap "
	    << "max " << overlap_max
	    << "value " << value;
  overlap_max = update_ewma (overlap_max, value);
}


void LocalizerStats::movement_detected () {
  movement_detected_count++;
}

/*
  void LocalizerStats::add_total_area_count(int){ }
  void LocalizerStats::add_total_space_count(int){ }
  void LocalizerStats::add_potential_area_count(int){ }
  void LocalizerStats::add_potential_space_count(int){ }
  void LocalizerStats::add_emit_new_location_sec(int){ }
*/

double LocalizerStats::update_ewma(double current, int value){
  return (ALPHA*(double)value + ((1.0-ALPHA)*current));
}

double LocalizerStats::update_ewma(double current, double value){
  return (ALPHA*(double)value + ((1.0-ALPHA)*current));
}
