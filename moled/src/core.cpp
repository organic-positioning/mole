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


QString pidfile = "/var/run/mole.pid";

void daemonize();
void usage ();

Core::Core (int argc, char *argv[]) : QCoreApplication (argc, argv) {

  initSettings ();

  QString logFilename = DEFAULT_LOG_FILE;
  //QString configFilename = DEFAULT_CONFIG_FILE;
  
  int port = DEFAULT_LOCAL_PORT;
  bool isDaemon = true;
  bool runWiFiScanner = false;
  bool runMovementDetector = false;
  bool runMotionLogger = false;

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
	  arg == "-A" ||
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
	//} else if (arg == "-c") {
	//configFilename = args_iter.next();
	//configFilename.trimmed();
      }
    }
  }


  //settings = new QSettings (configFilename,QSettings::NativeFormat);
  settings = new QSettings (QSettings::SystemScope, MOLE_ORGANIZATION,
			    MOLE_APPLICATION, this);
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
      } else if (arg == "-A") {
	runMotionLogger = true;
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

  initCommon (this, logFilename);

  //////////////////////////////////////////////////////////
  qWarning () << "Starting mole daemon " 
	      << "debug=" << debug
	      << "port=" << port
	      << "wifi_scanning=" << runWiFiScanner
	      << "movement_detector=" << runMovementDetector
	      << "motion_logger=" << runMotionLogger
    //<< "config=" << configFilename
	      << "logFilename=" << logFilename
	      << "map_server_url=" << mapServerURL
	      << "fingerprint_server_url=" << staticServerURL
	      << "rootPath=" << rootPathname;


  if (isDaemon) {
    daemonize ();
  } else {
    qDebug () << "not daemonizing";
  }



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

  localizer = new Localizer (this);
  binder = new Binder (this, localizer);

  speedsensor = NULL;
  if (runMovementDetector) {
    speedsensor = new SpeedSensor (this);
  }
  motionLogger = NULL;
  if (runMotionLogger) {
    motionLogger = new MotionLogger (this);
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
  if (motionLogger)
    delete motionLogger;
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
    //<< "-c config file [" << DEFAULT_CONFIG_FILE << "]"
    //<< " (overridden by cmd line parameters)\n"
	       << "-s map server URL [" << DEFAULT_MAP_SERVER_URL << "] \n"
	       << "-f fingerprint (static) server [" << DEFAULT_STATIC_SERVER_URL << "]\n"
	       << "-l log file [" << DEFAULT_LOG_FILE << "]\n"
	       << "-r root path [" << DEFAULT_ROOT_PATH << "]"
	       << " (app data stored here)\n"
	       << "-p local port [" << DEFAULT_LOCAL_PORT << "]\n"
	       << "-a run movement detection ourselves (requires accelerometer)\n"
	       << "-A trace all accelerometer data\n"
	       << "-w run wifi scanner ourselves\n";

  exit (0);

}
