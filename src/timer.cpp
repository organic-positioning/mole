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

#include "timer.h"
#include "math.h"

#include <QDebug>

AdjustingTimer::AdjustingTimer(int _minPeriod, int _maxPeriod, QObject *parent)
  : QTimer(parent)
  , minPeriod(_minPeriod)
  , maxPeriod(_maxPeriod)
{
  m_period = _minPeriod;
  setInterval(m_period);
}

AdjustingTimer::AdjustingTimer(QObject *parent)
  : QTimer(parent)
  , minPeriod(10000)
  , maxPeriod(3600000)
{
  m_period = minPeriod;
  setInterval(m_period);
}

void AdjustingTimer::restart(bool faster)
{
  if (faster) {
    m_period = m_period / 2;
  } else {
    m_period = m_period * 4;
  }

  qDebug() << "faster " << faster << " target " << m_period;

  if (m_period > maxPeriod)
    m_period = maxPeriod;

  if (m_period < minPeriod)
    m_period = minPeriod;

  int thisPeriod = randPoisson(m_period);
  qDebug() << "restartTimer " << thisPeriod << " target " << m_period;
  start(thisPeriod);
}
