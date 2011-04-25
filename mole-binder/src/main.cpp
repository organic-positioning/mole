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

#include "binder.h"
#include "ground_truth.h"

QString icon_root = "/usr/share/mole/icons/";

#ifdef Q_WS_MAEMO_5
const int UI_WIDTH = 800;
const int UI_HEIGHT = 400;
const int icon_size_reg = 64;
const int icon_size_small = 40;
#else
const int UI_WIDTH = 510;
const int UI_HEIGHT = 240;
const int icon_size_reg = 32;
const int icon_size_small = 20;
#endif

QString mole = QString("Mol")+QChar(0x00E9);
QString mole_binder = mole+" Binder";

void usage ();


int main(int argc, char *argv[])
{
  initSettings ();

  QApplication *app = new QApplication(argc, argv);
  QString groundTruthFilename = "";

  //settings = new QSettings (DEFAULT_CONFIG_FILE,QSettings::NativeFormat);

  // let user's settings override system's via fallback mechanism
  settings = new QSettings (MOLE_ORGANIZATION,
			    MOLE_APPLICATION, app);


  if (settings->contains("map_server_url")) {
    mapServerURL = settings->value ("map_server_url").toString();
  }
  if (settings->contains("fingerprint_server_url")) {
    staticServerURL = settings->value ("fingerprint_server_url").toString();
  }

  //if (settings->contains("root_path")) {
  //rootPathname = settings->value ("root_path").toString();
  //}


  QString logFilename = "";

  QStringList args = QCoreApplication::arguments();
  QStringListIterator args_iter (args);
  args_iter.next(); // = mole-binder

  while (args_iter.hasNext()) {
    QString arg = args_iter.next();
    //qDebug () << arg;
    if (arg == "-d") {
      debug = true;
    } else if (arg == "-s") {
	mapServerURL = args_iter.next();
	mapServerURL.trimmed();
      } else if (arg == "-f") {
	staticServerURL = args_iter.next();
	staticServerURL.trimmed();
      } else if (arg == "-l") {
	logFilename = args_iter.next();
	logFilename.trimmed();
    } else if (arg == "-T") {
      groundTruthFilename = args_iter.next();
    } else {
      usage ();
    }

  }

  initCommon(app, logFilename);

  //app->setStyleSheet (normal_group_box);

  QMainWindow *main_window = new QMainWindow ();
  // LINUX

  //main_window->resize(QSize(UI_WIDTH, UI_HEIGHT).expandedTo(main_window->minimumSizeHint()));

  //main_window->resize(UI_MAIN_WIDTH, UI_MAIN_HEIGHT);


  //QString title ("Mol")+QChar(0x0107);
  //main_window->setWindowTitle(title);

  main_window->setWindowTitle("Organic Indoor Positioning");

  main_window->setWindowIcon(QIcon(icon_root+"mole-binder.png"));

#ifdef Q_WS_MAEMO_5
  main_window->setStyleSheet("QMainWindow { background-color: #222222; } QGroupBox { background-color: #000000; }");
#endif

  QWidget *main_widget = new QWidget (main_window);
  main_window->setCentralWidget (main_widget);

  if (groundTruthFilename.isEmpty()) {
    Binder *binder = new Binder (main_widget);

  //QLineEdit *edit = new QLineEdit (main_widget);
  //QToolButton *about_button = new QToolButton (main_widget);
  //about_button->setIcon (QIcon("about.png"));
    app->connect (app, SIGNAL(aboutToQuit()), binder, SLOT(handle_quit()));
  } else {
    GroundTruth *gt = new GroundTruth (main_widget, groundTruthFilename);
    app->connect (app, SIGNAL(aboutToQuit()), gt, SLOT(handle_quit()));
  }
  
  //binder->show();
  main_window->show();

  main_window->resize(QSize(UI_WIDTH, UI_HEIGHT).expandedTo(main_window->minimumSizeHint()));

  return app->exec();
}

void usage () {
  qCritical ()
    << "mole-binder\n"
    << "-d debug\n"
    << "-s map server url\n"
    << "-f fingerprint (static) server\n"
    << "-l log file\n"
    << "-T ground truth file (for evaluation)";
}
