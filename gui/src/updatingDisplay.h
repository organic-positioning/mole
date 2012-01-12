/*
 * Mole - Mobile Organic Localisation Engine
 * Copyright 2010-2011 Nokia Corporation.
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

#ifndef UPDATING_DISPLAY_H
#define UPDATING_DISPLAY_H

#include <QDialog>
#include <QStandardItemModel>

#include "common.h"

class UpdatingDisplayEntry {

 public:
 UpdatingDisplayEntry(QString name, double score) : m_name(name), m_score(score) {}
  QString name() const { return m_name; }
  double score() const { return m_score; }

 private:
  QString m_name;
  double m_score;

};

class UpdatingDisplayModel : public QAbstractTableModel {

  Q_OBJECT

 public:
  UpdatingDisplayModel(QObject *parent = 0, QStringList headerNames = QStringList());
  
  int rowCount(const QModelIndex &parent) const;
  int columnCount(const QModelIndex &parent) const;
  QVariant data (const QModelIndex &index, int role) const;
  QVariant headerData (int selection, Qt::Orientation orientation,
		       int role) const;
  void handleUpdate(QByteArray &rawJson);
  void handleUpdate(QVariantMap &update);

 private:
  QStringList m_headerNames;
  QList<UpdatingDisplayEntry> m_entries;
};



class UpdatingDisplayProxyModel : public QSortFilterProxyModel {
 Q_OBJECT

 public:

  UpdatingDisplayProxyModel(QObject *parent = 0);

 protected:
  
  bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

 private:

};


class UpdatingDisplayDialog : public QDialog
{
  Q_OBJECT

public:
  UpdatingDisplayDialog(QWidget *parent = 0, QString title = QString(), QStringList headerNames = QStringList());
  ~UpdatingDisplayDialog();

  void handleUpdate(QByteArray &rawJson);
  void handleUpdate(QVariantMap &update);

private:
  void buildUI(QStringList headerNames);
 
  QVBoxLayout *m_layout;
  QTableView *m_view;
  UpdatingDisplayModel *m_model;
  UpdatingDisplayProxyModel *m_proxyModel;

private slots:

};

#endif /* UPDATING_DISPLAY_H */
