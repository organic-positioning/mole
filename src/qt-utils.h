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

#ifndef QT_UTILS_H_
#define QT_UTILS_H_

#include <QtGui>

class HintedLineEdit : public QLineEdit
{
 public:
  HintedLineEdit(QString hint, QWidget *parent = 0);

  void reset();

 protected:
  void focusInEvent(QFocusEvent *e);

 private:
  const QString m_hint;
  bool m_touched;

};

class ColoredLabel : public QLabel
{
 public:
  ColoredLabel(QString text, QWidget *parent = 0);
  void setLevel(int level);

 private:
  int m_level;

  void setColor();

};

#endif /* QT_UTILS_H_ */
