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

#ifndef MODELS_H_
#define MODELS_H_

#include <QtCore>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QStandardItemModel>

#include "mole.h"
#include "timer.h"

// Periodically update the model using the url.
// Assumes that the data from the url contains a list of entries.
// The url is changeable and results from urls are cached.

class UpdatingModel : public QStandardItemModel {
  Q_OBJECT

 public:
  UpdatingModel(const QUrl& url = QUrl(), QObject *parent = 0);
  UpdatingModel(QObject *parent = 0);

 protected:
  bool dirty;
  bool online;
  QUrl url;
  AdjustingTimer *timer;
  void updateFromNetwork ();
  QNetworkReply *reply;
  QDateTime last_modified;
  QMap<QUrl,QList<QString>*> *url2list_cache;

  void set_url (const QUrl& _url);
  void stop_refill ();
  void initialize ();
  void emit_network_state (bool _online);

 signals:
  void network_change (bool online);

 protected slots:
  void start_refill ();
  void finish_refill ();

};

// load up a model from a file, with one entry per line
// and
// periodically update the model using the file at the url
class UpdatingFileModel : public UpdatingModel {
  Q_OBJECT

 public:
  UpdatingFileModel(const QString& fileName = 0, const QUrl& url = QUrl(), QObject *parent = 0);

 protected slots:
  void finish_refill ();

 protected:
  const QString fileName;
  void loadModelFromFile ();
  void saveModelToFile ();

};


#endif /* MODELS_H_ */
