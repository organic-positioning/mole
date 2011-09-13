/*
 * Mole - Mobile Organic Localisation Engine
 * Copyright 2010-2011 Nokia Corporation.
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

#include <QtCore>
#include <QCoreApplication>
#include <QTcpSocket>
#include <qjson/parser.h>
#include <qjson/serializer.h>

#include "../../common/ports.h"
#include "../../common/version.h"

#define BUFFER_SIZE 1024

void usage();
void version();

int main(int argc, char *argv[])
{
  //initSettings();
  int port = DEFAULT_LOCAL_PORT;
  QCoreApplication *app = new QCoreApplication(argc, argv);

  // let user's settings override system's via fallback mechanism
  //settings = new QSettings(MOLE_ORGANIZATION, MOLE_APPLICATION, app);

  // TODO check how this handles params with spaces

  QStringList args = QCoreApplication::arguments();
  QStringListIterator argsIter(args);
  argsIter.next(); // = mole


  QVariantMap request;
  QVariantMap params;
  while (argsIter.hasNext()) {
    QString arg = argsIter.next();
    if (arg == "-p") {
      port = argsIter.next().toInt();
    } else if (arg == "-v") {
      version();
    } else if (arg == "query") {
      request["action"] = "query";
    } else if (arg == "bind") {
      request["action"] = "bind";
    } else if (arg == "add") {
      request["action"] = "bind";
    } else if (arg == "fix") {
      request["action"] = "bind";
    } else if (arg == "stats") {
      request["action"] = "stats";
    } else if (arg == "unbind") {
      request["action"] = "remove";
    } else if (arg == "remove") {
      request["action"] = "remove";
    } else if (arg == "validate") {
      request["action"] = "validate";
    } else if (arg.startsWith ("--")) {
      arg.remove ("--");
      if (!argsIter.hasNext()) {
	qWarning () << "Param" << arg << "has no value";
      }
      QString value = argsIter.next();
      params[arg] = value;
    } else {
      qDebug () << "unknown param";
      usage();
    }
  }

  if (!request.contains("action")) {
    qDebug () << "Error: no action provided";
    usage ();
  }

  if (params.size() > 0) {
    request["params"] = params;
  }

  //////////////////////////////////////////////////////////

  QJson::Serializer serializer;
  const QByteArray requestJson = serializer.serialize(request);

  QTcpSocket *socket = new QTcpSocket ();
  app->connect (socket, SIGNAL(disconnected()),
		socket, SLOT(deleteLater()));
  socket->connectToHost ("localhost", port);
  if (socket->waitForConnected ()) {
    socket->write (requestJson);
    if (socket->waitForReadyRead ()) {
      QByteArray replyJson = socket->read (BUFFER_SIZE);
      QJson::Parser parser;
      bool ok;
      QMap<QString, QVariant> reply = parser.parse (replyJson, &ok).toMap();
      if (!ok) {
	qWarning () << "Error: Could not parse reply" << replyJson;
	return -1;
      }
      if (!reply.contains ("status")) {
	qWarning () << "Error: No status found in reply" << replyJson;
	return -1;
      }
      QString status = reply["status"].toString();
      if (status != "OK") {
	// error message in status
	qWarning () << status;
	return -1;
      }
      QMapIterator<QString,QVariant> resIter (reply);
      while (resIter.hasNext()) {
	resIter.next();
	if (resIter.key() != "status") {
	  qWarning () << resIter.key() << "=" << resIter.value().toString();
	}
      }
    } else {
      qWarning () << "Error: No response from daemon";
      return -1;
    }
  } else {
    qWarning () << "Error: Could not connect on port" << port
		<< "Is Mole Daemon running?";
    return -1;
  }

  //////////////////////////////////////////////////////////  

  return 0;
  //app->exec();
}

void usage()
{
  qCritical()
    << "mole [query|bind|stats|remove|validate] [parameters]\n"
    << "query -> print current estimate\n"
    << "bind [fully-qualified place name] or [place name relative to current area]\n"
    << "stats -> print statistics\n"
    << "\n"
    << "Examples:\n\n"
    << "Add to the shared signature database:\n"
    << "mole bind --country \"USA\" --region \"Massachusetts\" --city \"Cambridge\" --area \"77 Massachusetts Ave\" --floor \"1\" --space \"Information Center\"\n"
    << "mole bind --building \"Uninterpreted text string\" --space \"Joe's Office\"\n"
    << "mole bind --space \"Mark's Office\"\n"
    << " (makes a bind relative to the current estimate)\n\n"
    << "Confirm that the current estimate is correct (provides positive reinforcement):\n"
    << "mole validate\n\n"
    << "Remove from the shared signature database:\n"
    << "mole remove --country \"USA\" --region \"Massachusetts\" --city \"Cambridge\" --area \"77 Massachusetts Ave\" --floor \"1\" --space \"Information Center\"\n";

  exit (-1);
}

void version()
{
  qCritical("mole version %s\n", MOLE_VERSION);
  exit(0);
}

