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

#include <QtCore>
#include <QtGui>
#include <QtDBus>
#include <QApplication>

#include "mole.h"
#include "network.h"

QSettings *settings;
QDir *app_root_dir;

#ifdef Q_WS_MAEMO_5

const int UI_WIDTH = 800;
const int UI_HEIGHT = 400;

#else

const int UI_WIDTH = 510;
const int UI_HEIGHT = 240;

#endif

bool debug = false;
bool verbose = false;


//QString setting_username_value = "";
//QString setting_whereami_frequency_value = "";
QString setting_server_url_value = "";
//QString setting_version_value = "";
QString setting_mapserver_url_value = "";
QString setting_user_agent_value = "";

QNetworkAccessManager *network_access_manager;

QFile *log_file = NULL;
QTextStream *log_stream = NULL;

struct CleanExit{
    CleanExit() {
        signal(SIGINT, &CleanExit::exitQt);
        signal(SIGTERM, &CleanExit::exitQt);
        //signal(SIGBREAK, &CleanExit::exitQt) ;
        signal(SIGHUP, &CleanExit::exitQt); // new
    }

  static void exitQt(int /*sig*/) {
      *log_stream << QDateTime::currentDateTime().toString() << " I: Exiting.\n\n";
      if (log_file != NULL) {
	log_file->close ();
      }
      QCoreApplication::exit(0);
    }
};


void output_handler(QtMsgType type, const char *msg)
 {
     switch (type) {
     case QtDebugMsg:
       if (debug) {
	 *log_stream << QDateTime::currentDateTime().toString() << " D: " << msg << "\n";
	 log_stream->flush();
       }
       break;
     case QtWarningMsg:
       *log_stream << QDateTime::currentDateTime().toString() << " I: " << msg << "\n";
       log_stream->flush();
       break;
     case QtCriticalMsg:
       *log_stream << QDateTime::currentDateTime().toString() << " C: " << msg << "\n";
       log_stream->flush();
       break;
     case QtFatalMsg:
       *log_stream << QDateTime::currentDateTime().toString() << " F: " << msg << "\n";
       log_stream->flush();
       abort();
     }
 }


void init_mole_app(int /*argc*/, char */*argv*/[], QCoreApplication *app, QString app_name, QString log_file_name)
{
  if (log_file_name != NULL) {
    log_file = new QFile (log_file_name);
    if (log_file->open (QFile::WriteOnly | QFile::Append)) {
      log_stream = new QTextStream (log_file);
    } else {
      fprintf (stderr, "Could not open log file %s.\n\n", log_file_name.toAscii().data());
      exit (-1);
    }
  } else {
    log_stream = new QTextStream (stderr);
  }

  CleanExit cleanExit;

  qInstallMsgHandler (output_handler);

  // see http://doc.trolltech.com/4.6/examples-mainwindow.html
  // good for saying where e.g. images (pngs) are
  //Q_INIT_RESOURCE(application);

  // seed rng
  QTime time = QTime::currentTime();
  qsrand ((uint)time.msec());

  app->setOrganizationName(MOLE_ORGANIZATION);
  app->setApplicationName(app_name);

  settings = new QSettings (MOLE_ORGANIZATION, MOLE_APPLICATION);
  
  settings->setValue(setting_version_key,MOLE_VERSION);

  qWarning ("Setting default server urls");
  settings->setValue(setting_server_url_key, MOLE_SERVER_URL);
  settings->setValue(setting_mapserver_url_key, MOLE_MAPSERVER_URL);

  check_settings (false);


  /*
  if (!QDBusConnection::sessionBus().isConnected()) {
    qWarning("Cannot connect to the D-Bus session bus.\n"
	     "Please check your system settings and try again.\n");
    QCoreApplication::exit (-1);      
  }

  if (!QDBusConnection::systemBus().isConnected()) {
    qWarning("Cannot connect to the D-Bus system bus.\n"
	     "Please check your system settings and try again.\n");
    QCoreApplication::exit (-1);      
  }
  */

  QDir user_home = QDir::home();

  if (!user_home.exists (".mole")) {
    bool ret = user_home.mkdir (".mole");
    if (!ret) {
      qFatal ("Failed to create HOME/.mole directory. Exiting...");
      QCoreApplication::exit (-1);      
    }
    qDebug () << "created app home dir";
  }
  QString home = user_home.absolutePath();
  home.append ("/.mole");
  app_root_dir = new QDir (home);


  // start create map directory
  if (!app_root_dir->exists ("map")) {
    bool ret = app_root_dir->mkdir ("map");

    qDebug () << "root " << app_root_dir->absolutePath();

    if (!ret) {
      qFatal ("Failed to create map directory. Exiting...");
      QCoreApplication::exit (-1);
    }
    qDebug () << "created map dir";
  }
  // end create map directory


  network_access_manager = new QNetworkAccessManager (app);

  reset_session_cookie ();

  qWarning() << "Starting " << app_name << " " << MOLE_VERSION;

}

void check_settings (bool reset) {

  if (reset) {
    settings->clear();
  }

  // Let the user pass these two URLs on the command line
  // and preserve them throughout the run of the program
  // even if the user hits reset.

  // The resetted values will be used the next time it is started.

  if (reset || !settings->contains(setting_server_url_key)) {
    settings->setValue(setting_server_url_key, MOLE_SERVER_URL);
    qDebug("Set mole_server_url to default");
  }
  if (setting_server_url_value.isEmpty()) {
    setting_server_url_value = settings->value(setting_server_url_key).toString();
  }

  if (reset || !settings->contains(setting_mapserver_url_key)) {
    settings->setValue(setting_mapserver_url_key, MOLE_MAPSERVER_URL);
    qDebug("Set mole_mapserver_url to default");
  }
  if (setting_mapserver_url_value.isEmpty()) {
    setting_mapserver_url_value = settings->value(setting_mapserver_url_key).toString();
  }

  QString agent = "moled/";
  agent.append (settings->value(setting_version_key).toString());
  setting_user_agent_value = agent;
  settings->setValue(setting_user_agent_key, agent);
  qDebug () << "Set user_agent" << setting_user_agent_value;

  // TODO delete .mole directory

}

/*
QString get_current_version () {
  QString version = "$Rev: 2100 $";
  version.remove (0,6);
  version.remove (version.length()-2,version.length()-1);
  return version;
}
*/

/*
void set_timer (QTimer *timer, bool increase, int &period, int min, int max) {

  if (increase) {
    period = min;
  } else {
    period *= 2;
    if (period > max) {
      period = max;
    }
  }
  
  timer->start (rand_poisson(period));

}
*/

/*
// see adjusting timer
void start_timer (QTimer *timer, int period) {
  timer->start (rand_poisson(period));
}
*/
