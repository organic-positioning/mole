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

const int min_username_length = 6;

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

  const int user_row = 0;
  const int checkin_row = 1;
  const int cookie_row = 2;
  //  const int scan_row = 3;
  const int empty_row = 3;
  const int reset_row = 4;
  //const int gps_row = 3;

  //resize (QSize(UI_WIDTH, UI_HEIGHT).expandedTo(minimumSizeHint()));
  setStyleSheet("QToolButton { border: 0px; padding 0px; }");


  QGridLayout *layout = new QGridLayout ();

  const int max_username_length = 40;
  username = new QLineEdit (this);
  username->setMaxLength (max_username_length);
  username->setEnabled (true);

  // min == 6
  // TODO change to allow periods
  username->setInputMask ("<ANNNNNnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn");
  //username->setInputMask ("<");

  username->setPlaceholderText ("Enter Username (optional)");
  
  //QValidator *validator = new UsernameValidator(username);
  //QRegExp rx ("[a-z][a-z0-9]+");
  //QValidator *validator = new QRegExpValidator(rx, username);
  //username->setValidator (validator);
  username->setCursorPosition (max_username_length+2);

  username_state = Invalid;
  if (settings->contains (setting_username_key)) {
    // we don't really know if this is valid or not
    username->setText (settings->value(setting_username_key).toString());
  }

  save_username_button = new QPushButton ("Save", this);
  save_username_button->setEnabled (false);
  layout->addWidget (new QLabel ("Username:"), user_row, 0);
  layout->addWidget (username, user_row, 2);
  layout->addWidget (save_username_button, user_row, 3);

  username_timer = new QTimer (this);
  username_timer->setSingleShot(true);
  username_timer->setInterval(500);
  connect(username_timer, SIGNAL(timeout()), SLOT(handle_check_username()));
  connect(username, SIGNAL(textEdited(QString)), username_timer, SLOT(start()));
  connect(username, SIGNAL(cursorPositionChanged(int,int)), SLOT(handle_username_focusIn()));

  //connect(username, SIGNAL(returnPressed()), SLOT(handle_username_return()));
  //connect(username, SIGNAL(editingFinished()), SLOT(handle_username_edited()));
  

  connect (save_username_button, SIGNAL(clicked()),
	   SLOT(handle_save_username()));


  layout->addWidget (new QLabel ("Check-in:"), checkin_row, 0);

  checkin_combo = new QComboBox (this);

  checkin_combo->insertItem (0, "Never");
  checkin_combo->insertItem (1, "Every 1 Minute");
  checkin_combo->insertItem (2, "Every 5 Minutes");
  checkin_combo->insertItem (3, "Every 30 Minutes");
  checkin_combo->insertItem (4, "Every 60 Minutes");
  enable_checkin_combo ();

  layout->addWidget (checkin_combo, checkin_row, 2);

  connect (checkin_combo, SIGNAL(currentIndexChanged(int)),
  	   SLOT(handle_checkin_combo_changed(int)));

  checkin_frequency = 0;
  if (settings->contains(setting_whereami_frequency_key)) {
    checkin_frequency = settings->value(setting_whereami_frequency_key).toInt();
  }

  checkin_combo->setCurrentIndex (checkin_frequency);



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

  reset_button = new QPushButton ("Reset All Settings", this);
  connect (reset_button, SIGNAL(clicked()), SLOT(handle_reset()));
  layout->addWidget (reset_button, reset_row, 2);


  layout->addWidget (new QLabel (""), empty_row, 0);
  layout->setRowStretch (empty_row,2);

  QToolButton *checkin_help_button = new QToolButton (this);
  checkin_help_button->setIcon (QIcon(icon_root+"help.png"));
  checkin_help_button->setIconSize (QSize(icon_size_small,icon_size_small));
  connect (checkin_help_button, SIGNAL(clicked()), SLOT(handle_clicked_checkin_help()));
  layout->addWidget (checkin_help_button, user_row, 1);

  QToolButton *cookie_help_button = new QToolButton (this);
  cookie_help_button->setIcon (QIcon(icon_root+"help.png"));
  cookie_help_button->setIconSize (QSize(icon_size_small,icon_size_small));
  connect (cookie_help_button, SIGNAL(clicked()), SLOT(handle_clicked_cookie_help()));
  layout->addWidget (cookie_help_button, cookie_row, 1);

  connect (random_cookie_button, SIGNAL(clicked()),
	   SLOT(handle_random_cookie()));
  

  setLayout (layout);

}

/*
void Settings::set_cookie (QString value) {
  //settings->setValue("cookie", value);

}
*/

/*
void Settings::handle_reset_cookie () {
  set_cookie (local_wlan_mac);
}
*/

/*
void Settings::handle_save_cookie () {
  qDebug() << "handle_save_cookie" << cookie->text();
  settings->setValue("cookie", cookie->text());
}
*/

void Settings::handle_username_focusIn () {
  qDebug () << "handle_username_focusIn";


  if (username->cursorPosition () > username->text().length()) {
    username->setCursorPosition (username->text().length());    
  }


  /*
  if (username->text().isEmpty()) {
    qDebug () << "setting cursor";
    username->setCursorPosition (0);    
  }
  */
}


void Settings::handle_check_username () {
  username_timer->stop();
  QString text = username->text();

  int j = 0;
  for (int i = 0; i < text.length(); i++)
    if (!text[i].isSpace())
      text[j++] = text[i];
  text.resize(j);
  username->setText(text);

  /*
  if (username->cursorPosition () > username->text().length()) {
    username->setCursorPosition (username->text().length());    
  }
  */

  //UsernameState current_username_state = username_state;
  qDebug () << "handle_check_username " << text;
  //QMetaObject::invokeMethod(username, "returnPressed");
  if (text.length() >= min_username_length) {
    // TODO check that this username is unique
    username_state = Acceptable;
  } else {
    username_state = Invalid;
  }
  //if (current_username_state != username_state) {
    handle_username_state_change ();
    //}
}

void Settings::handle_username_state_change () {
  qDebug () << "handle_username_state_change " << username_state;
  switch (username_state) {
  case Invalid:
    username->setStyleSheet("QLineEdit { color: red; }");
    save_username_button->setEnabled (false);
    break;
  case Intermediate:
    username->setStyleSheet("QLineEdit { color: yellow; }");
    save_username_button->setEnabled (false);
    break;
  case Acceptable:
    username->setStyleSheet("QLineEdit { color: green; }");
    save_username_button->setEnabled (true);
    break;
  }
}

/*
void Settings::handle_username_return () {
  qDebug () << "handle_username_return ";
}
*/

/*
void Settings::handle_username_edited () {
  qDebug () << "handle_username_edited ";
}
*/

void Settings::handle_random_cookie () {
  reset_user_cookie ();
  cookie->setText (QString (settings->value(setting_cookie_key).toString()));
}

void Settings::handle_checkin_combo_changed(int value) {
  qDebug () << "handle_checkin_combo_changed " << value;
  settings->setValue (setting_whereami_frequency_key, value);
  emit_whereami_update ();
}

void Settings::handle_save_username() {
  settings->setValue (setting_username_key, username->text());
  if (enable_checkin_combo ()) {
    emit_whereami_update ();
  }
  save_username_button->setEnabled (false);
}

bool Settings::enable_checkin_combo () {
  if (username->text().isEmpty()) {
    checkin_combo->setEnabled (false);
    checkin_combo->setCurrentIndex (0);
    return false;
  } else {
    checkin_combo->setEnabled (true);
    return true;
  }
}

void Settings::emit_whereami_update () {
  QDBusMessage msg = QDBusMessage::createSignal("/", "com.nokia.moled", "WhereAmIChanged");
  QDBusConnection::sessionBus().send(msg);
}

void Settings::handle_clicked_checkin_help() {
  qDebug () << "handle_clicked_checkin_help";

  QString help = 
    tr("If you like, you can select a username for yourself and set an interval for posting your location automatically.  Other users and apps can then be aware of your position.  Usernames must be at least six characters long, can contain letters and numbers, and must be unique.");

  QMessageBox::information
       (this, tr ("About Check-In"), help);

}

void Settings::handle_clicked_cookie_help() {
  qDebug () << "handle_clicked_cookie_help";

  QString help = 
    tr("Organic Indoor Positioning will use a random cookie to identify you when talking to a server.  You can regenerate this cookie whenever you like if you are concerned about your privacy.");

  QMessageBox::information
       (this, tr ("About Cookies"), help);

}

void Settings::handle_reset () {
  username->setText ("");
  handle_save_username ();
  reset_user_cookie ();
  check_settings (true);
}
