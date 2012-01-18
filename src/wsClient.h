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

// This and LocationProbability are just for pretty-printing 
// of the json'ified results from the server
class Location : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString container READ container WRITE setContainer)
  Q_PROPERTY(QString poi READ poi WRITE setPoi);

public:
 Location(QObject* parent = 0) : QObject(parent) {}
  ~Location() {}
 Location(const Location &l) : QObject(l.parent()) {
    m_container = l.container();
    m_poi = l.poi();
  }
  Location& operator= (const Location &rhs) {
    m_container = rhs.container();
    m_poi = rhs.poi();
    return *this;
  }

  QString container() const { return m_container; }
  QString poi() const { return m_poi; }

  void setContainer(const QString container) { m_container = container; }
  void setPoi(const QString poi) { m_poi = poi; }
  void set(const QVariantMap map) {
    m_container = map["container"].toString();
    m_poi = map["poi"].toString();
  }

 private:
  QString m_container;
  QString m_poi;
};

class LocationProbability : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantMap location WRITE setLocation) // not using READ...
  Q_PROPERTY(double probability READ probability WRITE setProbability)

friend QDebug operator<<(QDebug dbg, const LocationProbability &lP);

public:
  LocationProbability(QObject* parent = 0) : QObject(parent) {}
  ~LocationProbability() {}

  double probability() const { return m_probability; }
  Location location() const { return m_location; }

  void setProbability(const double probability) { m_probability = probability; }
  void setLocation(const QVariantMap map) { m_location.set(map); }

private:
  Location m_location;
  double m_probability;

};



//////////////////////////////////////////////////////////  
// Web Services Client
// The base class assumes that it can talk to a local daemon
// for any required scans and local identifying info

// The derived class does the scanning itself, 
// and does not talk to a local daemon.

// The purpose of the base class is for long-running localization,
// i.e. if we are going to be scanning and positioning continuously.

// The self-scanner is for one-off, quick, tell-me-where-I-am
// type of situations.

// Note that the base class (non-scanner) does not depend on 
// simpleScanQueue or any of the other scanning/accelerometer code.

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
