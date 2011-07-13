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

QString MOLE_ORGANIZATION = "Nokia";
QString MOLE_APPLICATION  = "Mole";
QString MOLE_DOMAIN       = "mole.research.nokia.com";
int     MOLE_VERSION      = 40;
QString MOLE_ICON_PATH    = "/usr/share/mole/icons/";

bool debug = false;
bool verbose = false;

QDir rootDir;
QString mapServerURL = DEFAULT_MAP_SERVER_URL;
QString staticServerURL = DEFAULT_STATIC_SERVER_URL;
QString rootPathname = DEFAULT_ROOT_PATH;

QFile *logFile = NULL;
QTextStream *logStream = NULL;
QSettings *settings;
QNetworkAccessManager *networkAccessManager;

struct CleanExit{
  CleanExit() {
    signal(SIGINT, &CleanExit::exitQt);
    signal(SIGTERM, &CleanExit::exitQt);
    signal(SIGHUP, &CleanExit::exitQt); 
  }

  static void exitQt(int /*sig*/) {
    *logStream << QDateTime::currentDateTime().toString() << " I: Exiting.\n\n";
    if (logFile != NULL) {
      logFile->close ();
    }
    QCoreApplication::exit(0);
  }
};


void output_handler(QtMsgType type, const char *msg)
{
  switch (type) {
  case QtDebugMsg:
    if (debug) {
      //*logStream << QDateTime::currentDateTime().toString() << " D: " << msg << "\n";
      *logStream << QDateTime::currentMSecsSinceEpoch() << " D: " << msg << "\n";
      logStream->flush();
    }
    break;
  case QtWarningMsg:
    //QDateTime::currentDateTime().toString()
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

void initSettings () {
  QCoreApplication::setOrganizationName(MOLE_ORGANIZATION);
  QCoreApplication::setOrganizationDomain(MOLE_DOMAIN);
  QCoreApplication::setApplicationName(MOLE_APPLICATION);
}

//////////////////////////////////////////////////////////
void initCommon (QObject *parent, QString logFilename) {

  // very basic sanity time check
  QDateTime localtime (QDateTime::currentDateTime());
  QDateTime checktime (QDateTime::fromTime_t((uint)1303313000));
  //checktime.setTime_t ();
  //checktime.setTime_t ();
  if (localtime < checktime) {
    fprintf (stderr, "Not starting because local clock is invalid\n\n");
    exit (-1);
  }

  rootDir.setPath (rootPathname);
  if (!rootDir.isReadable()) {
    fprintf (stderr, "Cannot read root dir %s\n\n", rootDir.path().toAscii().data());
    exit (-1);
  }

  // Initialize logger
  if (logFilename != NULL && !logFilename.isEmpty()) {
    logFile = new QFile (logFilename);
    if (logFile->open (QFile::WriteOnly | QFile::Append)) {
      logStream = new QTextStream (logFile);
    } else {
      fprintf (stderr, "Could not open log file %s.\n\n", 
	       logFilename.toAscii().data());
      exit (-1);
    }
  } else {
    logStream = new QTextStream (stderr);
  }

  // Set up shutdown handler
  CleanExit cleanExit;
  qInstallMsgHandler (output_handler);

  networkAccessManager = new QNetworkAccessManager (parent);

  staticServerURL.trimmed();
  rootPathname.trimmed();
  mapServerURL.trimmed();
  logFilename.trimmed();
}
