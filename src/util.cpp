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

#include "util.h"

#include <QSettings>
#include <QSystemDeviceInfo>
QTM_USE_NAMESPACE

bool debug = false;
FILE *logStream = NULL;

//////////////////////////////////////////////////////////
void initLogger(const char* logFilename) {
  // Initialize logger
  if (logFilename != NULL) {
    if((logStream = fopen(logFilename, "a")) == NULL) {
      fprintf(stderr, "Could not open log file %s.\n\n", logFilename);
      exit(-1);
    }
  } else {
    logStream = stderr;
  }
}

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
    exit(-1);
  }
}

//////////////////////////////////////////////////////////////////////////////
void daemonize(QString exec)
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

  QString pidfile = "/var/run/" + exec + ".pid";
  //QString pidfile = "/var/run/" + exec + "mole-scanner.pid";
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

//////////////////////////////////////////////////////////////////////////////
// return our uuid, and store it in settings if it wasn't already there
QString getUUID() {
  QSettings settings;
  if (!settings.contains("uuid") ||
      settings.value("uuid").toString().size() < 35) {
    QString uuid;
    for (int i = 0; i < 4; i++) {
      int nextRand = qrand();
      uuid.append(QString::number(nextRand, 16));
      if (i < 3) {
        uuid.append("-");
      }
      qDebug() << "uuid" << uuid;
    }
    settings.setValue("uuid", uuid);
  }
  return settings.value("uuid").toString();
}

//////////////////////////////////////////////////////////////////////////////
// return a description of the device
QString getDeviceInfo() {
  QSettings settings;
  if (!settings.contains("deviceinfo")) {
    QSystemDeviceInfo device;
    qDebug() << "product name  " << device.productName().simplified();
    QString deviceInfo;
    deviceInfo.append(device.productName().simplified());
    deviceInfo.append("/");
    qDebug() << "model " << device.model().simplified();
    deviceInfo.append(device.model().simplified());
    deviceInfo.append("/");
    qDebug() << "manufacturer  " << device.manufacturer().simplified();
    deviceInfo.append(device.manufacturer().simplified());
    settings.setValue("deviceinfo", deviceInfo);
  }
  return settings.value("deviceinfo").toString();
}

void serializeSource(QVariantMap &map) {
  map["uuid"] = getUUID();
  map["device"] = getDeviceInfo();
  map["version"] = MOLE_VERSION;
}
