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

#include "math.h"

#include <math.h>
#include <stdlib.h>

#include <QTime>

int randInt(int low, int high)
{
  return qrand() % ((high + 1) - low) + low;
}

double randPct()
{
  double ret = qrand() / (double) RAND_MAX;
  return ret;
}

double randPoisson(double mean)
{
  double u;
  while ((u = randPct()) == 0.);

  return (-mean) * log(u);
};

int randPoisson(int mean)
{
  return (int)(randPoisson((double)mean));
}


double erfcc(double x)
{
  double z = qAbs(x);
  double t = 1.0 / (1.0 + 0.5*z);

  double r = t * exp(-z*z-1.26551223+t*(1.00002368+t*(.37409196+
               t*(.09678418+t*(-.18628806+t*(.27886807+
               t*(-1.13520398+t*(1.48851587+t*(-.82215223+
               t*.17087277)))))))));
  if (x >= 0.)
    return r;
  else
    return (2. - r);
}

