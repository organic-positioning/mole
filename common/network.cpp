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

#include "network.h"

void set_network_request_headers (QNetworkRequest &request) {

  request.setRawHeader ("Create-Stamp", (QTime::currentTime()).toString().toAscii());

  /*
    // already done in init app
  if (!settings->contains("user_agent")) {
    QString agent = "moled/";
    agent.append (settings->value("version").toString());
    settings->setValue("user_agent", agent);
    qDebug("Set user_agent");
  }
  */

  request.setRawHeader(QString("User-Agent").toAscii(),
                       settings->value("user_agent").toString().toAscii());

  get_user_cookie();

  // just setting cookies directly so that we do not have to deal with
  // sharing a CookieJar among networkmanagers

  QString cookies;

  cookies.append ("cookie=");
  cookies.append (settings->value("cookie").toString());

  cookies.append ("; mole_version=");
  cookies.append (settings->value("version").toString());

  cookies.append ("; session=");
  cookies.append (settings->value("session").toString());

  //qDebug () << "session " << settings->value("session").toString();

  request.setRawHeader(QString("Cookie").toAscii(),
                       cookies.toAscii());

  qDebug() << "sending....";

}

int find_reply_latency_msec (QNetworkReply *reply) {

  QNetworkRequest request = reply->request();

  if (request.hasRawHeader ("Create-Stamp")) {
    QVariant header = request.rawHeader ("Create-Stamp");

    QTime send_time = QTime::fromString(header.toString());

    qDebug () << " raw send_time " << send_time.toString();

    int elapsed = send_time.elapsed();

    // Because we are using QTime, we could potentially get very large
    // values if midnight occurs in between sending and receiving, so
    // this screens out those values

    if (elapsed > 0 && elapsed < (10*60*1000)) {
      qDebug () << "URL elapsed " << elapsed;
      return elapsed;
    }
  }
  return -1;

}

QString get_user_cookie () {

  if (!settings->contains("cookie") ||
      settings->value("cookie").toString().isEmpty()) {

    reset_user_cookie ();

    qDebug("reset user cookie");
  }

  if (!settings->contains("session") ||
      settings->value("session").toString().isEmpty()) {

    reset_session_cookie ();

    qDebug("reset session cookie");
  }

  return settings->value("cookie").toString();
}

void reset_user_cookie () {
  settings->setValue("cookie",get_random_cookie(20));
  // also do session cookie
  reset_session_cookie ();
}

void reset_session_cookie () {
  settings->setValue("session",get_random_cookie(20));
}

QString get_random_cookie (int length) {
  char alphanum [] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

  QString cookie;
  for (int i = 0; i < length; i++) {
    cookie.append (alphanum[rand_int(0,35)]);

  }

  //qDebug () << "new cookie " << cookie;
  return cookie;

}
