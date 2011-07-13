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

#ifndef BINDER_H_
#define BINDER_H_

#include "moled.h"
#include "scanQueue.h"
#include "localizer.h"

// five minutes
const int EXPIRE_SECS = 300;

class Binder : public QObject {
  Q_OBJECT

public:
  Binder(QObject *parent = 0, Localizer *localizer = 0, ScanQueue *scanQueue = 0);
  ~Binder ();

  void set_location_estimate
    (QString estimated_space_name, double estimated_space_score);
  
  //void set_device_desc (QString _device_desc);
  void set_wifi_desc (QString _wifi_desc);
  //void handle_speed_estimate(int motion);

public slots:
  QString handleBindRequest
    (QString country, QString region, QString city, QString area, 
     QString space_name, QString tags);

  //void handle_quit ();

private slots:
  void handle_bind_response ();

  void xmit_bind ();

private:
  bool in_flight;
  QDir *binds_dir;
  QString bind_dir_name;

  QString wifi_desc;
  QString device_desc;

  QTimer xmit_bind_timer;
  QDateTime oldestValidScan;

  //QNetworkAccessManager *bind_network_manager;
  QNetworkReply *reply;

  //QList<AP_Scan*> *scan_list;

  //QString estimated_space_name;
  //double estimated_space_score;

  QMap<QString,QString> bfn2area;
  Localizer *localizer;
  ScanQueue *scanQueue;

};



#endif /* BINDER_H_ */
