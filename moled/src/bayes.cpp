/*
 * Mole - Mobile Organic Localisation Engine
 * Copyright 2010-2011 Nokia Corporation.
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

#include "localizer.h"


QSet<QString> Localizer::findSignatureApMacs(const QMap<QString,SpaceDesc*> &potentialSpaces)
{
  QSet<QString> apUnion;
  QSet<QString> *signatureMacsSet = 0;

  QMapIterator<QString,SpaceDesc*> it (potentialSpaces);
  while (it.hasNext()) {
    it.next();
    signatureMacsSet = it.value()->get_macs();

    QSetIterator<QString> itMac (*signatureMacsSet);
    while (itMac.hasNext()) {
      QString mac = itMac.next();
      if (!apUnion.contains(mac))
        apUnion.insert(mac);
    }
    //apUnion = apUnion.unite(*signatureMacsSet);
  }
  return apUnion;
}

QSet<QString> Localizer::findNovelApMacs(const QMap<QString, SpaceDesc*> &potentialSpaces)
{
  QSet<QString> signatureApSet = findSignatureApMacs(potentialSpaces);
  QSet<QString> scanApSet = QSet<QString>::fromList(active_macs->keys());
  QSet<QString> novelApMacSet;

  // Take a set difference scan_ap_set \ sig_ap_set.
  QSetIterator<QString> it (scanApSet);
  while (it.hasNext()) {
    QString mac = it.next();
    if (!signatureApSet.contains(mac))
      novelApMacSet.insert(mac);
  }

  return novelApMacSet;
}

double Localizer::probabilityEstimateWithHistogram(int rssi, const Sig &signature)
{
  Histogram* hist = signature.histogram();
  int histSize;
  double frequency;

  if (hist) {
    histSize = hist->size();
    frequency = hist->kernelFrequency(rssi); // Kernel estimate or raw frequency value.
  } else {
    histSize = 0;
    frequency = 0.0;
  }

  // These parameters can be a user input in the future
  int laplacePriorWeight = 1;
  int histNumberOfBinsAssumed = 50;

  double conditionalProbability = (double)(frequency + laplacePriorWeight) / (histSize + histNumberOfBinsAssumed * laplacePriorWeight);

  return conditionalProbability;
}

double Localizer::probabilityEstimate(int rssi, const Sig &signature)
{
  double mean = signature.mean;
  double std = signature.stddev;

  double upperLimit = rssi + 0.5;
  double lowerLimit = rssi - 0.5;

  double upperProb = probabilityXLessValue(upperLimit, mean, std);
  double lowerProb = probabilityXLessValue(lowerLimit, mean, std);

  return (upperProb - lowerProb);
}

double Localizer::erfcc(double x)
{
  double z = qAbs(x);
  double t = 1.0/ (1.0 + 0.5*z);

  double r = t * exp(-z*z-1.26551223+t*(1.00002368+t*(.37409196+
               t*(.09678418+t*(-.18628806+t*(.27886807+
               t*(-1.13520398+t*(1.48851587+t*(-.82215223+
               t*.17087277)))))))));
  if (x >= 0.)
    return r;
  else
    return (2. - r);
}

double Localizer::probabilityXLessValue(double value, double mean, double std)
{
  if (value <= mean) {
    double var = (mean - value) / std;
    return (0.5 - (1. - erfcc(var)));
  } else {
    double var = (value - mean) / std;
    return (0.5 + (1. - erfcc(var)));
  }
}

void Localizer::make_bayes_estimate(QMap<QString,SpaceDesc*> &potentialSpaces)
{
  qDebug() <<"=== BAYES ESTIMATE ===";

  //Novel AP MACs in testing_scans.
  QSet<QString> novelApMacs = findNovelApMacs(potentialSpaces);

  QString max_space;
  double max_score = -1.0;

  // Initialize beliefs.
  QMap<QString, double> beliefs;

  int spacesSize = potentialSpaces.size();
  double initialUniformValue = 1.0/spacesSize;

  QMapIterator<QString,SpaceDesc*> it (potentialSpaces);

  while (it.hasNext()) {
      it.next();
      beliefs.insert(it.key(), initialUniformValue);
  }

  // Update beliefs scan by scan.

  // Because we're not working in log-domain, this might cause
  // floating point underflow. However, it shouldn't be a matter
  // because 1) it needs ~150 readings with 1% class-conditional
  // probability without renormalization to reach underflow for double
  // type; 2) the localizer is stateless across sessions -- having
  // some zero probability doesn't matter as long as the normalizer is
  // not 0.

  QMapIterator<QString,MacDesc*> itMac (*active_macs);

  while (itMac.hasNext()) {
      itMac.next();
      QString apMac = itMac.key();

      // Skip this reading if ap is novel.
      if ( novelApMacs.contains(apMac) )
        continue;

      QList<int> apRssiList = itMac.value()->rssiList();
      QListIterator<int> itRssi (apRssiList);

      while(itRssi.hasNext()) {
          int rssi = itRssi.next();
          qreal normalizer = 0.0;
          QMapIterator<QString, double> itBeliefs (beliefs);

          while(itBeliefs.hasNext()) {
              itBeliefs.next();
              QString apBeliefs = itBeliefs.key();
              SpaceDesc *space = potentialSpaces.value(apBeliefs);
              QMap<QString,Sig*> *sigs = space->get_sigs();
              Sig *apSignature = sigs->value(apMac);

              double conditionalProbability = 1.0;

              if(apSignature)
                conditionalProbability += probabilityEstimate(rssi, *apSignature);

              double newProb = itBeliefs.value() * conditionalProbability;
              beliefs.insert(apBeliefs, newProb);
              normalizer += newProb;
          }
          //Renormalization
          Q_ASSERT(normalizer != 0);
          QMapIterator<QString,double> itNormalBeliefs (beliefs);
          while(itNormalBeliefs.hasNext()) {
            itNormalBeliefs.next();
            double newProb = itNormalBeliefs.value() / normalizer;
            beliefs.insert(itNormalBeliefs.key(), newProb);
          }
      }
  }

  QMapIterator<QString, double> itBeliefs (beliefs);

  while(itBeliefs.hasNext()) {
      itBeliefs.next();
      double prob =  itBeliefs.value();

      if (prob > max_score) {
         max_score = prob;
         max_space = itBeliefs.key();
      }
  }

  qDebug() <<"Bayes location estimate: " << max_space << max_score;

}

void Localizer::make_bayes_estimate_with_hist(QMap<QString,SpaceDesc*> &potentialSpaces)
{
  qDebug() <<"=== BAYES ESTIMATE USING HISTOGRAM (KERNEL) ===";

  //Novel AP MACs in testing_scans.
  QSet<QString> novelApMacs = findNovelApMacs(potentialSpaces);

  QString max_space;
  double max_score = -1.0;

  // Initialize beliefs.
  QMap<QString, double> beliefs;

  int spacesSize = potentialSpaces.size();
  double initialUniformValue = 1.0/spacesSize;

  QMapIterator<QString,SpaceDesc*> it (potentialSpaces);

  while (it.hasNext()) {
      it.next();
      beliefs.insert(it.key(), initialUniformValue);
  }

  // Update beliefs scan by scan.

  // Because we're not working in log-domain, this might cause
  // floating point underflow. However, it shouldn't be a matter
  // because 1) it needs ~150 readings with 1% class-conditional
  // probability without renormalization to reach underflow for double
  // type; 2) the localizer is stateless across sessions -- having
  // some zero probability doesn't matter as long as the normalizer is
  // not 0.

  QMapIterator<QString,MacDesc*> itMac (*active_macs);

  while (itMac.hasNext()) {
      itMac.next();
      QString apMac = itMac.key();

      // Skip this reading if ap is novel.
      if ( novelApMacs.contains(apMac) )
        continue;

      QList<int> apRssiList = itMac.value()->rssiList();
      QListIterator<int> itRssi (apRssiList);

      while(itRssi.hasNext()) {
          int rssi = itRssi.next();
          qreal normalizer = 0.0;
          QMapIterator<QString, double> itBeliefs (beliefs);

          while(itBeliefs.hasNext()) {
              itBeliefs.next();
              QString apBeliefs = itBeliefs.key();
              SpaceDesc *space = potentialSpaces.value(apBeliefs);
              QMap<QString,Sig*> *sigs = space->get_sigs();
              Sig *apSignature = sigs->value(apMac);

              double conditionalProbability = 1.0/50.0;

              if(apSignature)
                conditionalProbability = probabilityEstimateWithHistogram(rssi, *apSignature);

              double newProb = itBeliefs.value() * conditionalProbability;
              beliefs.insert(apBeliefs, newProb);
              normalizer += newProb;
          }
          //Renormalization
          Q_ASSERT(normalizer != 0);
          QMapIterator<QString,double> itNormalBeliefs (beliefs);
          while(itNormalBeliefs.hasNext()) {
            itNormalBeliefs.next();
            double newProb = itNormalBeliefs.value() / normalizer;
            beliefs.insert(itNormalBeliefs.key(), newProb);
          }
      }
  }

  QMapIterator<QString, double> itBeliefs (beliefs);

  while(itBeliefs.hasNext()) {
      itBeliefs.next();
      double prob =  itBeliefs.value();

      if (prob > max_score) {
         max_score = prob;
         max_space = itBeliefs.key();
      }
  }

  qDebug() <<"Bayes location estimate using kernel histogram: " << max_space << max_score;

}

