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

#ifndef SCANNER_H_
#define SCANNER_H_

#include <QtCore>
#include <QtCore>
#include <QtDBus>
#include <QCoreApplication>
//#include <QSystemNetworkInfo>
//#include <QNetworkConfigurationManager>
#include <QNetworkInterface>

#include "moled.h"
#include "scanQueue.h"
#include "binder.h"

class Scanner : QObject {
  Q_OBJECT

public:
  Scanner(QObject *parent = 0, ScanQueue *scanQueue = 0, Binder *binder = 0);
  ~Scanner ();

 private:
  ScanQueue *scanQueue;
  Binder *binder;
  QTimer timer;
  bool have_set_driver;
  QDBusInterface *wifi;
  void initWiFi ();

private slots:
  void scanAccessPoints ();


};


#endif /* SCANNER_H_ */
