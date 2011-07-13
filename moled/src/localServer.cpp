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

#define BUFFER_SIZE 1024

LocalServer::LocalServer(QObject *parent, Localizer *_localizer, Binder *_binder, int port):
  QTcpServer (parent), localizer (_localizer), binder (_binder)
{

  //bool ok = listen (QHostAddress::Any, port);
  //bool ok = listen ();
  bool ok = listen (QHostAddress::LocalHost, port);
  if (!ok) {
    qFatal ("LocalServer cannot listen on port");
  }

  connect (this, SIGNAL (newConnection()), this, SLOT(handleRequest()));

  if (!isListening()) {
    qFatal ("LS: not listening");
  }

  qDebug () << "LocalServer started on" << serverAddress() << ":" << serverPort();

}

LocalServer::~LocalServer () {
  close ();
}

void LocalServer::handleRequest () {


  if (!hasPendingConnections()) {
    qWarning () << "LS: handleRequest had no pending connection";
    return;
  }
  QTcpSocket *socket = nextPendingConnection();
  connect (socket, SIGNAL(disconnected()),
	   socket, SLOT(deleteLater()));
  // This blocks everything...
  // so can be used for testing (sending request via telnet)
  socket->waitForReadyRead (10000);
  bool monitor = false;
  QByteArray reply;
  if (socket->bytesAvailable() > 0) {
    QByteArray request = socket->read (BUFFER_SIZE);

    reply = handleRequest (request, monitor);
  }

  if (!monitor) {
  socket->write (reply);
  qDebug() << "handleRequest from port" << socket->peerPort();
  socket->disconnectFromHost ();
  } else {
    localizer->addMonitor (socket);
  }

}

QByteArray LocalServer::handleRequest (QByteArray &rawJson, bool &monitor) {

  QJson::Parser parser;
  bool ok;

  QVariantMap request = parser.parse (rawJson, &ok).toMap();
  if (!ok) {
    qWarning () << "LS: failed to parse json" << rawJson;
    return QString("Error: invalid json").toAscii();
  }
  QString action;
  if (request.contains("action")) {
    action = request["action"].toString();
  } else {
    qWarning () << "LS: no action found in json" << rawJson;
    return QString("Error: no action in request").toAscii();
  }
  QVariantMap params;
  if (request.contains("params")) {
    params = request["params"].toMap();
  }
  if (action == "bind") {
    return handleBind (params);
  } else if (action == "stats") {
    return handleStats (params);
  } else if (action == "query") {
    return handleQuery (params);
  } else if (action == "monitor") {
    monitor = true;
    return NULL;
  } else {
    qWarning () << "LS: unknown action " << action;
    return QString("Error: unknown action in request").toAscii();
  }
}

QByteArray LocalServer::handleBind (QVariantMap &params) {
  QString country, region, city, area, space, tags;
  QString building;
  if (params.contains("country")) {
    country = params["country"].toString();
  }
  if (params.contains("region")) {
    region = params["region"].toString();
  }
  if (params.contains("city")) {
    city = params["city"].toString();
  }
  if (params.contains("area")) {
    area = params["area"].toString();
  }
  if (params.contains("space")) {
    space = params["space"].toString();
  }
  if (params.contains("tags")) {
    tags = params["tags"].toString();
  }

  if (params.contains("building")) {
    building = params["building"].toString();
    if (building.size() < 10) {
      return QString("Error: building string too short").toAscii();
    }
    country = "NOK";
    region = building.at(0);
    region.append (building.at(1));
    region.append (building.at(2));
    city = building.at(3);
    city.append (building.at(4));
    city.append (building.at(5));
    area = building;
  }

  QString res = binder->handleBindRequest (country, region, city, area, space, tags);
  return res.toAscii();
}

QByteArray LocalServer::handleStats (QVariantMap &/*params*/) {
  QVariantMap statsMap;
  localizer->getStats()->getStatsAsMap (statsMap);
  QJson::Serializer serializer;
  const QByteArray json = serializer.serialize(statsMap);
  return json;
}

QByteArray LocalServer::handleQuery (QVariantMap &/*params*/) {
  QVariantMap placeMap;
  localizer->getEstimateAsMap (placeMap);

  if (placeMap["country"] == "NOK") {
    QString building = placeMap["area"].toString();
    QString space = placeMap["space"].toString();
    placeMap.clear ();
    placeMap["building"] = building;
    placeMap["space"] = space;
  }

  QJson::Serializer serializer;
  const QByteArray json = serializer.serialize(placeMap);
  return json;
}


