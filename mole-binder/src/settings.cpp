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

#include "settings.h"

QString setting_cookie_key = "cookie";

// TODO The handling of whitespace in the username could be improved.
// e.g. it allows the user to input spaces, but these are not reflected in the underlying text.
// I don't think the inputmask should even allow whitespace...

// TODO when starting up, username does not get shown properly in center of window.

Settings::Settings(QWidget *parent) : QDialog (parent) {

  //local_wlan_mac = "00:00:00:00:00:00";
  //QSystemNetworkInfo info (parent);
  //local_wlan_mac = info.macAddress (QSystemNetworkInfo::WlanMode);

  //qDebug() << "cookie " << settings->value("cookie").toString();
  //qDebug() << "local mac " << local_wlan_mac;

  //username_completer = new UsernameCompleter (this);
  build_ui ();
  setWindowTitle ("Preferences");

}

void Settings::handle_quit() {
  qDebug () << "Settings: aboutToQuit";

}

Settings::~Settings() {
  qDebug () << "Settings: destructor";
}


void Settings::build_ui () {

  const int cookie_row = 0;

  //resize (QSize(UI_WIDTH, UI_HEIGHT).expandedTo(minimumSizeHint()));
  setStyleSheet("QToolButton { border: 0px; padding 0px; }");

  QGridLayout *layout = new QGridLayout ();

  const int max_cookie_length = 20;
  cookie = new QLineEdit (this);
  cookie->setMaxLength (max_cookie_length);
  //cookie->setText (QString (settings->value("cookie").toString()));
  cookie->setText (get_user_cookie());
  cookie->setEnabled (false);

  random_cookie_button = new QPushButton ("Regenerate", this);

  layout->addWidget (new QLabel ("Cookie:"), cookie_row, 0);
  layout->addWidget (cookie, cookie_row, 2);
  layout->addWidget (random_cookie_button, cookie_row, 3);

  // http://wiki.forum.nokia.com/index.php/Maemo_5_liblocation_minimalistic_test_application

  //layout->addWidget (new QLabel ("GPS:"), gps_row, 0);  
  //layout->addWidget (new QLabel ("To be implemented..."), gps_row, 1);  

  //layout->addWidget (new QLabel (""), empty_row, 0);
  //layout->setRowStretch (empty_row,2);


  QToolButton *cookie_help_button = new QToolButton (this);
  cookie_help_button->setIcon (QIcon(icon_root+"help.png"));
  cookie_help_button->setIconSize (QSize(icon_size_small,icon_size_small));
  connect (cookie_help_button, SIGNAL(clicked()), SLOT(handle_clicked_cookie_help()));
  layout->addWidget (cookie_help_button, cookie_row, 1);

  connect (random_cookie_button, SIGNAL(clicked()),
	   SLOT(handle_random_cookie()));
  

  setLayout (layout);

}




void Settings::handle_random_cookie () {
  reset_user_cookie ();
  cookie->setText (QString (settings->value(setting_cookie_key).toString()));
}



void Settings::handle_clicked_cookie_help() {
  qDebug () << "handle_clicked_cookie_help";

  QString help = 
    tr("Organic Indoor Positioning will use a random cookie to identify you when talking to a server.  You can regenerate this cookie whenever you like if you are concerned about your privacy.");

  QMessageBox::information
       (this, tr ("About Cookies"), help);

}

