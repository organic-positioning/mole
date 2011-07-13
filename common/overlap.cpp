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

#include "overlap.h"

// Parameters for averaging sample standard deviations.

double Overlap::compare_sig_overlap (const QMap<QString,Sig*> *sig_a, 
				     const QMap<QString,Sig*> *sig_b) {

  double score = 0.;
  int hit_count = 0;
  int total_count = 0;

  QMapIterator<QString,Sig*> i_a (*sig_a);
  while (i_a.hasNext()) {
    i_a.next();
    QString mac = i_a.key();
    Sig* a = i_a.value();
    total_count++;
    if (sig_b->contains (mac)) {

      Sig* b = sig_b->value (mac);

      double overlap = compute_overlap (a->mean(), a->stddev(),
					b->mean(), b->stddev());

      

      //qDebug () << "overlap " << overlap
      //<< " w-a " << a->weight
      //<< " w-b " << b->weight;
      double delta = overlap * ((a->weight()+b->weight()) / 2.);
      score += delta;
      hit_count++;

//       qDebug () << "compare "<< mac 
// 		<< " overlap= " << delta
// 		<< " delta=" << delta
// 		<< " score=" << score
// 		<< " mean a="<< a->mean << " b=" << b->mean
// 		<< " stddev a="<< a->stddev << " b=" << b->stddev;


    } else {

      //score -= a->weight/2.;

//       qDebug () << "compare "<< mac << " no overlap"
// 		<< " score=" << score;

    }

  }

  //qDebug () << "compare_sig_overlap A " << score;


//   QMapIterator<QString,Sig*> i_b (*sig_b);
//   while (i_b.hasNext()) {
//     i_b.next();
//     QString mac = i_b.key();
//     Sig* b = i_b.value();
//     if (!sig_a->contains (mac)) {
//       //score -= b->weight/2.;
//       qDebug () << "compare "<< mac << " no overlap"
// 		<< " score=" << score;
//       total_count++;
//     }

//   }

//   qDebug () << "compare hit_rate="<< hit_count/(double)total_count;


  return score;
}

double computePenalty (double weight, int penalty) {

  if (penalty <= 0) {
    return 0;
  }
  return weight/(double)penalty;

}

double Overlap::compareHistOverlap(const QMap<QString,Sig*> *sig_a,
                                   const QMap<QString,Sig*> *sig_b, int penalty)
{

  double score = 0.;
  int hitCount = 0;

  QMapIterator<QString,Sig*> it (*sig_a);
  while (it.hasNext()) {
    it.next();
    QString mac = it.key();
    //Histogram *histA = (Histogram*)(it.value());
    Sig* sigA = it.value();

    if (sig_b->contains(mac)) {
      //Histogram *histB = sig_b->value(mac)->histogram();
      Sig* sigB = sig_b->value(mac);
      double overlap = 0.0;
      if (sigA && sigB) {
        overlap = Sig::computeHistOverlap(sigA, sigB);
	if (penalty == -1) {
	  score += overlap;
	} else {
	  score += overlap * ((sigA->weight()+sigB->weight()) / 2.);
	}
        ++hitCount;
      }
    } else {

      score -= computePenalty (sigA->weight(), penalty);

    }
  }

  if (penalty > 0) {

    QMapIterator<QString,Sig*> it (*sig_b);
    while (it.hasNext()) {
      it.next();
      QString mac = it.key();
      Sig* sigB = it.value();
      if (!sig_a->contains (mac)) {
	score -= computePenalty (sigB->weight(), penalty);
      }

    }
  }

  if (hitCount == 0)
    return -1.0;

  if (penalty == -1)
    return score/hitCount;

  return score;

}

double Overlap::compute_overlap
(double mean_1, double sigma_1, double mean_2, double sigma_2) {
  
  // Overlap coefficient between two normal distributions. See
  // [Inman and Bradley (1989) "The Overlapping Coefficient as a
  // Measure of Agreement between Probability Distributions and Point
  // Estimation of the Overlap of Two Normal Densities", Communications
  // in Statistics - Theory and Methods
    
  // Standard deviations must be positive

  //qDebug () << "sigma_1 " << sigma_1;
  //qDebug () << "sigma_2 " << sigma_2;

  Q_ASSERT (sigma_1 > 0);
  Q_ASSERT (sigma_2 > 0);

  double OVLap = 0;

  // Same variance. One intersection.
  if (abs(sigma_1 - sigma_2) < 0.001) {
    double sigma = sigma_1;
    double delta = (mean_1-mean_2) / sigma;
    OVLap = 2.0 * ncdf(-0.5 * abs(delta));
  } else {
    // Two intersections.
    // Swap so that sigma_1 < sigma_2
    if (sigma_1 > sigma_2) {
      double mean_tmp = mean_1;
      double sigma_tmp = sigma_1;
      mean_1 = mean_2;
      sigma_1 = sigma_2;
      mean_2 = mean_tmp;
      sigma_2 = sigma_tmp;
    }

    double x1, x2;
    intersection(mean_1, sigma_1, mean_2, sigma_2, x1, x2);

    OVLap = ncdf((x1-mean_1)/sigma_1) + ncdf((x2-mean_2)/sigma_2) -
      ncdf((x1-mean_2)/sigma_2) - ncdf((x2-mean_1)/sigma_1) + 1;
  }
  return OVLap;
}

// Cumulative normal distribution function.
double Overlap::ncdf (double x) {
    return 1.0 - 0.5*erfcc(x/sqrt(2.0));
}


// Return X1 and X2 where X1 is smaller.
void Overlap::intersection 
(double mean_1, double sigma_1, double mean_2, double sigma_2,
 double &x1_ret, double &x2_ret) {


  double x1 = ( (mean_1*(pow(sigma_2,2.0)) - mean_2*(pow(sigma_1,2.0))
		 + sigma_1*sigma_2*pow((pow((mean_1-mean_2),2.0)+(pow(sigma_2,2.0)-pow(sigma_1,2.0))*log((sigma_2)/(sigma_1))),(0.5))) /
		((pow(sigma_2,2.0))-(pow(sigma_1,2.))) );

  double x2 = ( (mean_1*(pow(sigma_2,2.0)) - mean_2*(pow(sigma_1,2.0))
		 - sigma_1*sigma_2*pow((pow((mean_1-mean_2),2.0)+(pow(sigma_2,2.0)-pow(sigma_1,2.0))*log((sigma_2)/(sigma_1))),(0.5))) /
		((pow(sigma_2,2.0))-(pow(sigma_1,2.))) );

  if (x1 <= x2) {
    x1_ret = x1;
    x2_ret = x2;

  } else {

    x1_ret = x2;
    x2_ret = x1;

  }
}


// http://mail.python.org/pipermail/python-list/2000-June/039873.html
// which translated "Numerical Recipes" book

// Complementary error function.
double Overlap::erfcc (double x) {
    double z = abs(x);
    double t = 1.0/ (1.0 + 0.5*z);

    double r = t * exp(-z*z-1.26551223+t*(1.00002368+t*(.37409196+
                 t*(.09678418+t*(-.18628806+t*(.27886807+
                 t*(-1.13520398+t*(1.48851587+t*(-.82215223+
                 t*.17087277)))))))));
    if (x >= 0.) {
        return r;
    } else {
        return 2. - r;
    }
}

Overlap::Overlap() {

}

Overlap::~Overlap() {

}
