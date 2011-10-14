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

#include "updatingDisplay.h"



UpdatingDisplayDialog::UpdatingDisplayDialog
(QWidget *parent, QString title, QStringList headerNames)
  : QDialog(parent)
{
  buildUI(headerNames);

  setWindowTitle(title);
}

UpdatingDisplayDialog::~UpdatingDisplayDialog()
{
  //qDebug() << "UpdatingDisplay: destructor";
  QLayoutItem *child;
  while (!m_layout->isEmpty()) {
    child = m_layout->takeAt(0);
    m_layout->removeItem(child);
    delete child;
  }
  delete m_layout;
}

void UpdatingDisplayDialog::buildUI(QStringList headerNames)
{
  setStyleSheet("QToolButton { border: 0px; padding 0px; }");

  //qDebug () << "UpdatingDisplayDialog::buildUI";

  m_model = new UpdatingDisplayModel (this, headerNames);

  m_proxyModel = new UpdatingDisplayProxyModel (this);
  m_proxyModel->setDynamicSortFilter (true);

  m_view = new QTableView (this);
  m_view->setSortingEnabled (true);
  m_view->sortByColumn (0);
  m_view->sortByColumn (1);

  m_view->setShowGrid(true);
  m_view->setGridStyle(Qt::DotLine);

  m_proxyModel->setSourceModel (m_model);
  m_view->setModel(m_proxyModel);
  //m_view->setModel(m_model);

  //m_view->setSelectionMode(QAbstractItemView::NoSelection);

  m_layout = new QVBoxLayout ();
  m_layout->addWidget (m_view);
  setLayout(m_layout);
  m_view->resizeColumnsToContents();

  //qDebug () << "UpdatingDisplayDialog::buildUI finished";
}

UpdatingDisplayModel::UpdatingDisplayModel(QObject *parent, QStringList headerNames) : QAbstractTableModel(parent), m_headerNames (headerNames) {
}

UpdatingDisplayProxyModel::UpdatingDisplayProxyModel(QObject *parent) :
  QSortFilterProxyModel (parent) {
}

bool UpdatingDisplayProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {

  /*
  QModelIndex index1 = sourceModel()->index(sourceRow, 1, sourceParent);
  double score = sourceModel()->data(index1).toDouble();

  qDebug () << "score " << score
            << " min " << min_score_to_display;


  if (score >= min_score_to_display) {
    return true;
  }
  return false;
  */
  return true;
}


void UpdatingDisplayDialog::handleUpdate(QByteArray &rawJson) {
  m_model->handleUpdate(rawJson);
  m_view->resizeColumnsToContents();
}

void UpdatingDisplayDialog::handleUpdate(QVariantMap &update) {
  m_model->handleUpdate(update);
  m_view->resizeColumnsToContents();
}

void UpdatingDisplayModel::handleUpdate(QVariantMap &update) {
  int currentSize = m_entries.size();
  m_entries.clear();

  for (QVariantMap::Iterator it = update.begin(); 
       it != update.end(); ++it) {
    //qDebug() << "P: it";
    QString name = it.key();
    double score = it.value().toDouble();
    m_entries.append(UpdatingDisplayEntry(name, score));
    //qDebug () << "P nearby: " << name << score;
  }

  int sizeDelta = m_entries.size() - currentSize;
  //qDebug() << "P: update sizeDelta" << sizeDelta;
  if (sizeDelta < 0) {
    beginRemoveRows(QModelIndex(), m_entries.size(), currentSize);
    endRemoveRows();
  } else if (sizeDelta > 0) {
    beginInsertRows(QModelIndex(), currentSize, currentSize+sizeDelta-1);
    endInsertRows();
  }

  QModelIndex upper_left = createIndex (0, 0);
  QModelIndex lower_right = createIndex (m_entries.size(), 2);

  emit dataChanged(upper_left, lower_right);
}

void UpdatingDisplayModel::handleUpdate(QByteArray &rawJson) {
  if (rawJson.isEmpty()) {
    qDebug () << "P: invalid json (empty)";
    return;
  }
  QJson::Parser parser;
  bool ok;
  QVariantMap response = parser.parse(rawJson, &ok).toMap();

  //qDebug() << "P: handleUpdate (model)";

  if (!ok) {
    qWarning() << "P: failed to parse json" << rawJson;
    return;
  }

  //qDebug() << "P: clearing";

  handleUpdate(response);
  //qDebug() << "P: cleared";
  //qDebug () << "P: dataChanged size" << m_entries.size();
}

int UpdatingDisplayModel::rowCount(const QModelIndex & /*parent*/) const {
  //qDebug () << "P: rowCount" << m_entries.size();
  return m_entries.size();
}

int UpdatingDisplayModel::columnCount(const QModelIndex & /*parent*/) const {
  return 2;
}

QVariant UpdatingDisplayModel::headerData
(int section, Qt::Orientation orientation, int role) const {

  if (role == Qt::TextAlignmentRole) {
    return int (Qt::AlignCenter);
  }

  if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
    //qDebug() << "section role is display and horizontal";
    if (section >= m_headerNames.size()) {
      return QVariant();
    }
    return m_headerNames.at(section);
  }

  return QVariant();
}


QVariant UpdatingDisplayModel::data (const QModelIndex &index, int role) const {
   qDebug () << "P: model data row=" << index.row()
	     << " col=" << index.column()
	     << " role=" << role;

  if (!index.isValid())
    return QVariant();

  if (role == Qt::TextAlignmentRole) {
    return int (Qt::AlignLeft);
    //return int (Qt::AlignLeft | Qt::AlignCenter);

  } else if (role == Qt::BackgroundRole) {

    double score = m_entries.at(index.row()).score();

    if (score >= 0.1) {
      QColor color ("#B22222");
      return color;
    } else if (score >= 0.01) {
      QColor color ("#E47833");
      return color;
    } else if (score >= 0.000001) {
      QColor color ("#FAEBD7");
      return color;
    }


  } else if (role == Qt::DisplayRole) {

    if (index.column() == 1) {
      return m_entries.at(index.row()).score();
    } else if (index.column() == 0) {
      return m_entries.at(index.row()).name();
    } else {
      qDebug() << "unknown column " << index.column();
    }
  }

  return QVariant();
}


