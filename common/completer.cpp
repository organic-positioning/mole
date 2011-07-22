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

#include "completer.h"

#include <QStringList>

MultiCompleter::MultiCompleter(QObject *parent)
  : QCompleter(parent)
  , m_sep(QLatin1String(""))
{
}

QStringList MultiCompleter::splitPath(const QString &path) const
{
  if (m_sep.isNull())
    return QCompleter::splitPath(path);

  return path.split(m_sep);
}

QString MultiCompleter::pathFromIndex(const QModelIndex &index) const
{
  if (m_sep.isNull())
    return QCompleter::pathFromIndex(index);

  // navigate up and accumulate data
  QStringList dataList;
  for (QModelIndex i = index; i.isValid(); i = i.parent())
    dataList.prepend(model()->data(i, completionRole()).toString());

  return dataList.join(m_sep);
}
