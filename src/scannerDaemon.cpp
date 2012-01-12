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

#include <qjson/serializer.h>
#include "version.h"
#include "util.h"
#include "scannerDaemon.h"
#include "speedsensor.h"

#ifdef Q_WS_MAEMO_5
#include "scanner-maemo.h"
#else
#include "scanner-nm.h"
#endif

const int  BUFFER_SIZE = 1024;
#define DEFAULT_LOG_FILE     "/var/log/mole-scanner.log"
#define DEFAULT_PORT         5950

void usage();

int main(int argc, char *argv[])
{
  QCoreApplication::setOrganizationName("Nokia");
  QCoreApplication::setOrganizationDomain("nrcc.noklab.com");
  QCoreApplication::setApplicationName("mole-scanner");
  QCoreApplication::setApplicationVersion(MOLE_VERSION);
  qsrand(QDateTime::currentMSecsSinceEpoch()+getpid());

  QCoreApplication *app = new QCoreApplication(argc, argv);

  bool isDaemon = false;
  char defaultLogFilename[] = DEFAULT_LOG_FILE;
  char* logFilename = defaultLogFilename;
  int port = DEFAULT_PORT;

  QStringList args = QCoreApplication::arguments();
  {
    QStringListIterator args_iter(args);
    args_iter.next(); // mole-scanner

    while (args_iter.hasNext()) {
      QString arg = args_iter.next();
      if (arg == "-d") {
	debug = true;
      } else if (arg == "-n") {
	isDaemon = false;
	logFilename = NULL;
      } else if (arg == "-l") {
	logFilename = args_iter.next().toAscii().data();
      } else if (arg == "-p") {
        port = args_iter.next().toInt();
        if (port <= 0) {
          fprintf(stderr, "Port must be positive");
          usage();
        }
      } else {
	usage();
      }
    }
  }

  if (isDaemon) {
    daemonize("mole-scanner");
  } else {
    qDebug() << "not daemonizing";
  }

  initLogger(logFilename);
  qInstallMsgHandler(outputHandler);

  qDebug() << "installed msg handler";

  LocalServer *localServer = new LocalServer(app, port);
  Scanner *scanner = new Scanner(app);
  /*
  connect(m_scanner, SIGNAL(addReading(QString,QString,qint16,qint8)),
	  m_scanQueue, SLOT(addReading(QString,QString,qint16,qint8)));
  connect(m_scanner, SIGNAL(scanCompleted()), m_scanQueue, SLOT(scanCompleted()));
  */
  
  if (SpeedSensor::haveAccelerometer()) {
    SpeedSensor speedSensor = new SpeedSensor(app);
    connect(m_speedSensor, SIGNAL(hibernate(bool)), m_scanner, SLOT(handleHibernate(bool)));
  }

  //////////////////////////////////////////////////////////
  qWarning() << "Starting mole-scanner " 
	     << "debug=" << debug
	      << "isDaemon=" << isDaemon
	      << "port=" << port
	      << "logFilename=" << logFilename;

  //app->connect(app, SIGNAL(aboutToQuit()), syncer, SLOT(handleQuit()));
  return app->exec();

}

void usage() {
  qCritical() << "mole-scanner\n"
	      << "-h print usage\n"
	      << "-d debug\n"
	      << "-n run in foreground (do not daemonize)\n"
	      << "-p port [" << DEFAULT_PORT << "]\n"
	      << "-l log file [" << DEFAULT_LOG_FILE << "]\n";
  qCritical() << "version" << MOLE_VERSION;
  exit(0);
}


LocalServer::LocalServer(QObject *parent, int port)
  :QTcpServer(parent)
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
  QByteArray reply = "error";
  if (socket->bytesAvailable() > 0) {
    QByteArray request = socket->read(BUFFER_SIZE);

    QString requestString (request);
    if (requestString == "scans" ||
	requestString == "/scans") {

      QJson::Serializer serializer;
      reply = serializer.serialize(scans);
    }
  }
  socket->write(reply);
  qDebug () << "wrote reply" << reply;
  qDebug() << "handleRequest from port" << socket->peerPort();
  socket->disconnectFromHost();
}


