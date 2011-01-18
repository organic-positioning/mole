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

#include "whereami.h"

// IDEA allow creation of other places / plugin for emiting location

WhereAmI::WhereAmI(QObject *parent) :
  QObject(parent) {

  //network_manager = new QNetworkAccessManager (this);
  //connect(network_manager, SIGNAL(finished(QNetworkReply*)),
  //SLOT(post_response(QNetworkReply*)));

  post_timer = new QTimer(this);
  connect(post_timer, SIGNAL(timeout()),
	  this, SLOT (post_location()));

  handle_whereami_update ();

  QDBusConnection::sessionBus().connect
    (QString(), QString(),
     "com.nokia.moled", "WhereAmIChanged", this, 
     SLOT(handle_whereami_update()));

}

WhereAmI::~WhereAmI () {
  //delete network_manager;
  delete post_timer;
}

void WhereAmI::handle_whereami_update () {

  int interval_index = 0;
  int interval;
  
  if (!settings->contains(setting_whereami_frequency_key)) {
    return;
  }

  interval_index = settings->value(setting_whereami_frequency_key).toInt();

  // ok, so these are hardcoded from the UI.
  switch (interval_index) {
  case 0:
    return;
  case 1:
    interval = 1000 * 60 * 1;
    break;
  case 2:
    interval = 1000 * 60 * 5;
    break;
  case 3:
    interval = 1000 * 60 * 30;
    break;
  case 4:
    interval = 1000 * 60 * 60;
    break;
  default:
    settings->setValue(setting_whereami_frequency_key, 0);
    return;
  }

  post_timer->start (interval);

}

void WhereAmI::post_location() {

  // do we have the current estimate?
  if (current_estimated_space_name.isEmpty() ||
      current_estimated_space_name == unknown_space_name) {
    qDebug () << "not posting location because no estimate";
    return;
  }

  QString username;
  if (settings->contains (setting_username_key)) {
    username = settings->value(setting_username_key).toString();
  }


  QNetworkRequest request;
  QString url_str = setting_server_url_value;
  QUrl url(url_str.append("/iamhere"));

  url.addQueryItem ("username", username);
  url.addQueryItem ("est", current_estimated_space_name);
  request.setUrl(url);

  qDebug () << "req " << url;

  set_network_request_headers (request);
  reply = network_access_manager->get (request);
  connect (reply, SIGNAL(finished()), 
	     SLOT (post_response()));

}

void WhereAmI::post_response () {

  if (reply->error() != QNetworkReply::NoError) {
    qWarning() << "post i am here request failed "
	       << reply->errorString()
      	       << " url " << reply->url();


  } else {

    qDebug () << "post i am here request succeeded";

  }
  reply->deleteLater ();
}
