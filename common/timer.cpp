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


AdjustingTimer::AdjustingTimer(int _min_period, int _max_period, QObject *parent) : QTimer (parent), min_period (_min_period), max_period (_max_period) {
  period = min_period;
  setInterval (period);
}

//AdjustingTimer::AdjustingTimer(QObject *parent = 0) : QTimer (parent), min_period (10000), max_period (3600000) {
AdjustingTimer::AdjustingTimer(QObject *parent) : QTimer (parent), min_period (10000), max_period (3600000)  {
  period = min_period;
  setInterval (period);
}

void AdjustingTimer::restart (bool faster) {
  if (faster) {
    period = period / 2;
  } else {
    period = period * 4;

  }

  qDebug () << "faster " << faster << " target " << period;

  if (period > max_period) {
    period = max_period;
  }
  if (period < min_period) {
    period = min_period;
  }
  int this_period = rand_poisson (period);
  qDebug () << "restart_timer " << this_period << " target " << period;
  start (this_period);
}
