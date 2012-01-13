/*
 * Mole - Mobile Organic Localisation Engine
 * Copyright 2010-2012 Nokia Corporation.
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

#ifndef SCANNER_DAEMON_H
#define SCANNER_DAEMON_H

#include <QtCore>
#include <QTcpServer>
#include <QTcpSocket>
#include <QVariant>
#include "simpleScanQueue.h"

class LocalServer : public QTcpServer
{
  Q_OBJECT

 public:
  LocalServer(QObject *parent = 0, SimpleScanQueue *scanQueue = NULL, int port = 0);
  ~LocalServer ();

 private:
  SimpleScanQueue *m_scanQueue;

 private slots:
  void handleRequest();

};

#endif // SCANNER_DAEMON_H
