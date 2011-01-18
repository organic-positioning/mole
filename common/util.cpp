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

#include "util.h"

/*
int rand_int (int a, int b) {
  //return qrand() % ((high +1) - low) + low;
  
  //ASSERT (a >= 0 && b >= 0);
  int diff = ABS ((a-b));
  int randValue = (int)(Rand()*(double)diff);
  return (int)(a + randValue);

}
*/

int rand_int (int low, int high) {
  return qrand() % ((high+1) - low) + low;
}


double rand_pct () {
  double ret = qrand () / (double) RAND_MAX;
  //assert (ret >= 0. && ret < 1);
  //qDebug () << "rand_pct " << ret;
  return ret;
}


double rand_poisson (double mean) {
  double u;
  while ((u = rand_pct()) == 0.);
  return (-mean) * log(u);
};

int rand_poisson (int mean) {
  return (int)(rand_poisson((double)mean));
}

