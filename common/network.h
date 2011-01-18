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

#ifndef NETWORK_H_
#define NETWORK_H_

#include <QtCore>
#include <QNetworkRequest>
#include <QNetworkReply>
//#include <QNetworkCookie>

#include "mole.h"
#include "util.h"

/* #ifdef Q_WS_MAEMO_5 */
//#include "../libqjson_0.7.2_armel/usr/include/qjson/serializer.h"
//#include "../libqjson_0.7.2_armel/usr/include/qjson/parser.h"
/* #else */
/* #include "../libqjson_0.7.2_i386/usr/include/qjson/serializer.h" */
/* #include "../libqjson_0.7.2_i386/usr/include/qjson/parser.h" */
/* #endif */
//#include "../qjson/src/serializer.h"
//#include "../qjson/src/parser.h"

//#include "../../qjson/src/serializer.h"
//#include "../../qjson/src/parser.h"

#include <qjson/parser.h>
#include <qjson/serializer.h>
/*
class NetworkRequest : public QNetworkRequest {
  Q_OBJECT

 public:
  NetworkRequest(const QUrl &url);
  NetworkRequest(const QNetworkRequest &request);
  //~NetworkRequest ();

 private:
  int id;

};

class NetworkAccessManager : public QNetworkAccessManager {
  Q_OBJECT

 public:
  NetworkAccessManager(QObject *parent = 0);
  ~NetworkAccessManager ();

 private slots:
  void handle_network_finished (QNetworkReply* reply);

};
*/

void set_network_request_headers (QNetworkRequest &request);
int find_reply_latency_msec (QNetworkReply *reply);

QString get_user_cookie ();
void reset_user_cookie ();
void reset_session_cookie ();
QString get_random_cookie (int length);

#endif /* NETWORK_H_ */
