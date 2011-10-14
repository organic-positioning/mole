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

#include <QDialog>

#include "common.h"

enum UsernameState {Invalid, Intermediate, Acceptable};

class Settings : public QDialog
{
  Q_OBJECT

public:
  Settings(QWidget *parent = 0);
  ~Settings();

  void setCookie(QString value);

private:
  void buildUI();
  void notifyDaemon();

  bool m_proximityActiveCached;
  QGridLayout *m_layout;
  QLineEdit *m_cookie;
  QLineEdit *m_proximityName;
  QPushButton *saveProximityChangeButton;
  QRadioButton *m_proximityActive;

private slots:
  void onRandomCookieClicked();
  void onHelpCookieClicked();
  void onSaveProximityChangeClicked();
  void onHelpProximityClicked();
  void onProximityActiveClicked();
  void onProximityNameChanged(QString name);
};

#endif /* SETTINGS_H_ */
