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

#include "core.h"

#include "binder.h"
#include "localizer.h"
#include "localServer.h"
#include "scanQueue.h"
#include "speedsensor.h"

#ifdef Q_WS_MAEMO_5
#include "scanner-maemo.h"
#else
#include "scanner-nm.h"
#endif

#include <csignal>
#include <fcntl.h>
#include <iostream>

const QString pidfile = "/var/run/mole.pid";

void daemonize();
void usage();

Core::Core(int argc, char *argv[])
  : QCoreApplication(argc, argv)
{
  initSettings();

  QString logFilename = DEFAULT_LOG_FILE;

  int port = DEFAULT_LOCAL_PORT;
  bool isDaemon = true;
  bool runWiFiScanner = true;
  bool runMovementDetector = true;

  //////////////////////////////////////////////////////////
  // Make sure no other arguments have been given
  QStringList args = QCoreApplication::arguments();
  QStringListIterator argsIter(args);
  argsIter.next(); // moled

  while (argsIter.hasNext()) {
    QString arg = argsIter.next();
    if (arg == "-h") {
      usage();
    } else if (arg == "-d") {
        debug = true;
    } else if (arg == "-n") {
        isDaemon = false;
        logFilename = "";
    } else if (arg == "-l") {
        logFilename = argsIter.next();
    } else if (arg == "-s") {
        mapServerURL = argsIter.next();
    } else if (arg == "-f") {
        staticServerURL = argsIter.next();
    } else if (arg == "-r") {
        rootPathname = argsIter.next();
    } else if (arg == "-p") {
        port = argsIter.next().toInt();
    } else if (arg == "--no-accelerometer") {
        runMovementDetector = false;
    } else if (arg == "--no-wifi") {
        runWiFiScanner = false;
    } else {
        usage();
    }
  }

  // Seed rng
  QTime time = QTime::currentTime();
  qsrand((uint)time.msec());

  settings = new QSettings(QSettings::SystemScope, MOLE_ORGANIZATION,
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
    mapServerURL = settings->value("map_server_url").toString();
  }
  if (settings->contains("fingerprint_server_url")) {
    staticServerURL = settings->value("fingerprint_server_url").toString();
  }
  if (settings->contains("port")) {
    port = settings->value("port").toInt();
  }
  if (settings->contains("root_path")) {
    rootPathname = settings->value("root_path").toString();
  }

  //////////////////////////////////////////////////////////
  // check a few things before daemonizing
  initCommon(this, logFilename);

  qWarning() << "Starting mole daemon "
             << "debug=" << debug
             << "port=" << port
             << "wifi_scanning=" << runWiFiScanner
             << "movement_detector=" << runMovementDetector
             << "logFilename=" << logFilename
             << "map_server_url=" << mapServerURL
             << "fingerprint_server_url=" << staticServerURL
             << "rootPath=" << rootPathname;

  if (isDaemon) {
    daemonize();
  } else {
    qDebug() << "not daemonizing";
  }

  // start create map directory
  if (!rootDir.exists("map")) {
    bool ret = rootDir.mkdir("map");
    if (!ret) {
      qFatal("Failed to create map directory. Exiting...");
      exit(-1);
    }
    qDebug() << "created map dir";
  }
  // end create map directory

  // reset session cookie on MOLEd restart
  resetSessionCookie();

  m_localizer = new Localizer(this);

  if (runMovementDetector && SpeedSensor::haveAccelerometer()) {
    m_scanQueue = new ScanQueue(this, m_localizer);
  } else {
    // since we are not detecting motion, only use this many scans
    // for localization
    const int maxActiveQueueLength = 12;
    m_scanQueue = new ScanQueue(this, m_localizer, maxActiveQueueLength);
  }

  m_binder = new Binder(this, m_localizer, m_scanQueue);
  m_localServer = new LocalServer(this, m_localizer, m_binder, port);

  m_speedSensor = 0;
  if (runMovementDetector && SpeedSensor::haveAccelerometer())
    m_speedSensor = new SpeedSensor(this, m_scanQueue);

  m_scanner = 0;
  if (runWiFiScanner)
    m_scanner = new Scanner(this, m_scanQueue, m_binder);

  connect(this, SIGNAL(aboutToQuit()), SLOT(handle_quit()));
}

void Core::handle_quit()
{
  qWarning() << "Stopping mole daemon...";

  delete m_scanQueue;
  if (m_scanner)
    delete m_scanner;
  if (m_speedSensor)
    delete m_speedSensor;
  delete m_localizer;
  delete m_localServer;
  delete m_binder;

  qWarning() << "Stopped mole daemon";
}

Core::~Core()
{
}

int Core::run()
{
  return exec();
}

int main(int argc, char *argv[])
{
  qDebug("Mole Daemon Starting");

  Core *app = new Core(argc, argv);
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
    if (lfp < 0)
      qFatal("Cannot open pidfile %s\n", qPrintable(pidfile));

    if (lockf(lfp, F_TLOCK, 0) < 0)
      qFatal("Can't get a lock on %s - another instance may be running\n", qPrintable(pidfile));
    QByteArray ba = QByteArray::number(::getpid());
    ::write(lfp, ba.constData(), ba.size());

    ::signal(SIGCHLD,SIG_IGN);
    ::signal(SIGTSTP,SIG_IGN);
    ::signal(SIGTTOU,SIG_IGN);
    ::signal(SIGTTIN,SIG_IGN);
}

void usage()
{
  qCritical() << "moled usage\n"
              << "-h print usage\n"
              << "-d debug\n"
              << "-n run in foreground\n"
              << "-s map server URL [" << DEFAULT_MAP_SERVER_URL << "] \n"
              << "-f fingerprint (static) server [" << DEFAULT_STATIC_SERVER_URL << "]\n"
              << "-l log file [" << DEFAULT_LOG_FILE << "]\n"
              << "-r root path [" << DEFAULT_ROOT_PATH << "]"
              << " (app data stored here)\n"
              << "-p local port [" << DEFAULT_LOCAL_PORT << "]\n"
              << "--no-accelerometer turn off movement detection\n"
              << "--no-wifi turn off wifi scanner\n";

  exit(0);
}
