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

#ifndef SCANNER_MAEMO_H
#define SCANNER_MAEMO_H

#include <QDBusConnection>
#include <QObject>
#include <QTime>
#include <QTimer>

class Binder;
class ICDScan;
class ScanQueue;

class Scanner : public QObject
{
  Q_OBJECT
  Q_ENUMS(Mode)

 public:
  enum Mode { Active, ActiveSaved, Passive };

  Scanner(QObject *parent = 0, ScanQueue *scanQueue = 0,
          Binder *binder = 0, Mode scanMode = Scanner::Passive);
  ~Scanner();

  bool stop();
  void hibernate(bool goToSleep);

 signals:
  void scannedAccessPoint(ICDScan *network);

 private:
  QDBusConnection m_bus;
  ScanQueue *m_scanQueue;

  Mode m_mode;
  bool m_scanning;

  //QTime m_interarrival;
  QTimer m_startTimer;

 public slots:
  bool start();

  void scanResultHandler(const QDBusMessage&);
  void print(ICDScan *network);
  void handleHibernate(bool goToSleep);

 private slots:
  void addReadings(ICDScan *network);

};

#endif // SCANNER_H
