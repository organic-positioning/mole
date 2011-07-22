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

#include <QDateTime>
#include <QNetworkReply>
#include <QStandardItemModel>
#include <QUrl>

class AdjustingTimer;

// Periodically update the model using the url.
// Assumes that the data from the url contains a list of entries.
// The url is changeable and results from urls are cached.

class UpdatingModel : public QStandardItemModel
{
  Q_OBJECT

 public:
  UpdatingModel(const QUrl& url = QUrl(), QObject *parent = 0);
  UpdatingModel(QObject *parent = 0);

 protected:
  bool dirty;
  bool online;
  QUrl url;
  AdjustingTimer *timer;
  QNetworkReply *reply;
  QDateTime lastModified;
  QMap<QUrl,QList<QString>*> *url2listCache;

  void updateFromNetwork();
  void setUrl(const QUrl& _url);
  void stopRefill();
  void initialize();

  void emitNetworkState(bool _online);

 signals:
  void networkChange(bool online);

 protected slots:
  void startRefill();
  void finishRefill();
};

// load up a model from a file, with one entry per line
// and
// periodically update the model using the file at the url
class UpdatingFileModel : public UpdatingModel
{
  Q_OBJECT

 public:
  UpdatingFileModel(const QString& fileName = 0, const QUrl& url = QUrl(), QObject *parent = 0);

 protected slots:
  void finishRefill();

 protected:
  const QString fileName;
  void loadModelFromFile();
  void saveModelToFile();
};

#endif /* MODELS_H_ */
