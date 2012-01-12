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

#include <stdlib.h>
#include <QtCore>
#include <QCoreApplication>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QNetworkConfigurationManager>

#include <qjson/parser.h>
#include <qjson/serializer.h>
#include "wsClient.h"
#include "ports.h"
#include "version.h"

QString DEFAULT_SERVER_URL = "http://mole.research.nokia.com:8090";
#define BUFFER_SIZE 4096

void usage();
void version();

int main(int argc, char *argv[]) {
  WSClient *app = new WSClient(argc, argv);
  app->start();
  return app->exec();
}

WSClient::WSClient(int argc, char *argv[])
  : QCoreApplication(argc, argv),
    m_targetScanCount(1),
    m_scansCompleted(0),
    m_serverUrl (DEFAULT_SERVER_URL),
    m_request(),
    m_scanner(0),
    m_scanQueue(0),
    m_networkAccessManager(0) {

  //connect(this, SIGNAL(aboutToQuit()), SLOT(handleQuit()));
  QStringList args = QCoreApplication::arguments();
  QStringListIterator argsIter(args);
  argsIter.next(); // = mole

  while (argsIter.hasNext()) {
    QString arg = argsIter.next();
    if (arg == "-v") {
      version();
    } else if (arg == "-s") {
      m_serverUrl = argsIter.next();
    } else if (arg == "--container" || arg == "-c") {
      m_container = argsIter.next();
    } else if (arg == "--poi" || arg == "-p") {
      m_poi = argsIter.next();
    } else if (arg == "-t") {
      m_targetScanCount = argsIter.next().toInt();
    } else if (arg == "--query" || arg == "-q") {
      m_request = "query";
    } else if (arg == "--bind" || arg == "-b") {
      m_request = "bind";
    } else if (arg == "--remove" || arg == "-r") {
      m_request = "remove";
    } else {
      qDebug() << "unknown parameter";
      usage();
    }
  }

  if (m_request.isEmpty()) {
    qWarning() << "Error: no request given";
    usage();
  }

  if ( (m_request == "bind" || m_request == "remove") &&
       (m_container.isEmpty() || m_poi.isEmpty()) ) {
    qWarning() << "Error: container and POI required for 'bind' and 'remove'";
    usage();
  }

  if (m_request == "bind" || m_request == "query") {
    m_scanQueue = new SimpleScanQueue;
    m_scanner = new Scanner;
    connect(m_scanner, SIGNAL(addReading(QString,QString,qint16,qint8)),
	    m_scanQueue, SLOT(addReading(QString,QString,qint16,qint8)));
    connect(m_scanner, SIGNAL(scanCompleted()),
	    m_scanQueue, SLOT(scanCompleted()));
    connect(m_scanQueue, SIGNAL(scanQueueCompleted()),
	    this, SLOT(scanCompleted()));
  }
}

//////////////////////////////////////////////////////////
 
void WSClient::start() {
  if (m_request == "bind" || m_request == "query") {
    // kick off a few local scans, and send them with the request
    // for remote processing
    m_scanner->start();
    qWarning() << "Waiting for" << m_targetScanCount<<"scan(s)";
  }
  m_networkAccessManager = new QNetworkAccessManager;

  if (m_request == "remove") {
    sendRequest();
  }
}

//////////////////////////////////////////////////////////

void WSClient::scanCompleted() {
  ++m_scansCompleted;
  if (m_scansCompleted >= m_targetScanCount) {
    m_scanner->stop();
    qWarning() << "Finished scanning";
    m_scanQueue->serialize(m_requestMap);
    sendRequest();
  }
}

//////////////////////////////////////////////////////////

void WSClient::sendRequest() {
  QJson::Serializer serializer;

  // TODO placeholders that describe the client
  QVariantMap source;
  source["key"] = "key1";
  source["secret"] = "secret1";
  source["device"] = "device1";
  source["version"] = MOLE_VERSION;
  m_requestMap["source"] = source;

  if (!m_container.isEmpty() && !m_poi.isEmpty()) {
    QVariantMap location;
    location["container"] = m_container;
    location["poi"] = m_poi;
    m_requestMap["location"] = location;
  }

  const QByteArray requestJson = serializer.serialize(m_requestMap);
  qDebug () << "json" << requestJson;


  QString urlStr = m_serverUrl;
  urlStr.append("/");
  urlStr.append(m_request);
  QUrl url(urlStr);
  QNetworkRequest request;
  request.setUrl(url);

  QNetworkReply *reply = m_networkAccessManager->post(request, requestJson);
  connect(reply, SIGNAL(finished()), SLOT (handleResponse()));
  qDebug() << "sent request to" << urlStr;
}

void WSClient::handleResponse() {
  QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

  if (reply->error() != QNetworkReply::NoError) {
    qWarning() << "request failed "
               << reply->errorString()
               << "url " << reply->url();
  } else {
    qDebug() << "request succeeded";
    QByteArray responseRawJson = reply->readAll();
    QJson::Parser parser;
    bool ok;
    QMap<QString, QVariant> response = parser.parse (responseRawJson, &ok).toMap();
    if (!ok) {
      qWarning () << "Error: Could not parse response " << responseRawJson;
    } else if (!response.contains ("status")) {
      qWarning () << "Error: No status found in response " << responseRawJson;
    } else {
      QString status = response["status"].toString();
      if (status != "OK") {
	// error message in status
	qWarning () << status;
      } else {
	if (response.contains ("queryResult")) {
	  QVariant resultList = response["queryResult"];
	  qWarning () << "queryResult=" << resultList;
	  /*
	    QListIterator<QVariant> resIter (resultList);
	    while (resIter.hasNext()) {
	    LocationProbability locProb = (LocationProbability) resIter.next();
	    qWarning () << locProb;
	    }
	  */
	}
      }
    }
  }
  reply->deleteLater();
  QCoreApplication::quit();
}

//////////////////////////////////////////////////////////  

void usage()
{
  qCritical()
    << "molews\n"
    << "--query     (-q)\n"
    << "--bind      (-d)\n"
    << "--remove    (-r)\n"
    << "--poi       (-p) string [place of interest within container]\n"
    << "--container (-c) string [name of container] \n"
    << "-s remote server url (default=" << DEFAULT_SERVER_URL << ")\n"
    << "-t scan this many times before sending to remote server (default 1)\n"
    << "Bind and Remove require container and poi\n";
  exit (-1);
}

void version()
{
  qCritical("mole version %s\n", MOLE_VERSION);
  exit(0);
}

