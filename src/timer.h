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

#ifndef TIMER_H_
#define TIMER_H_

#include <QTimer>

class AdjustingTimer : public QTimer
{
  Q_OBJECT

 public:
  AdjustingTimer(int _minPeriod, int _maxPeriod, QObject *parent = 0);
  AdjustingTimer(QObject *parent = 0);

  const int minPeriod;
  const int maxPeriod;

  void restart(bool faster);

 private:
  int m_period;
};

#endif /* TIMER_H_ */
