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

#ifndef LOCAL_SERVER_H
#define LOCAL_SERVER_H

#include "moled.h"

#include <QTcpServer>
#include <QVariant>

class Binder;
class Localizer;

class LocalServer : public QTcpServer
{
  Q_OBJECT

 public:
  LocalServer(QObject *parent = 0, Localizer *localizer = 0, Binder *binder = 0, int port = DEFAULT_LOCAL_PORT);
  ~LocalServer ();

 private:
  Localizer *m_localizer;
  Binder *m_binder;

  QVariantMap handleRequest(QByteArray &rawJson, bool &monitor);
  QVariantMap handleBind(QVariantMap &params, QString source);
  QVariantMap handleStats(QVariantMap &params);
  QVariantMap handleQuery(QVariantMap &params);
  QVariantMap handleMonitor(QVariantMap &params);

 private slots:
  void handleRequest();

};

#endif // LOCAL_SERVER_H
