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

UpdatingFileModel::UpdatingFileModel 
(const QString& _fileName, const QUrl& _url, QObject *parent) :
  UpdatingModel (_url, parent), fileName (_fileName) {

  loadModelFromFile ();
}



UpdatingModel::UpdatingModel 
(const QUrl& _url, QObject *parent) :
  QStandardItemModel (parent) {
  initialize ();
  set_url (_url);
}

UpdatingModel::UpdatingModel 
(QObject *parent) : QStandardItemModel (parent) {
  initialize ();
}

void UpdatingModel::initialize () {
  online = true;
  timer = new AdjustingTimer (this);
  connect (timer, SIGNAL(timeout()), SLOT(start_refill()));
  url2list_cache = new QMap<QUrl,QList<QString>*> () ;
  dirty = true;
}

void UpdatingModel::set_url
(const QUrl& _url) {

  qDebug () << "model set_url " 
	    << "old " << url	    << "new " << _url;

  if (_url != url) {
  
    url = _url;

    qDebug () << "cleared model";
    clear();

    if (settings->contains(url.toString())) {
      last_modified = settings->value(url.toString()).toDateTime();
    }

    // attempt to fill from cache
    if (url2list_cache->contains(url)) {
      qDebug () << "pulling from cache " << url;

      QList<QString> *item_list = url2list_cache->value(url);
      QListIterator<QString> i (*item_list);
      while (i.hasNext()) {
	appendRow (new QStandardItem (i.next()));
      }
      sort (0);

    } else {
      // fire the timer now
      timer->start (50);
    }

  }
}


void UpdatingModel::stop_refill () {
  qDebug () << "stop_refill " << url;
  timer->stop ();
}

void UpdatingModel::start_refill () {
  qDebug () << "start_refill " << url;
  timer->stop ();

  QNetworkRequest request;
  request.setUrl (url);
  reply = network_access_manager->get (request);
  connect (reply, SIGNAL(finished()), 
	   SLOT (finish_refill()));
}

void UpdatingFileModel::finish_refill () {
  UpdatingModel::finish_refill ();
  if (dirty) {
    // note: I/O access
    saveModelToFile ();
    dirty = false;
  }
}

void UpdatingModel::emit_network_state (bool _online) {
  if (online != _online) {
    online = _online;
    this->network_change (online);
  }
}

// assumes models are not very long, as we just do a linear insertion
void UpdatingModel::finish_refill () {

  dirty = false;
  reply->deleteLater ();

  if (reply->error() != QNetworkReply::NoError) {
    qWarning() << "finish_refill request failed " << reply->errorString();
    timer->restart (false);
    emit_network_state (false);
    return;
  }


  qDebug() << "finish_refill request succeeded";
  QDateTime modified = 
    (reply->header(QNetworkRequest::LastModifiedHeader)).toDateTime();

  if (modified != last_modified ||
      !url2list_cache->contains(reply->url())) {
    last_modified = modified;

    // TODO might be a problem is something is replaced
    // when the user has an item selected

    const int buffer_size = 256;

    QList<QString> *item_list = new QList<QString>();

    /*
    QString blank = "";
    QList<QStandardItem*> items = findItems (blank);
    if (items.size() == 0) {
      appendRow (new QStandardItem (blank));
      item_list->append (blank);
      qDebug () << "appended blank";
    }
    */

    QByteArray data = reply->readLine(buffer_size);
    while  (!data.isEmpty()) {
      QString item_name (data);
      item_name = item_name.trimmed();

      QList<QStandardItem*> items = findItems (item_name);
      if (items.size() == 0) {
	appendRow (new QStandardItem (item_name));
	item_list->append (item_name);
	qDebug () << "appended " << item_name;
      }

      qDebug() << "refill item_name " << item_name;

      data = reply->readLine(buffer_size);
    }

    sort (0);

    if (url2list_cache->contains(reply->url())) {
      QList<QString> *old_item_list = 
	url2list_cache->value(reply->url());
	delete old_item_list;
      }
    url2list_cache->insert (reply->url(), item_list);
    qDebug () << " insert into cache " << reply->url() 
	      << " size " << item_list->size();

    dirty = true;

    settings->setValue(url.toString(), last_modified);
    timer->restart (true);
  } else {
    qDebug () << "finish_refill unchanged";
    timer->restart (false);
  }

  emit_network_state (true);

}

void UpdatingFileModel::loadModelFromFile ()
 {
     QFile file(fileName);
     if (!file.open(QFile::ReadOnly)) {
       qWarning () << "Cannot load model from file" << fileName;
       return;
     }

     // #ifndef QT_NO_CURSOR
     //     QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
     //#endif
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
         } else {
             if (line.startsWith("\t")) {
                 level = re.cap(0).length();
             } else {
                 level = re.cap(0).length()/4;
             }
         }

         if (level+1 >= parents.size())
             parents.resize(parents.size()*2);

         QStandardItem *item = new QStandardItem;
         item->setText(trimmedLine);
         parents[level]->appendRow(item);
         parents[level+1] = item;
     }

     // #ifndef QT_NO_CURSOR
     //QApplication::restoreOverrideCursor();
     // #endif

 }


// assumes model is not modified while we are writing it out
void UpdatingFileModel::saveModelToFile ()
 {
     QFile file(fileName);
     if (!file.open(QFile::WriteOnly | QIODevice::Text)) {
       qWarning () << "Cannot load model from file" << fileName;
       return;
     }
     QTextStream out (&file);

     QStandardItem* root = invisibleRootItem();
     int r = root->rowCount();
     for (int i = 0; i < r; i++) {
       QStandardItem* it = root->child (i);
       out << it->data(Qt::DisplayRole).toString() << "\n";
     }

     file.close ();

 }
