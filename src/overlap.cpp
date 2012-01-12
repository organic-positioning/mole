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

#include "sig.h"

// Parameters for averaging sample standard deviations.
double Overlap::compareSigOverlap(const QMap<QString,Sig*> *sigA,
                                  const QMap<QString,Sig*> *sigB)
{
  double score = 0.;
  int hitCount = 0;
  int totalCount = 0;

  QMapIterator<QString,Sig*> itA (*sigA);
  while (itA.hasNext()) {
    itA.next();
    QString mac = itA.key();
    Sig* a = itA.value();
    ++totalCount;
    if (sigB->contains(mac)) {
      Sig* b = sigB->value(mac);
      double overlap = computeOverlap(a->mean(), a->stddev(),
                                      b->mean(), b->stddev());

      double delta = overlap * ((a->weight() + b->weight()) / 2.);
      score += delta;
      ++hitCount;
    }
  }
  return score;
}

double computePenalty(double weight, int penalty)
{
  if (penalty <= 0)
    return 0;

  return weight/(double)penalty;

}

double Overlap::compareHistOverlap(const QMap<QString,Sig*> *sigA,
                                   const QMap<QString,Sig*> *sigB, int penalty)
{
  double score = 0.;
  int hitCount = 0;

  QMapIterator<QString,Sig*> it (*sigA);
  while (it.hasNext()) {
    it.next();
    QString mac = it.key();
    Sig* sig1 = it.value();

    if (sigB->contains(mac)) {
      Sig* sig2 = sigB->value(mac);
      double overlap = 0.0;
      if (sig1 && sig2) {
        overlap = Sig::computeHistOverlap(sig1, sig2);
        if (penalty == -1) {
          score += overlap;
        } else {
          score += overlap * ((sig1->weight() + sig2->weight()) / 2.);
        }
        ++hitCount;
      }
    } else {
      score -= computePenalty(sig1->weight(), penalty);
    }
  }

  if (penalty > 0) {
    QMapIterator<QString,Sig*> it (*sigB);
    while (it.hasNext()) {
      it.next();
      QString mac = it.key();
      Sig* sig2 = it.value();
      if (!sigA->contains(mac)) {
        score -= computePenalty(sig2->weight(), penalty);
      }
    }
  }

  if (hitCount == 0)
    return -1.0;

  if (penalty == -1)
    return score/hitCount;

  return score;
}

double Overlap::computeOverlap(double mean1, double sigma1,
                               double mean2, double sigma2)
{
  // Overlap coefficient between two normal distributions. See
  // [Inman and Bradley (1989) "The Overlapping Coefficient as a
  // Measure of Agreement between Probability Distributions and Point
  // Estimation of the Overlap of Two Normal Densities", Communications
  // in Statistics - Theory and Methods

  // Standard deviations must be positive
  Q_ASSERT (sigma1 > 0);
  Q_ASSERT (sigma2 > 0);

  double OVLap = 0;

  // Same variance. One intersection.
  if (abs(sigma1 - sigma2) < 0.001) {
    double sigma = sigma1;
    double delta = (mean1 - mean2) / sigma;
    OVLap = 2.0 * ncdf(-0.5 * abs(delta));
  } else {
    // Two intersections.
    // Swap so that sigma1 < sigma2
    if (sigma1 > sigma2) {
      double meanTmp = mean1;
      double sigmaTmp = sigma1;
      mean1 = mean2;
      sigma1 = sigma2;
      mean2 = meanTmp;
      sigma2 = sigmaTmp;
    }

    double x1, x2;
    intersection(mean1, sigma1, mean2, sigma2, x1, x2);

    OVLap = ncdf((x1 - mean1)/sigma1) + ncdf((x2 - mean2)/sigma2) -
            ncdf((x1 - mean2)/sigma2) - ncdf((x2 - mean1)/sigma1) + 1;
  }
  return OVLap;
}

// Cumulative normal distribution function.
double Overlap::ncdf(double x)
{
  return 1.0 - 0.5 * erfcc(x / sqrt(2.0));
}


// Return X1 and X2 where X1 is smaller.
void Overlap::intersection(double mean1, double sigma1,
                           double mean2, double sigma2,
                           double &x1Ret, double &x2Ret)
{
  double x1 = ( (mean1 * (pow(sigma2, 2.0)) - mean2 * (pow(sigma1, 2.0))
              + sigma1 * sigma2 * pow((pow((mean1 - mean2), 2.0) + (pow(sigma2, 2.0) - pow(sigma1, 2.0)) * log((sigma2)/(sigma1))),(0.5)))
              / ((pow(sigma2, 2.0)) - (pow(sigma1, 2.))) );

  double x2 = ( (mean1 * (pow(sigma2, 2.0)) - mean2 * (pow(sigma1, 2.0))
              - sigma1 * sigma2 * pow((pow((mean1 - mean2), 2.0) + (pow(sigma2, 2.0) - pow(sigma1, 2.0)) * log((sigma2)/(sigma1))),(0.5)))
              / ((pow(sigma2, 2.0)) - (pow(sigma1, 2.))) );

  if (x1 <= x2) {
    x1Ret = x1;
    x2Ret = x2;
  } else {
    x1Ret = x2;
    x2Ret = x1;
  }
}

// Complementary error function.
double Overlap::erfcc(double x)
{
  double z = abs(x);
  double t = 1.0 / (1.0 + 0.5 * z);
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
