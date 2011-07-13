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

  setStyleSheet("QToolButton { border: 0px; padding 0px; }");

  m_layout = new QGridLayout();

  const int maxCookieLength = 20;

  m_cookie = new QLineEdit(this);
  m_cookie->setMaxLength(maxCookieLength);
  m_cookie->setText(get_user_cookie());
  m_cookie->setEnabled(false);

  QPushButton *randomCookieButton = new QPushButton(tr("Regenerate"), this);

  m_layout->addWidget(new QLabel("Cookie:"), cookieRow, 0);
  m_layout->addWidget(m_cookie, cookieRow, 2);
  m_layout->addWidget(randomCookieButton, cookieRow, 3);

  QToolButton *cookieHelpButton = new QToolButton(this);
  cookieHelpButton->setIcon(QIcon(MOLE_ICON_PATH + "help.png"));
  cookieHelpButton->setIconSize(QSize(ICON_SIZE_SMALL, ICON_SIZE_SMALL));
  connect(cookieHelpButton, SIGNAL(clicked()), SLOT(onHelpCookieClicked()));
  m_layout->addWidget(cookieHelpButton, cookieRow, 1);

  connect(randomCookieButton, SIGNAL(clicked()), SLOT(onRandomCookieClicked()));

  setLayout(m_layout);
}

void Settings::onRandomCookieClicked()
{
  reset_user_cookie();
  m_cookie->setText(QString(settings->value("cookie").toString()));
}

void Settings::onHelpCookieClicked()
{
  qDebug() << "handle clicked cookie help";

  QString help =
    tr("Organic Indoor Positioning will use a random cookie to identify you when talking to a server.  You can regenerate this cookie whenever you like if you are concerned about your privacy.");

  QMessageBox::information(this, tr("About Cookies"), help);
}
