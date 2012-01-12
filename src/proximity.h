/*
 * Mole - Mobile Organic Localisation Engine
 * Copyright 2010-2012 Nokia Corporation.
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

#ifndef PROXIMITY_H_
#define PROXIMITY_H_

#include "localizer.h"
#include "moled.h"

#include <qjson/parser.h>
#include <qjson/serializer.h>

#include <QNetworkReply>
#include <QNetworkRequest>

class Proximity : public QObject
{
  //friend QDebug operator<<(QDebug dbg, const Proximity &proximity);
  Q_OBJECT

 public:
  Proximity(QObject *parent = 0, Localizer *localizer = 0);


 private:
  Localizer *m_localizer;
  bool m_active;
  QString m_name;
  QTimer m_updateTimer;

  void setTimer ();

 private slots:
  void update();
  void handleUpdateResponse();
  void handleProximitySettings(bool activate, QString name);
};

#endif /* PROXIMITY_H_ */
