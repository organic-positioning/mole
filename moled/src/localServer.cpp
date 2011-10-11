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


#include "localServer.h"

#include "binder.h"
#include "localizer.h"

#include <QTcpSocket>

#include <qjson/parser.h>
#include <qjson/serializer.h>

const int  BUFFER_SIZE = 1024;

LocalServer::LocalServer(QObject *parent, Localizer *_localizer, Binder *_binder, int port)
  :QTcpServer(parent)
  , m_localizer(_localizer)
  , m_binder(_binder)
{
  bool ok = listen(QHostAddress::LocalHost, port);
  if (!ok)
    qFatal ("LocalServer cannot listen on port");

  connect(this, SIGNAL(newConnection()), this, SLOT(handleRequest()));

  if (!isListening())
    qFatal ("LS: not listening");

  qDebug() << "LocalServer started on" << serverAddress() << ":" << serverPort();
}

LocalServer::~LocalServer()
{
  close();
}

void LocalServer::handleRequest()
{
  if (!hasPendingConnections()) {
    qWarning() << "LS: handleRequest had no pending connection";
    return;
  }

  QTcpSocket *socket = nextPendingConnection();
  connect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));
  // This blocks everything...
  // so can be used for testing (sending request via telnet)
  socket->waitForReadyRead(10000);
  bool monitor = false;
  QByteArray reply;
  if (socket->bytesAvailable() > 0) {
    QByteArray request = socket->read(BUFFER_SIZE);

    bool ok;
    bool isHttp = httpToContent (request, ok);

    if (ok) {
      QVariantMap replyMap = handleRequest(request, monitor);
      QJson::Serializer serializer;
      reply = serializer.serialize(replyMap);
    }
    if (isHttp) {
      contentToHttp (reply, ok);
    }
  }

  if (!monitor) {
    socket->write(reply);
    qDebug() << "handleRequest from port" << socket->peerPort();
    socket->disconnectFromHost();
  } else {
    m_localizer->addMonitor(socket);
  }

}

QVariantMap LocalServer::handleRequest(QByteArray &rawJson, bool &monitor)
{
  QJson::Parser parser;
  bool ok;
  QVariantMap resMap;

  qDebug () << "rawJson " << rawJson;

  QVariantMap request = parser.parse(rawJson, &ok).toMap();
  if (!ok) {
    qWarning() << "LS: failed to parse json" << rawJson;
    resMap["status"] = "Error: could not parse json in request";
    return resMap;
  }
  QString action;
  if (request.contains("action")) {
    action = request["action"].toString();
  } else {
    qWarning() << "LS: no action found in json" << rawJson;
    resMap["status"] = "Error: no action in request";
    return resMap;
  }
  QVariantMap params;
  if (request.contains("params"))
    params = request["params"].toMap();

  // qDebug () << "action " << action;

  if (action == "bind") {
    return handleBind(params, "fix");
  } else if (action == "add") {
    return handleBind(params, "add");
  } else if (action == "remove") {
    return handleBind(params, "remove");
  } else if (action == "validate") {
    return handleBind(params, "validate");
  } else if (action == "stats") {
    return handleStats(params);
  } else if (action == "query") {
    return handleQuery(params);
  } else if (action == "monitor") {
    monitor = true;
  } else {
    qWarning() << "LS: unknown action " << action;
    resMap["status"] = "Error: unknown action in request";
  }
  return resMap;
}

QVariantMap LocalServer::handleBind(QVariantMap &params, QString source)
{
  QString country;
  QString region;
  QString city;
  QString area;
  QString space;
  QString tags;
  QString building;
  int floor = 0;
  QVariantMap resMap;
  if (params.contains("country"))
    country = params["country"].toString();

  if (params.contains("region"))
    region = params["region"].toString();

  if (params.contains("city"))
    city = params["city"].toString();

  if (params.contains("area"))
    area = params["area"].toString();

  if (params.contains("space"))
    space = params["space"].toString();

  if (params.contains("source"))
    source = params["source"].toString();

  if (params.contains("tags"))
    tags = params["tags"].toString();

  if (params.contains("floor"))
    floor = params["floor"].toInt();

  if (params.contains("building")) {
    building = params["building"].toString();
    if (building.size() < 10) {
      resMap["status"] = "Error: building string too short";
      return resMap;
    }
    country = "NOK";
    region = building.at(0);
    region.append(building.at(1));
    region.append(building.at(2));
    city = building.at(3);
    city.append(building.at(4));
    city.append(building.at(5));
    area = building;
  }

  QString res = m_binder->handleBindRequest(source, country, region, city, 
					    area, floor, space, tags);
  resMap["status"] = res;
  return resMap;
}

QVariantMap LocalServer::handleStats(QVariantMap&)
{
  QVariantMap statsMap;
  m_localizer->stats()->statsAsMap(statsMap);
  statsMap["status"] = "OK";
  return statsMap;
}

QVariantMap LocalServer::handleQuery(QVariantMap&)
{
  QVariantMap placeMap;
  m_localizer->estimateAsMap(placeMap);

  if (placeMap["country"] == "NOK") {
    QString building = placeMap["area"].toString();
    QString space = placeMap["space"].toString();
    placeMap.clear ();
    placeMap["building"] = building;
    placeMap["space"] = space;
  }

  placeMap["status"] = "OK";

  return placeMap;
}

// if the request is preceded by http headers,
// discard them
bool LocalServer::httpToContent (QByteArray &request, bool &ok) {
  ok = true;
  QList<QByteArray> reqLines = request.split('\n');
  if (reqLines[0].startsWith("GET")) {
    ok = false;
    return true;
  }
  if (reqLines.size() == 1) {
    return false;
  }
  if (!reqLines[0].startsWith("POST")) {
    ok = false;
    return true;
  }
  request = reqLines[reqLines.size()-1];
  return true;
}

// this request came from an http client,
// so put http response headers at the front
void LocalServer::contentToHttp (QByteArray &reply, bool ok) {
  //const QByteArray header = "HTTP/1.0 200 OK\n\n";
  const QByteArray header = "HTTP/1.0 200 OK\nContent-Type: application/json\nServer: Mole/"+QByteArray(MOLE_VERSION)+"\n";
  const QByteArray notOkHeader = "HTTP/1.0 400 Bad Request\n\n";

  if (ok) {
  qDebug () << "old content" << reply;
  QString contentLength = "Content-length: "+ QString::number(reply.size()) + "\n";
  QByteArray originalReply (reply);
  reply = header;
  reply += contentLength;
  reply += "\n";
  reply += originalReply;
  //reply = header;
  qDebug () << "new content" << reply;
  } else {
    reply = notOkHeader + reply;
  }
}
