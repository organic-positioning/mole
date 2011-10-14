/*
 * Mole - Mobile Organic Localisation Engine
 * Copyright 2010-2011 Nokia Corporation.
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

#include "settings_access.h"

QString getUserCookie()
{
  if (!settings->contains("cookie") ||
      settings->value("cookie").toString().isEmpty()) {

    resetUserCookie();
    qDebug() << "reset user cookie";
  }

  if (!settings->contains("session") ||
      settings->value("session").toString().isEmpty()) {

    resetSessionCookie();
    qDebug() << "reset session cookie";
  }

  return settings->value("cookie").toString();
}

void resetUserCookie()
{
  settings->setValue("cookie", getRandomCookie(20));
  // also do session cookie
  resetSessionCookie();
}

void resetSessionCookie()
{
  settings->setValue("session", getRandomCookie(20));
}

QString getRandomCookie(int length)
{
  char alphanum [] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

  QString cookie;
  for (int i = 0; i < length; ++i)
    cookie.append(alphanum[randInt(0,35)]);

  return cookie;
}

bool getProximityActive() {
  if (settings->contains("proximity_active")) {
    return settings->value("proximity_active").toBool();
  }
  return false;
}

void setProximityActive(bool isActive) {
  settings->setValue("proximity_active", isActive);
}

QString getUserProximityName() {
  if (settings->contains("proximity_name")) {
    return settings->value("proximity_name").toString();
  }
  return QString ();
}

void setUserProximityName (QString name) {
  settings->setValue("proximity_name", name);
}
