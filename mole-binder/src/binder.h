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

#ifndef BINDER_H
#define BINDER_H

#include <QtGui>
#include <QtDBus>
#include <QWidget>

#include "common.h"
#include "places.h"
#include "settings.h"

enum SubmitState { DISPLAY, CORRECT};

class Binder : public QWidget
{
    Q_OBJECT
public:
    explicit Binder(QWidget *parent = 0);
    ~Binder ();

signals:

public slots:
  void handle_quit();

private:
  bool online;
  SubmitState submit_state;
  QTimer *bind_timer;
  QTimer *request_location_timer;
  QTimer *walking_timer;

  void build_ui ();
  void set_places_enabled (bool isEnabled);
  void reset_bind_timer();
  void set_walking_label (bool);

  QToolButton *about_button;
  QToolButton *feedback_button;
  QToolButton *help_button;
  QToolButton *settings_button;
  QPushButton *submit_button;

  QNetworkReply *feedback_reply;

  Settings *settings_dialog;

  PlaceEdit *country_edit;
  PlaceEdit *region_edit;
  PlaceEdit *city_edit;
  PlaceEdit *area_edit;
  PlaceEdit *space_name_edit;
  QLineEdit *tags_edit;

  QString country_estimate;
  QString region_estimate;
  QString city_estimate;
  QString area_estimate;
  QString space_name_estimate;
  QString tags_estimate;

  void refresh_last_estimate ();
  void send_bind_msg ();
  void set_submit_state (SubmitState _submit_state);

  QLabel *walking_label;
  ColoredLabel *net_label;
  ColoredLabel *daemon_label;
  QLabel *scan_count_label;
  //QLabel *cache_size_label;
  QLabel *cache_spaces_label;
  QLabel *scan_rate_label;
  QLabel *overlap_max_label;
  QLabel *churn_label;

  int request_location_estimate_counter;
  void handle_daemon_change (bool online);

private slots:
  void handle_clicked_about();
  void handle_clicked_feedback();
  void handle_clicked_help();
  void handle_clicked_settings();

  void handle_send_feedback_finished ();
  void handle_place_changed ();

  void handle_clicked_submit ();
  void handle_bind_timer();
  void handle_location_estimate(QString fq_name, bool online);
  void request_location_estimate();

  void handle_walking_timer();
  void handle_speed_estimate(int moving_count);
  void handle_online_change (bool online);

  /*
  void handle_location_stats
    (QString fq_name, QDateTime, int,int,int,int,int,int,int,
     double,double,double,double,double,double);
  */

  void handle_location_stats
    (QString fq_name, QDateTime start_time,
     int,int,int,int,int,int,int,
     double,double,double,double,double,double);


};

#endif // BINDER_H
