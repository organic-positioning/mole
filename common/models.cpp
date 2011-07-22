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

#include "models.h"

#include "mole.h"
#include "timer.h"

#include <QNetworkRequest>

UpdatingFileModel::UpdatingFileModel(const QString& _fileName, const QUrl& _url, QObject *parent)
  : UpdatingModel(_url, parent)
  , fileName(_fileName)
{
  loadModelFromFile();
}

UpdatingModel::UpdatingModel(const QUrl& _url, QObject *parent)
  : QStandardItemModel(parent)
{
  initialize();
  setUrl(_url);
}

UpdatingModel::UpdatingModel(QObject *parent)
  : QStandardItemModel(parent)
{
  initialize();
}

void UpdatingModel::initialize()
{
  online = true;
  timer = new AdjustingTimer(this);
  connect(timer, SIGNAL(timeout()), SLOT(startRefill()));
  url2listCache = new QMap<QUrl,QList<QString>*> () ;
  dirty = true;
}

void UpdatingModel::setUrl(const QUrl& _url)
{
  qDebug() << "model set_url " << "old " << url << "new " << _url;

  if (_url != url) {
    url = _url;
    qDebug() << "cleared model";
    clear();

    if (settings->contains(url.toString()))
      lastModified = settings->value(url.toString()).toDateTime();

    // attempt to fill from cache
    if (url2listCache->contains(url)) {
      qDebug() << "pulling from cache " << url;

      QList<QString> *itemList = url2listCache->value(url);
      QListIterator<QString> i (*itemList);
      while (i.hasNext()) {
        appendRow(new QStandardItem(i.next()));
      }
      sort(0);

    } else {
      // fire the timer now
      timer->start(50);
    }
  }
}

void UpdatingModel::stopRefill()
{
  qDebug() << "stopRefill " << url;
  timer->stop();
}

void UpdatingModel::startRefill()
{
  qDebug() << "startRefill " << url;
  timer->stop();

  QNetworkRequest request;
  request.setUrl(url);
  qDebug() << "url " << url << "net " << networkAccessManager;
  reply = networkAccessManager->get(request);
  connect(reply, SIGNAL(finished()), SLOT(finishRefill()));
}

void UpdatingFileModel::finishRefill()
{
  UpdatingModel::finishRefill();
  if (dirty) {
    // note: I/O access
    saveModelToFile();
    dirty = false;
  }
}

void UpdatingModel::emitNetworkState(bool _online)
{
  if (online != _online) {
    online = _online;
    networkChange(online);
  }
}

// assumes models are not very long, as we just do a linear insertion
void UpdatingModel::finishRefill()
{
  dirty = false;
  reply->deleteLater();

  if (reply->error() != QNetworkReply::NoError) {
    qWarning() << "finishRefill request failed " << reply->errorString();
    timer->restart(false);
    emitNetworkState(false);
    return;
  }

  qDebug() << "finishRefill request succeeded";
  QDateTime modified = (reply->header(QNetworkRequest::LastModifiedHeader)).toDateTime();

  if (modified != lastModified || !url2listCache->contains(reply->url())) {
    lastModified = modified;

    // TODO might be a problem is something is replaced
    // when the user has an item selected

    const int bufferSize = 256;

    QList<QString> *itemList = new QList<QString>();

    QByteArray data = reply->readLine(bufferSize);
    while (!data.isEmpty()) {
      QString itemName(data);
      itemName = itemName.trimmed();

      QList<QStandardItem*> items = findItems(itemName);
      if (items.isEmpty()) {
        appendRow(new QStandardItem(itemName));
        itemList->append(itemName);
        qDebug() << "appended " << itemName;
      }

      qDebug() << "refill itemName " << itemName;
      data = reply->readLine(bufferSize);
    }

    sort(0);

    if (url2listCache->contains(reply->url())) {
      QList<QString> *oldItemList = url2listCache->value(reply->url());
      delete oldItemList;
    }
    url2listCache->insert(reply->url(), itemList);
    qDebug() << " insert into cache " << reply->url() << " size " << itemList->size();

    dirty = true;

    settings->setValue(url.toString(), lastModified);

    timer->restart(true);
  } else {
    qDebug() << "finishRefill unchanged";
    timer->restart(false);
  }

  emitNetworkState(true);

}

void UpdatingFileModel::loadModelFromFile()
{
  QFile file(fileName);
  if (!file.open(QFile::ReadOnly)) {
    qWarning() << "Cannot load model from file" << fileName;
    return;
  }

  QStringList words;

  QVector<QStandardItem *> parents(10);
  parents[0] = invisibleRootItem();

  while (!file.atEnd()) {
    QString line = file.readLine();
    QString trimmedLine = line.trimmed();
    if (line.isEmpty() || trimmedLine.isEmpty())
      continue;

    QRegExp re("^\\s+");
    int nonws = re.indexIn(line);
    int level = 0;
    if (nonws == -1) {
      level = 0;
    } else if (line.startsWith("\t")) {
             level = re.cap(0).length();
    } else {
             level = re.cap(0).length() / 4;
    }

    if ((level + 1) >= parents.size())
      parents.resize(parents.size() * 2);

    QStandardItem *item = new QStandardItem;
    item->setText(trimmedLine);
    parents[level]->appendRow(item);
    parents[level + 1] = item;
  }
}

// assumes model is not modified while we are writing it out
void UpdatingFileModel::saveModelToFile()
{
  QFile file(fileName);
  if (!file.open(QFile::WriteOnly | QIODevice::Text)) {
    qWarning() << "Cannot load model from file" << fileName;
    return;
  }

  QTextStream out(&file);

  QStandardItem* root = invisibleRootItem();
  int r = root->rowCount();
  for (int i = 0; i < r; ++i) {
    QStandardItem* it = root->child(i);
    out << it->data(Qt::DisplayRole).toString() << "\n";
  }

  file.close();
}
