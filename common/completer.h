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

#ifndef COMPLETER_H_
#define COMPLETER_H_

#include <QtCore>
#include <QCompleter>


class MultiCompleter : public QCompleter {
  Q_OBJECT
  Q_PROPERTY(QString separator READ separator WRITE setSeparator)
  

 public:
  MultiCompleter(QObject *parent = 0);
  MultiCompleter(QAbstractItemModel *model, QObject *parent = 0);

  void setSeparator (QLatin1String _sep);
  QString separator() const;

 public slots:
  void setSeparator(const QString &separator);

 protected:
  QStringList splitPath(const QString &path) const;
  QString pathFromIndex(const QModelIndex &index) const;

 private:
  QString sep;

};

#endif /* COMPLETER_H_ */
