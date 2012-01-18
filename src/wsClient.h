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

#ifndef WSCLIENT_H_
#define WSCLIENT_H_

#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkConfigurationManager>

#include "simpleScanQueue.h"
#ifdef Q_WS_MAEMO_5
#include "scanner-maemo.h"
#else
#include "scanner-nm.h"
#endif

class WSClient : public QObject {
  Q_OBJECT

public:
  WSClient(QString request, QString container, QString poi, QString serverUrl, int port);
  virtual void start();


protected:
  QString m_request;
  QString m_container;
  QString m_poi;
  QString m_serverUrl;
  int m_localScannerPort;

  QVariantList m_scans;
  QVariantMap m_source;
  QVariantMap m_requestMap;
  QNetworkAccessManager *m_networkAccessManager;
  void sendRequest();
  //QByteArray getDataFromDaemon(QString request);
  QVariant getDataFromDaemon(QString request);

public slots:
  void handleResponse();

};

class WSClientSelfScanner : public WSClient
{
  Q_OBJECT

public:
  WSClientSelfScanner(QString request, QString container, QString poi, QString serverUrl, int targetScanCount);
  virtual void start();
  //void sendRequest();

public slots:
  void scanCompleted();

private:
  int m_targetScanCount;
  int m_scansCompleted;
  Scanner *m_scanner;
  SimpleScanQueue *m_scanQueue;

};


#endif /* WSCLIENT_H_ */
