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


#include "qt-utils.h"

HintedLineEdit::HintedLineEdit(QString _hint, QWidget *parent)
  : QLineEdit(parent)
  , m_hint(_hint)
  , m_touched(false)
{
  setText(_hint);
}

void HintedLineEdit::reset()
{
  m_touched = false;
  setText(m_hint);
}

void HintedLineEdit::focusInEvent(QFocusEvent *e)
{
  qDebug() << "focusInEvent";
  if (!m_touched) {
    clear();
    m_touched = true;
  }
  QLineEdit::focusInEvent(e);
}

ColoredLabel::ColoredLabel(QString _text, QWidget *parent)
  : QLabel(_text, parent)
  , m_level(1)
{
  setColor();
}

void ColoredLabel::setLevel(int _level)
{
  if (_level > 0)
    _level = 1;

  if (_level < 0)
    _level = -1;

  if (_level != m_level) {
    m_level = _level;
    setColor();
  }
}

void ColoredLabel::setColor()
{
  switch (m_level) {
  case 1:
    setStyleSheet("QLabel { color: green }");
    break;
  case 0:
    setStyleSheet("QLabel { color: yellow }");
    break;
  case -1:
    setStyleSheet("QLabel { color: red }");
    break;
  }
}
