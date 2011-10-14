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

#include "network.h"

#include "mole.h"
#include "util.h"
#include "settings_access.h"

void setNetworkRequestHeaders(QNetworkRequest &request)
{
  request.setRawHeader("Create-Stamp", (QTime::currentTime()).toString().toAscii());

  QString agent = "moled/";
  agent.append(MOLE_VERSION);

  request.setRawHeader(QString("User-Agent").toAscii(),
                       settings->value("user_agent").toString().toAscii());

  getUserCookie();

  // just setting cookies directly so that we do not have to deal with
  // sharing a CookieJar among networkmanagers
  QString cookies;
  cookies.append("cookie=");
  cookies.append(settings->value("cookie").toString());

  cookies.append("; mole_version=");
  cookies.append(MOLE_VERSION);

  cookies.append("; session=");
  cookies.append(settings->value("session").toString());

  request.setRawHeader(QString("Cookie").toAscii(),
                       cookies.toAscii());

  qDebug() << "setting network request header...";
}

int findReplyLatencyMsec(QNetworkReply *reply)
{
  QNetworkRequest request = reply->request();

  if (request.hasRawHeader("Create-Stamp")) {
    QVariant header = request.rawHeader("Create-Stamp");
    QTime sendTime = QTime::fromString(header.toString());

    qDebug() << " raw sendTime " << sendTime.toString();

    int elapsed = sendTime.elapsed();

    // Because we are using QTime, we could potentially get very large
    // values if midnight occurs in between sending and receiving, so
    // this screens out those values
    if (elapsed > 0 && elapsed < (10 * 60 * 1000)) {
      qDebug() << "URL elapsed " << elapsed;
      return elapsed;
    }
  }
  return -1;
}


