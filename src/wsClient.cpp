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
#include <QTcpSocket>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QNetworkConfigurationManager>

#include <qjson/parser.h>
#include <qjson/serializer.h>
#include <qjson/qobjecthelper.h>
#include <qjson/qjson_export.h>
#include "wsClient.h"
#include "ports.h"
#include "version.h"
#include "util.h"
#include "source.h"

QString DEFAULT_SERVER_URL = "http://mole.research.nokia.com:8090";
#define BUFFER_SIZE 4096
#define APPLICATION_NAME     "mole-ws"

void usage();
void version();

int main(int argc, char *argv[]) {
  QCoreApplication::setOrganizationName("Nokia");
  QCoreApplication::setOrganizationDomain("nrcc.noklab.com");
  QCoreApplication::setApplicationName(APPLICATION_NAME);
  QCoreApplication::setApplicationVersion(MOLE_VERSION);
  qsrand(QDateTime::currentMSecsSinceEpoch()+getpid());

  QString serverUrl = DEFAULT_SERVER_URL;
  QString request;
  QString container;
  QString poi;
  int targetScanCount = 0;
  int port = DEFAULT_SCANNER_DAEMON_PORT;

  QCoreApplication *app = new QCoreApplication(argc,argv);
  QStringList args = QCoreApplication::arguments();
  QStringListIterator argsIter(args);
  argsIter.next(); // = mole-ws

  while (argsIter.hasNext()) {
    QString arg = argsIter.next();
    if (arg == "-v") {
      version();
    } else if (arg == "-s") {
      serverUrl = argsIter.next();
    } else if (arg == "--container" || arg == "-c") {
      container = argsIter.next();
    } else if (arg == "--poi" || arg == "-p") {
      poi = argsIter.next();
    } else if (arg == "-t") {
      targetScanCount = argsIter.next().toInt();
    } else if (arg == "-l") {
      port = argsIter.next().toInt();
    } else if (arg == "--query" || arg == "-q") {
      request = "query";
    } else if (arg == "--bind" || arg == "-b") {
      request = "bind";
    } else if (arg == "--remove" || arg == "-r") {
      request = "remove";
    } else {
      qDebug() << "unknown parameter";
      usage();
    }
  }

  if (request.isEmpty()) {
    qWarning() << "Error: no request given";
    usage();
  }

  if ( (request == "bind" || request == "remove") &&
       (container.isEmpty() || poi.isEmpty()) ) {
    qWarning() << "Error: container and POI required for 'bind' and 'remove'";
    usage();
  }

  WSClient *client = NULL;
  if (targetScanCount == 0) {
    client = new WSClient(request, container, poi, serverUrl, port);
  } else {
    client = new WSClientSelfScanner(request, container, poi, serverUrl, targetScanCount);
  }
  
  client->start();
  return app->exec();
}

//////////////////////////////////////////////////////////

WSClient::WSClient(QString request, QString container, QString poi, QString serverUrl, int localScannerPort) :
  m_request(request),
  m_container(container),
  m_poi(poi),
  m_serverUrl (serverUrl),
  m_localScannerPort(localScannerPort),
  m_networkAccessManager(0) {
  m_networkAccessManager = new QNetworkAccessManager;
}

//////////////////////////////////////////////////////////
// If we need scans for this type of request,
// contact the local scannerDaemon for them.
// If we are doing a remove, we still need our uuid.
void WSClient::start() {

  // we always send this along, whether or not we are sending scans
  if (m_source.isEmpty()) {
    m_source = getDataFromDaemon("/source").toMap();

  }

  if (m_request == "bind" || m_request == "query") {
    m_scans = getDataFromDaemon("/scans").toList();
    m_requestMap["scans"] = m_scans;
  }
  sendRequest();
}

//////////////////////////////////////////////////////////
// Fetch either scans or our uuid from the daemon
//QByteArray WSClient::getDataFromDaemon(QString request) {
QVariant WSClient::getDataFromDaemon(QString request) {
    QTcpSocket socket;
    socket.connectToHost(DEFAULT_LOCAL_HOST, m_localScannerPort);

    const int timeout = 5*1000;
    if (!socket.waitForConnected(timeout)) {
      qFatal("Error connecting to local daemon: %s\n", qPrintable(socket.errorString()));
      exit(-1);
    }

    socket.write(request.toAscii());

    while (socket.bytesAvailable() < (int)sizeof(quint16)) {
      if (!socket.waitForReadyRead(timeout)) {
	qFatal("Error reading from local daemon: %s\n", qPrintable(socket.errorString()));
	exit(-1);
      }
    }

    QByteArray json = socket.readAll();
    qDebug() << "received" << json;
    socket.close();

    QJson::Parser parser;
    bool ok;
    QVariant response = parser.parse (json, &ok);
    if (!ok) {
      qFatal("Could not parse response from scanner daemon");
    }
    return response;
}

//////////////////////////////////////////////////////////
// If we are scanning ourself, then build our own scanner and scanQueue
// wait for the desired number of scans, then proceed to the request.
WSClientSelfScanner::WSClientSelfScanner(QString request, QString container, QString poi, QString serverUrl, int targetScanCount) :
  WSClient(request, container, poi, serverUrl, 0),
  m_targetScanCount(targetScanCount),
  m_scansCompleted(0),
  m_scanner(0),
  m_scanQueue(0) {

  // in addition to scanning (if necessary)
  // include the description of ourselves (uuid, device info, mole version)
  serializeSource(m_source);

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
 
void WSClientSelfScanner::start() {
  if (m_request == "bind" || m_request == "query") {
    // kick off a few local scans, and send them with the request
    // for remote processing
    m_scanner->start();
    qWarning() << "Waiting for" << m_targetScanCount << "scan(s)";
  }

  if (m_request == "remove") {
    sendRequest();
  }
}

//////////////////////////////////////////////////////////
// We are sent this signal by the scanQueue
void WSClientSelfScanner::scanCompleted() {
  ++m_scansCompleted;
  if (m_scansCompleted >= m_targetScanCount) {
    m_scanner->stop();
    qWarning() << "Finished scanning";
    QVariantList list;
    m_scanQueue->serialize(list);
    m_requestMap["scans"] = list;
    sendRequest();
  }
}

//////////////////////////////////////////////////////////
// Through self-scanning or talking to our local daemon,
// we now have whatever scans are needed.
// Send the request to the web service and wait for the response.
void WSClient::sendRequest() {
  QJson::Serializer serializer;
  m_requestMap["source"] = m_source;

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

//////////////////////////////////////////////////////////

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
	  qDebug () << "queryResult=" << resultList;
	  int resultCount = 0;
	  QListIterator<QVariant> resIter (resultList.toList());
	    while (resIter.hasNext()) {
	      resultCount++;
	      QVariant variant = resIter.next();
	      LocationProbability locationProbability;
	      QJson::QObjectHelper::qvariant2qobject(variant.toMap(), 
						     &locationProbability);
	      qWarning () << locationProbability;
	    }
	    qWarning () << "Finished" << resultCount << "results";
	}
      }
    }
  }
  reply->deleteLater();
  QCoreApplication::quit();
}

//////////////////////////////////////////////////////////  
QDebug operator<<(QDebug dbg, const LocationProbability &lP) {
  dbg.nospace()
    << "["
    << "c=" << lP.location().container()
    << ",p="<< lP.location().poi()
    << ",pr="<< lP.probability();
  dbg.nospace() << "]";
  return dbg.space();
}

//////////////////////////////////////////////////////////  

void usage()
{
  qCritical()
    << "mole-ws\n"
    << "--query     (-q)\n"
    << "--bind      (-d)\n"
    << "--remove    (-r)\n"
    << "--poi       (-p) string [place of interest within container]\n"
    << "--container (-c) string [name of container] \n"
    << "-s remote server url (default=" << DEFAULT_SERVER_URL << ")\n"
    << "-t scan this many times before sending to remote server\n"
    << "   Otherwise contact scanner daemon for scans\n"
    << "-l Contact scanner daemon on this port (default=" << DEFAULT_SCANNER_DAEMON_PORT << ")\n"
    << "Bind and Remove require container and poi\n";
  exit (-1);
}

void version()
{
  qCritical("mole version %s\n", MOLE_VERSION);
  exit(0);
}

