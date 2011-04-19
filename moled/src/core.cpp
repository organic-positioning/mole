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
#include "core.h"

void print_usage () {
  qFatal("moled\n -d debug\n -r rapid scan mode (improves response to movement, hurts battery life)\n -s server_url\n -m mapserver_url\n");

}

Core::Core (int argc, char *argv[]) : QCoreApplication (argc, argv) {

  //char c = 0;
  bool rapid_mode = false;
  QString log_file = "/var/log/moled/moled.log";

  QStringList args = QCoreApplication::arguments();
  QStringListIterator args_iter (args);
  args_iter.next(); // moled

  while (args_iter.hasNext()) {
    QString arg = args_iter.next();
    //qDebug () << arg;
    if (arg == "-d") {
      debug = true;
    } else if (arg == "-D") {
      verbose = true;
      debug = true;
    } else if (arg == "-r") {
      rapid_mode = true;
    } else if (arg == "-l") {
      log_file = args_iter.next();
    } else if (arg == "-s") {
      setting_server_url_value = args_iter.next();
      setting_server_url_value.trimmed();
    } else if (arg == "-m") {
      setting_mapserver_url_value = args_iter.next();      
      setting_mapserver_url_value.trimmed();
    } else {
      print_usage ();
      this->exit (0);
    }

  }

  // not using log_file for now
  init_mole_app (argc, argv, this, "moled", NULL);

  // reset session cookie on MOLEd restart
  reset_session_cookie ();


  qDebug() << "mole_server_url" << setting_server_url_value
	   << "mole_mapserver_url" << setting_mapserver_url_value;

  /*
  if (!QDBusConnection::systemBus().isConnected()) {
    qFatal("Cannot connect to the D-Bus system bus.\n"
	   "Please check your system settings and try again.\n");
    //return 1;
  }
  */

  qWarning ("Starting whereami");
  whereAmI = new WhereAmI (this);
  qWarning ("Starting binder");
  binder = new Binder (this);
  //binder = NULL;
  localizer = new Localizer (this, binder);

#ifdef Q_WS_MAEMO_5
  qWarning ("Starting old speedsensor");

  speedsensor = new SpeedSensor2 ();

  // TODO Scanner::Active vs Scanner::Passive does not seem to do anything
  Scanner::Mode mode = Scanner::Passive;
  //Scanner::Mode mode = Scanner::Active;
  if (rapid_mode) {
    mode = Scanner::Active;
  }

  qWarning ("Starting scanner");

  scanner = new Scanner (this, localizer, binder, mode);

#else
  scanner = new Scanner (this, localizer, binder);

#endif

  connect (this, SIGNAL(aboutToQuit()), SLOT(handle_quit()));
    
}

void Core::handle_quit () {
  qWarning () << "Stopping MOLE Daemon...";
  delete binder;
  delete localizer;
  delete scanner;

  qDebug () << "deleting speedsensor";

#ifdef Q_WS_MAEMO_5
  if (speedsensor != NULL) {
    speedsensor->shutdown ();
    delete speedsensor;
  }
#endif

  qWarning () << "Stopped MOLE Daemon";
}

Core::~Core () {
  
}

int Core::run () {
  return exec ();
}

