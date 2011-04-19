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

#include <QSystemNetworkInfo>
#include <QAbstractSocket>
#include <iostream>
#include <fcntl.h>
#include "core.h"

bool debug = false;
bool verbose = false;

QDir rootDir;
QString mapServerURL = DEFAULT_MAP_SERVER_URL;
QString staticServerURL = DEFAULT_STATIC_SERVER_URL;
QString rootPathname = DEFAULT_ROOT_PATH;
QString pidfile = "/var/run/mole.pid";
QSettings *settings;
QNetworkAccessManager *networkAccessManager;

QFile *logFile = NULL;
QTextStream *logStream = NULL;

void daemonize();
void usage ();

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
      *logStream << QDateTime::currentDateTime().toString() << " D: " << msg << "\n";
      logStream->flush();
    }
    break;
  case QtWarningMsg:
    *logStream << QDateTime::currentDateTime().toString() << " I: " << msg << "\n";
    logStream->flush();
    break;
  case QtCriticalMsg:
    *logStream << QDateTime::currentDateTime().toString() << " C: " << msg << "\n";
    logStream->flush();
    break;
  case QtFatalMsg:
    *logStream << QDateTime::currentDateTime().toString() << " F: " << msg << "\n";
    logStream->flush();
    abort();
  }
}

Core::Core (int argc, char *argv[]) : QCoreApplication (argc, argv) {

  QString logFilename = DEFAULT_LOG_FILE;
  QString configFilename = DEFAULT_CONFIG_FILE;
  
  int port = DEFAULT_LOCAL_PORT;
  bool isDaemon = true;
  bool runWiFiScanner = false;
  bool runMovementDetector = false;

  //////////////////////////////////////////////////////////
  // Make sure no other arguments have been given
  //qDebug () << "arguments pass 1";
  QStringList args = QCoreApplication::arguments();
  {
    QStringListIterator args_iter (args);
    args_iter.next(); // moled

    while (args_iter.hasNext()) {
      QString arg = args_iter.next();
      if (arg == "-d" ||
	  arg == "-n" ||
	  arg == "-a" ||
	  arg == "-w") {
	// nop
      } else if (arg == "-c" ||
	  arg == "-s" ||
	  arg == "-f" ||
	  arg == "-l" ||
	  arg == "-r" ||
	  arg == "-p") {
	// check that these guys have another arg and skip it
	if (!args_iter.hasNext()) {
	  usage ();
	}
	args_iter.next();
      } else {
	usage ();
      }
    }
  }


  // Seed rng
  QTime time = QTime::currentTime();
  qsrand ((uint)time.msec());

  //////////////////////////////////////////////////////////
  // first pass on arguments
  //QStringList args = QCoreApplication::arguments();
  args = QCoreApplication::arguments();
  {
    QStringListIterator args_iter (args);
    args_iter.next(); // moled

    while (args_iter.hasNext()) {
      QString arg = args_iter.next();
      if (arg == "-d") {
	debug = true;
      } else if (arg == "-n") {
	isDaemon = false;
	logFilename = "";
      } else if (arg == "-l") {
	logFilename = args_iter.next();
	logFilename.trimmed();
      } else if (arg == "-c") {
	configFilename = args_iter.next();
	configFilename.trimmed();
      }
    }
  }


  settings = new QSettings (configFilename,QSettings::NativeFormat);

  //////////////////////////////////////////////////////////
  // Read default settings
  if (settings->contains("wifi_scanning")) {
    runWiFiScanner = settings->value("wifi_scanning").toBool();
  }
  if (settings->contains("movement_detector")) {
    runMovementDetector = settings->value("movement_detector").toBool();
  }
  if (settings->contains("map_server_url")) {
    mapServerURL = settings->value ("map_server_url").toString();
  }
  if (settings->contains("fingerprint_server_url")) {
    staticServerURL = settings->value ("fingerprint_server_url").toString();
  }
  if (settings->contains("port")) {
    port = settings->value ("port").toInt();
  }
  if (settings->contains("root_path")) {
    rootPathname = settings->value ("root_path").toString();
  }

  //////////////////////////////////////////////////////////
  // Allow user to override these on command line
  //qDebug () << "arguments pass 2";
  args = QCoreApplication::arguments();
  {
    QStringListIterator args_iter (args);
    //args_iter (args);

    args_iter.next(); // moled

    while (args_iter.hasNext()) {
      QString arg = args_iter.next();
      if (arg == "-s") {
	mapServerURL = args_iter.next();
	mapServerURL.trimmed();
      } else if (arg == "-f") {
	staticServerURL = args_iter.next();
	staticServerURL.trimmed();
      } else if (arg == "-a") {
	runMovementDetector = true;
      } else if (arg == "-w") {
	runWiFiScanner = true;
      } else if (arg == "-r") {
	rootPathname = args_iter.next();
	rootPathname.trimmed();
      }
    }
  }

  //////////////////////////////////////////////////////////
  // check a few things before daemonizing
  rootDir.setPath (rootPathname);
  if (!rootDir.isReadable()) {
    qCritical () << "Cannot read root dir " << rootDir.path();
    usage ();
  }

  //////////////////////////////////////////////////////////
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

  //////////////////////////////////////////////////////////
  qWarning () << "Starting mole daemon " 
	      << "debug=" << debug
	      << "port=" << port
	      << "wifi_scanning=" << runWiFiScanner
	      << "movement_detector=" << runMovementDetector
	      << "config=" << configFilename
	      << "logFilename=" << logFilename
	      << "map_server_url=" << mapServerURL
	      << "fingerprint_server_url=" << staticServerURL
	      << "rootPath=" << rootPathname;


  if (isDaemon) {
    daemonize ();
  } else {
    qDebug () << "not daemonizing";
  }

  networkAccessManager = new QNetworkAccessManager (this);

 // start create map directory
  if (!rootDir.exists ("map")) {
    bool ret = rootDir.mkdir ("map");
    if (!ret) {
      qFatal ("Failed to create map directory. Exiting...");
      exit (-1);
    }
    qDebug () << "created map dir";
  }
  // end create map directory

  // reset session cookie on MOLEd restart
  reset_session_cookie ();

  binder = new Binder (this);
  localizer = new Localizer (this, binder);

  speedsensor = NULL;
  if (runMovementDetector) {
    speedsensor = new SpeedSensor (localizer);
  }

  scanner = NULL;
  if (runWiFiScanner) {
    scanner = new Scanner (this, localizer, binder);
  }

  connect (this, SIGNAL(aboutToQuit()), SLOT(handle_quit()));

}

void Core::handle_quit () {
  qWarning () << "Stopping mole daemon...";

  if (scanner)
    delete scanner;
  if (speedsensor)
    delete speedsensor;
  delete localizer;
  delete binder;

  qWarning () << "Stopped mole daemon";
}

Core::~Core () {
  
}

int Core::run () {
  return exec ();
}

int main(int argc, char *argv[])
{
  printf ("Mole Daemon Starting\n");

  Core *app = new Core (argc, argv);
  return app->run();
}

void daemonize()
{
    if (::getppid() == 1) 
	return;  // Already a daemon if owned by init

    int i = fork();
    if (i < 0) exit(1); // Fork error
    if (i > 0) exit(0); // Parent exits
    
    ::setsid();  // Create a new process group

    ::close(0);
    ::open("/dev/null", O_RDONLY);  // Stdin


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

//using namespace std;

void usage () {
  // d n a w
  // c s f l r p

  qCritical () << "moled\n"
	       << "-h print usage\n"
	       << "-d debug\n"
	       << "-n run in foreground\n"
	       << "-c config file [" << DEFAULT_CONFIG_FILE << "]"
	       << " (overridden by cmd line parameters)\n"
	       << "-s map server URL [" << DEFAULT_MAP_SERVER_URL << "] \n"
	       << "-f fingerprint (static) server [" << DEFAULT_STATIC_SERVER_URL << "]\n"
	       << "-l log file [" << DEFAULT_LOG_FILE << "]\n"
	       << "-r root path [" << DEFAULT_ROOT_PATH << "]\n"
	       << "-p local port [" << DEFAULT_LOCAL_PORT << "]\n"
	       << " (app data stored here)\n"
	       << "-a run movement detection ourselves (requires accelerometer)\n"
	       << "-w run wifi scanner ourselves\n";

  exit (0);

}
