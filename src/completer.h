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

#include <QCompleter>

class MultiCompleter : public QCompleter
{

public:
  MultiCompleter(QObject *parent = 0);

  QString separator() const { return m_sep; }
  void setSeparator(QLatin1String separator) { m_sep = separator; }

protected:
  QStringList splitPath(const QString &path) const;
  QString pathFromIndex(const QModelIndex &index) const;

private:
  QString m_sep;

};

#endif /* COMPLETER_H_ */
