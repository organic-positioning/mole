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

#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <QtGui>
#include <QtDBus>
//#include "username.h"
#include "common.h"

enum UsernameState { Invalid, Intermediate, Acceptable };

class Settings : public QDialog {
  Q_OBJECT

public:
  Settings(QWidget *parent = 0);
  ~Settings ();

  void set_cookie (QString value);


public slots:
  void handle_quit();

private:
  void build_ui ();
  QString local_wlan_mac;

  QLineEdit *cookie;

  QPushButton *random_cookie_button;

  /*
  QPushButton *save_username_button;
  QLineEdit *username;
  QComboBox *checkin_combo;
  int checkin_frequency;
  QTimer *username_timer;

  QPushButton *reset_button;

  bool enable_checkin_combo ();
  void emit_whereami_update ();

  UsernameState username_state;

  void set_username_color ();
  */

private slots:
  void handle_random_cookie ();
  void handle_clicked_cookie_help ();
  /*
  void handle_checkin_combo_changed(int value);
  void handle_save_username();
  void handle_check_username();

  void handle_clicked_checkin_help ();


  //void handle_username_return ();
  //void handle_username_edited ();

  void handle_username_focusIn ();

  void handle_username_state_change ();

  void handle_reset ();
  */
};

#endif /* SETTINGS_H_ */
