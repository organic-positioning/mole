/****************************************************************************
**
** Copyright (C) 2010-2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
**
****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fcntl.h>
#include <csignal>
#include <sys/types.h>
#include <sys/stat.h>

#include <QCoreApplication>
#include <QDebug>

#include <QStringList>
#include <QTextStream>
#include <QDateTime>
#include <QFile>

#include "localServer.h"

#define DEFAULT_LOG_FILE     "/var/log/mole-scanner.log"
#define MOLE_SCANNER_VERSION "0.1"
#define DEFAULT_PORT         5950

void usage();

bool debug = false;
FILE *logStream = NULL;

//////////////////////////////////////////////////////////////////////////////
// Set up logging
void outputHandler(QtMsgType type, const char *msg)
{
  switch(type) {
  case QtDebugMsg:
    if(debug) {
      fprintf(logStream, "%s D: %s\n", QDateTime::currentDateTime().toString().toAscii().data(), msg);
      fflush(logStream);
    }
    break;
  case QtWarningMsg:
    fprintf(logStream, "%s I: %s\n", QDateTime::currentDateTime().toString().toAscii().data(), msg);
    fflush(logStream);
    break;
  case QtCriticalMsg:
    fprintf(logStream, "%s C: %s\n", QDateTime::currentDateTime().toString().toAscii().data(), msg);
    fflush(logStream);
    break;
  case QtFatalMsg:
    fprintf(logStream, "%s F: %s\n", QDateTime::currentDateTime().toString().toAscii().data(), msg);
    fflush(logStream);
    std::exit(-1);
  }
}

void daemonize()
{
    if (::getppid() == 1) 
	return;  // Already a daemon if owned by init

    int i = fork();
    if (i < 0) exit(1); // Fork error
    if (i > 0) exit(0); // Parent exits
    
    ::setsid();  // Create a new process group

    for (i = ::getdtablesize() ; i > 0 ; i-- )
	::close(i);   // Close all file descriptors
    i = ::open("/dev/null", O_RDWR); // Stdin
    ::dup(i);  // stdout
    ::dup(i);  // stderr

    ::close(0);
    ::open("/dev/null", O_RDONLY);  // Stdin

    ::umask(027);

    QString pidfile = "/var/run/mole-scanner.pid";
    int lfp = ::open(qPrintable(pidfile), O_RDWR|O_CREAT, 0640);
    if (lfp<0)
	qFatal("Cannot open pidfile %s\n", qPrintable(pidfile));
    if (lockf(lfp, F_TLOCK, 0)<0)
	qFatal("Can't get a lock on %s - another instance may be running\n", qPrintable(pidfile));
    QByteArray ba = QByteArray::number(::getpid());
    ::write(lfp, ba.constData(), ba.size());

    ::signal(SIGCHLD,SIG_IGN);
    ::signal(SIGTSTP,SIG_IGN);
    ::signal(SIGTTOU,SIG_IGN);
    ::signal(SIGTTIN,SIG_IGN);
}

int main(int argc, char *argv[])
{
  QCoreApplication::setOrganizationName("Nokia");
  QCoreApplication::setOrganizationDomain("nrcc.noklab.com");
  QCoreApplication::setApplicationName("mole-scanner");
  QCoreApplication::setApplicationVersion(MOLE_SCANNER_VERSION);
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
    daemonize();
  } else {
    qDebug() << "not daemonizing";
  }

  //////////////////////////////////////////////////////////
  // Initialize logger
  if (logFilename != NULL) {
    if((logStream = fopen(logFilename, "a")) == NULL) {
      fprintf(stderr, "Could not open log file %s.\n\n", logFilename);
      std::exit(-1);
    }
  } else {
    logStream = stderr;
  }

  qInstallMsgHandler(outputHandler);
  qDebug() << "installed msg handler";

  LocalServer *localServer = new LocalServer(app, port);

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
  qCritical() << "version" << MOLE_SCANNER_VERSION;
  exit(0);
}
/****************************************************************************
**
** Copyright (C) 2010-2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
**
****************************************************************************/

#include "localServer.h"
#include <qjson/serializer.h>

const int  BUFFER_SIZE = 1024;

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


