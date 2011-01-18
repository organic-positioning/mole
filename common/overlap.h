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

#include <QtCore>

#include "sig.h"

// Overlap object is used to compare scans in a few places.
// Each instance might keep its own data.
// In addition, they will share a cache of overlap computations.

class Overlap {

public:
  Overlap();
  ~Overlap ();

  double compare_sig_overlap (const QMap<QString,Sig*> *sig_a, 
			      const QMap<QString,Sig*> *sig_b);

  double compute_overlap
    (double mean_1, double sigma_1, double mean_2, double sigma_2);

  double ncdf (double x);

  void intersection 
    (double mean_1, double sigma_1, double mean_2, double sigma_2,
     double &x1_ret, double &x2_ret);
 
  double erfcc (double x);


};


#endif /* OVERLAP_H_ */
