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


// TODO The handling of whitespace in the username could be improved.
// e.g. it allows the user to input spaces, but these are not reflected in the underlying text.
// I don't think the inputmask should even allow whitespace...

// TODO when starting up, username does not get shown properly in center of window.

Settings::Settings(QWidget *parent)
  : QDialog(parent)
{
  buildUI();
  setWindowTitle(tr("Preferences"));
}

Settings::~Settings()
{
  qDebug() << "Settings: destructor";
  QLayoutItem *child;
  while (!m_layout->isEmpty()) {
    child = m_layout->takeAt(0);
    m_layout->removeItem(child);
    delete child;
  }
  delete m_layout;
}

void Settings::buildUI()
{
  const int cookieRow = 0;
  const int proximityRow = 1;

  setStyleSheet("QToolButton { border: 0px; padding 0px; }");

  m_layout = new QGridLayout();

  const int maxCookieLength = 20;
  const int maxProximityLength = 40;

  /////////////////////////////////////////////////////////

  m_cookie = new QLineEdit(this);
  m_cookie->setMaxLength(maxCookieLength);
  m_cookie->setText(getUserCookie());
  m_cookie->setEnabled(false);

  QPushButton *randomCookieButton = new QPushButton(tr("Regenerate"), this);

  m_layout->addWidget(new QLabel("Cookie:"), cookieRow, 0);
  m_layout->addWidget(m_cookie, cookieRow, 2);
  m_layout->addWidget(randomCookieButton, cookieRow, 3);
  connect(randomCookieButton, SIGNAL(clicked()), SLOT(onRandomCookieClicked()));

  QToolButton *cookieHelpButton = new QToolButton(this);
  cookieHelpButton->setIcon(QIcon(MOLE_ICON_PATH + "help.png"));
  cookieHelpButton->setIconSize(QSize(ICON_SIZE_SMALL, ICON_SIZE_SMALL));
  connect(cookieHelpButton, SIGNAL(clicked()), SLOT(onHelpCookieClicked()));
  m_layout->addWidget(cookieHelpButton, cookieRow, 1);

  /////////////////////////////////////////////////////////

  m_proximityName = new QLineEdit(this);
  m_proximityName->setMaxLength(maxProximityLength);
  m_proximityName->setText(getUserProximityName());
  m_proximityName->setEnabled(true);
  m_proximityName->setPlaceholderText("Display name");
  connect (m_proximityName, SIGNAL(textChanged(QString)), SLOT(onProximityNameChanged(QString)));

  m_proximityActive = new QRadioButton ("Enable", this);
  m_proximityActive->setChecked(getProximityActive());
  m_proximityActiveCached = getProximityActive();
  connect (m_proximityActive, SIGNAL(clicked()), SLOT(onProximityActiveClicked()));

  saveProximityChangeButton = new QPushButton(tr("Save"), this);
  saveProximityChangeButton->setEnabled(false);

  m_layout->addWidget(new QLabel("Proximity:"), proximityRow, 0);
  m_layout->addWidget(m_proximityName, proximityRow, 2);
  m_layout->addWidget(m_proximityActive, proximityRow, 3);
  m_layout->addWidget(saveProximityChangeButton, proximityRow, 4);

  QToolButton *helpProximityButton = new QToolButton(this);
  helpProximityButton->setIcon(QIcon(MOLE_ICON_PATH + "help.png"));
  helpProximityButton->setIconSize(QSize(ICON_SIZE_SMALL, ICON_SIZE_SMALL));
  connect(helpProximityButton, SIGNAL(clicked()), SLOT(onHelpProximityClicked()));
  m_layout->addWidget(helpProximityButton, proximityRow, 1);

  connect(saveProximityChangeButton, SIGNAL(clicked()), SLOT(onSaveProximityChangeClicked()));

  setLayout(m_layout);
}

void Settings::onRandomCookieClicked()
{
  qDebug () << "onRandomCookieClicked";
  resetUserCookie();
  m_cookie->setText(QString(settings->value("cookie").toString()));
}

void Settings::onSaveProximityChangeClicked()
{
  setUserProximityName(m_proximityName->text());
  setProximityActive (m_proximityActive->isChecked());
  qDebug () << "saved" << m_proximityName->text() << "settings" << getUserProximityName();
  notifyDaemon ();
  saveProximityChangeButton->setEnabled(false);
}

void Settings::onProximityActiveClicked() {
  qDebug () << "onProximityActiveClicked" << m_proximityActive->isChecked();
  saveProximityChangeButton->setEnabled(true);
  m_proximityActiveCached = m_proximityActive->isChecked();
}

void Settings::onProximityNameChanged(QString name) {
  qDebug () << "onProximityNameChanged" << name;
  saveProximityChangeButton->setEnabled(true);
  if (name.isEmpty()) {
    m_proximityActive->setChecked(false);
  } else {
    m_proximityActive->setChecked(m_proximityActiveCached);
  }
}

void Settings::onHelpCookieClicked()
{
  qDebug() << "handle clicked cookie help";

  QString help =
    tr("Organic Indoor Positioning will use a random cookie to identify you when talking to a server.  You can regenerate this cookie whenever you like if you are concerned about your privacy.");

  QMessageBox::information(this, tr("About Cookies"), help);
}

void Settings::onHelpProximityClicked()
{
  qDebug() << "handle clicked proximity help";

  QString help =
    tr("Organic Indoor Positioning can notify others when you are near them, indoors and out.  See who is near you by touching the @ button.");

  QMessageBox::information(this, tr("About Cookies"), help);
}

void Settings::notifyDaemon() {
  QDBusMessage msg = QDBusMessage::createSignal("/", "com.nokia.moled", "ProximitySettings");
  msg << getProximityActive();
  msg << getUserProximityName();
  QDBusConnection::systemBus().send(msg);
}
