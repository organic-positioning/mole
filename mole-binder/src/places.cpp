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

#include "places.h"

// yes, this could be done recursively...
QVector<QString> places(6);

LineValidator::LineValidator(QObject *parent)
  : QRegExpValidator(parent)
{
}

QValidator::State LineValidator::validate(QString &input, int &pos) const
{
  qDebug () << "validate " << input << " pos " << pos;

  if (input.contains ('/')) {
    return QValidator::Invalid;
  }
  return QValidator::Acceptable;
}

void LineValidator::fixup(QString &input) const
{
  input.replace(QChar('/'), QChar());
}

PlaceEdit::PlaceEdit(PlaceEdit *_parent, int _index, QString placeholderText, QWidget *qparent)
  : QLineEdit(qparent)
{
  qDebug() << "creating place edit";

  setMaxLength(40);

  setPlaceholderText(placeholderText);

  m_model = new PlaceModel(_index, this);
  m_completer = new QCompleter(m_model, this);
  m_completer->setCaseSensitivity(Qt::CaseInsensitive);

  // TODO put in mask
  // TODO prevent slashes

  m_completer->setModel(m_model);
  setCompleter(m_completer);

  setValidator(new LineValidator(this));

  if (_parent) {
    connect(_parent->model(), SIGNAL(valueChanged(QString)), m_model, SLOT(onParentTextChanged(QString)));
    connect(_parent->model(), SIGNAL(valueChanged(QString)), this, SLOT(onParentTextChanged(QString)));
  }
}

void PlaceEdit::onParentTextChanged(QString text)
{
  qDebug () << "place_edit: handle parent text changed " << text;

  setText ("");
}

PlaceModel::PlaceModel(int _index, QObject *parent)
  : UpdatingModel(parent)
  , m_index(_index)
  , m_settingsName("places-")
{
  qDebug() << "creating area model index" << _index;

  m_settingsName.append(QString::number(m_index));

  qDebug() << "settings name "<< m_settingsName;

  if (m_index == 0) {
    if (!settings->contains(m_settingsName)) {
      settings->setValue(m_settingsName, "USA");
    }
  }

  qDebug() << "A";

  if (settings->contains(m_settingsName)) {
    qDebug() << "B";
    places[m_index] = settings->value(m_settingsName).toString();

  } else {
    qDebug() << "C";
    places[m_index] = "";
  }

  qDebug() << "index " << m_index << " places " << places[m_index];

  // TODO memory leak?
  appendRow(new QStandardItem(places[m_index]));

  setUrl();
}

PlaceModel::~PlaceModel()
{
}

void PlaceModel::setUrl()
{
  for (int i = 0; i < m_index; ++i) {
    if (places[i].isEmpty()) {
      qDebug() << "places " << m_index << " not setting url "
               << "with empty parent " << i;
      stopRefill();
      return;
    }
  }

  QString urlStr(staticServerURL);
  urlStr.append("/map/");

  for (int i = 0; i < m_index; ++i) {
    urlStr.append(places[i]);
    urlStr.append("/");
  }
  urlStr.append("places.txt");

  qDebug() << "place url " << urlStr;

  UpdatingModel::setUrl(QUrl(urlStr));
}

void PlaceModel::onTextChanged(QString text)
{
  qDebug() << "index " << m_index << " handle_text_changed " << text;

  places[m_index] = text;
  settings->setValue(m_settingsName, places[m_index]);

  // add this new place into the list associated with our current parent
  setUrl();

  emit valueChanged(text);
}

void PlaceModel::onParentTextChanged(QString text)
{
  qDebug() << "index " << m_index << " handle parent text changed " << text;

  places[m_index] = "";
  settings->setValue(m_settingsName, places[m_index]);

  setUrl();

  emit valueChanged("");
}
