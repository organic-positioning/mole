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

#ifndef OVERLAP_H_
#define OVERLAP_H_

#include <QMap>
#include <QString>

class Histogram;
class Sig;

// Overlap object is used to compare scans in a few places.
// Each instance might keep its own data.
// In addition, they will share a cache of overlap computations.
class Overlap
{
 public:
  Overlap() {};
  ~Overlap() {};

  double compareSigOverlap(const QMap<QString,Sig*> *sigA,
                           const QMap<QString,Sig*> *sigB);

  double compareHistOverlap(const QMap<QString,Sig*> *sigA,
                            const QMap<QString,Sig*> *sigB, int penalty);

  double computeOverlap(double mean1, double sigma1,
                        double mean2, double sigma2);

  double computeHistOverlap(Histogram *hist1, Histogram *hist2);
  double ncdf(double x);
  void intersection(double mean1, double sigma1,
                    double mean2, double sigma2,
                    double &x1Ret, double &x2Ret);

  double erfcc(double x);
};

#endif /* OVERLAP_H_ */
