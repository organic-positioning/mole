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

#ifndef WHEREAMI_H_
#define WHEREAMI_H_

#include <QNetworkReply>
#include <QDebug>
#include <QNetworkRequest>
#include <QUrl>
#include <QtDBus> 

#include "moled.h"
#include "localizer.h"

class WhereAmI : public QObject {
  Q_OBJECT

public:
  WhereAmI(QObject *parent = 0);
  ~WhereAmI ();


private slots:
  void post_location();
  void post_response();
  void handle_whereami_update ();

private:
  QTimer *post_timer;
  QNetworkReply *reply;

};



#endif /* WHEREAMI_H_ */
