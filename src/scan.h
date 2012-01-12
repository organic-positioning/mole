/*
 * Mole - Mobile Organic Localisation Engine
 * Copyright 2010-2012 Nokia Corporation.
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

#ifndef SCAN_H_
#define SCAN_H_

#include <QtCore>

#include "sig.h"

class APDesc : public Sig
{
 friend QDebug operator<<(QDebug dbg, const APDesc &apDesc);

 public:
  APDesc(QString _mac, QString _ssid, qint16 _frequency)
    : mac(_mac), ssid(_ssid), frequency(_frequency), m_count(0) {}

  const QString mac;
  const QString ssid;
  const qint16 frequency;
  void incrementUse() { ++m_count; }
  void decrementUse() { --m_count; Q_ASSERT(m_count >= 0); }
  qint16 useCount() const { return m_count; }

private:
  qint16 m_count;

};

#endif
