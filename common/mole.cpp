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

#include "mole.h"

#include <csignal>

QString MOLE_ORGANIZATION = "Nokia";
QString MOLE_APPLICATION  = "Mole";
QString MOLE_DOMAIN       = "mole.research.nokia.com";

QString DEFAULT_MAP_SERVER_URL = "http://mole.research.nokia.com:8080";
QString DEFAULT_STATIC_SERVER_URL = "https://s3.amazonaws.com/mole-nokia";

#ifdef Q_WS_MAEMO_5
QString DEFAULT_ROOT_PATH = "/opt/var/lib";
QString MOLE_ICON_PATH    = "/usr/share/mole/icons/";
#else
QString DEFAULT_ROOT_PATH = "/var/cache/mole";
QString MOLE_ICON_PATH    = "/usr/share/pixmaps/mole/";
#endif

bool debug = false;
bool verbose = false;

QDir rootDir;
QString mapServerURL = DEFAULT_MAP_SERVER_URL;
QString staticServerURL = DEFAULT_STATIC_SERVER_URL;
QString rootPathname = DEFAULT_ROOT_PATH;

QFile *logFile;
QTextStream *logStream;
QSettings *settings;
QNetworkAccessManager *networkAccessManager;
QNetworkConfigurationManager *networkConfigurationManager;

struct CleanExit{
  CleanExit()
  {
    signal(SIGINT, &CleanExit::exitQt);
    signal(SIGTERM, &CleanExit::exitQt);
    signal(SIGHUP, &CleanExit::exitQt);
  }

  static void exitQt(int /*sig*/)
  {
    *logStream << QDateTime::currentDateTime().toString() << " I: Exiting.\n\n";
    if (logFile)
      logFile->close();
    QCoreApplication::exit(0);
  }
};

void outputHandler(QtMsgType type, const char *msg)
{
  switch (type) {
  case QtDebugMsg:
    if (debug) {
      *logStream << QDateTime::currentMSecsSinceEpoch() << " D: " << msg << "\n";
      logStream->flush();
    }
    break;
  case QtWarningMsg:
    *logStream << QDateTime::currentMSecsSinceEpoch() << " I: " << msg << "\n";
    logStream->flush();
    break;
  case QtCriticalMsg:
    *logStream << QDateTime::currentMSecsSinceEpoch() << " C: " << msg << "\n";
    logStream->flush();
    break;
  case QtFatalMsg:
    *logStream << QDateTime::currentMSecsSinceEpoch() << " F: " << msg << "\n";
    logStream->flush();
    abort();
  }
}

void initSettings()
{
  QCoreApplication::setOrganizationName(MOLE_ORGANIZATION);
  QCoreApplication::setOrganizationDomain(MOLE_DOMAIN);
  QCoreApplication::setApplicationName(MOLE_APPLICATION);
}

void initCommon(QObject *parent, QString logFilename)
{
  // very basic sanity time check
  QDateTime localtime(QDateTime::currentDateTime());
  QDateTime checktime(QDateTime::fromTime_t((uint)1303313000));

  if (localtime < checktime) {
    fprintf(stderr, "Not starting because local clock is invalid.  Exiting.\n");
    exit(-1);
  }

  rootDir.setPath(rootPathname);
  if (!rootDir.exists()) {
    bool ok = QDir().mkpath(rootPathname);
    if (!ok) {
      fprintf(stderr, "Cannot create root dir %s.  Exiting.\n", rootDir.path().toAscii().data());
      exit(-1);
    } else {
      fprintf(stderr, "Created root dir %s.\n", rootDir.path().toAscii().data());
      rootDir.setPath(rootPathname);
    }
  }

  if (!rootDir.isReadable()) {
    fprintf(stderr, "Cannot read root dir %s.  Exiting.\n", rootDir.path().toAscii().data());
    exit(-1);
  }

  // Initialize logger
  if (!logFilename.isEmpty()) {
    logFile = new QFile(logFilename);
    if (logFile->open(QFile::WriteOnly | QFile::Append)) {
      logStream = new QTextStream(logFile);
    } else {
      fprintf(stderr, "Could not open log file %s.  Exiting.\n", logFilename.toAscii().data());
      exit(-1);
    }
  } else {
    logStream = new QTextStream(stderr);
  }

  // Set up shutdown handler
  CleanExit cleanExit;
  qInstallMsgHandler(outputHandler);

  networkAccessManager = new QNetworkAccessManager(parent);
  networkConfigurationManager = new QNetworkConfigurationManager(parent);

  staticServerURL.trimmed();
  rootPathname.trimmed();
  mapServerURL.trimmed();
  logFilename.trimmed();
}
