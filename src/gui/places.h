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

#ifndef PLACES_H_
#define PLACES_H_

#include <QRegExpValidator>

#include "common.h"

class LineValidator : public QRegExpValidator
{
  Q_OBJECT

 public:
  LineValidator(QObject *parent);

  QValidator::State validate(QString &, int &) const;
  void fixup(QString &) const;
};

class PlaceModel : public UpdatingModel
{
  Q_OBJECT

 public:
  PlaceModel(int index = -1, QObject *parent = 0);
  ~PlaceModel();

 public slots:
  void onParentTextChanged(QString);
  void onTextChanged(QString);

 signals:
  void valueChanged(QString);

 private:
  const int m_index;
  QString m_settingsName;

  void setUrl();
};

class PlaceEdit : public QLineEdit
{
  Q_OBJECT

 public:
  PlaceEdit(PlaceEdit *_parent, int _index, QString placeholder_text, QWidget *qparent = 0);
  PlaceModel* model() const { return m_model; }

 public slots:
  void onParentTextChanged(QString);

 private:
  QTimer *m_timer;
  PlaceModel *m_model;
  QCompleter *m_completer;
};

#endif /* PLACES_H_ */
