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

  QMapIterator<QString,SpaceDesc*> it (potentialSpaces);
  while (it.hasNext()) {
    QList<QString> signatureMacsList = it.value()->macs();

    QListIterator<QString> itMac (signatureMacsList);
    while (itMac.hasNext()) {
      QString mac = itMac.next();
      if (!apUnion.contains(mac))
	apUnion.insert(mac);
    }
  }
  return apUnion;
}

QSet<QString> Localizer::findNovelApMacs(const QMap<QString, SpaceDesc*> &potentialSpaces)
{
  QSet<QString> signatureApSet = findSignatureApMacs(potentialSpaces);
  QSet<QString> scanApSet = QSet<QString>::fromList(m_fingerprint->keys());
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

double Sig::probabilityEstimateWithHistogram(int rssi)
{
  int histSize = 0;
  double frequency = 0.0;

  histSize = m_histogram->size();
  frequency = m_histogram->at(rssi); // Kernel estimate or raw frequency value.

  Q_ASSERT (histSize < 105);
  Q_ASSERT (frequency > 0. && frequency < 110.);

  // These parameters can be a user input in the future
  int laplacePriorWeight = 1;
  int histNumberOfBinsAssumed = 50;

  double conditionalProbability = (double)(frequency + laplacePriorWeight) / (histSize + histNumberOfBinsAssumed * laplacePriorWeight);

  return conditionalProbability;
}

double Sig::probabilityEstimateWithGaussian(int rssi)
{
  Q_ASSERT (mean() > 0. && mean() < 110.);
  Q_ASSERT (stddev() >= 0. && stddev() < 200.);

  double upperLimit = rssi + 0.5;
  double lowerLimit = rssi - 0.5;

  double upperProb = probabilityXLessValue(upperLimit, mean(), stddev());
  double lowerProb = probabilityXLessValue(lowerLimit, mean(), stddev());

  return (upperProb - lowerProb);
}

double Sig::probabilityXLessValue(double value, double mean, double std)
{
  if (value <= mean) {
    double var = (mean - value) / std;
    return (0.5 - (1. - erfcc(var)));
  } else {
    double var = (value - mean) / std;
    return (0.5 + (1. - erfcc(var)));
  }
}

void Localizer::makeBayesEstimate(QMap<QString,SpaceDesc*> &potentialSpaces)
{
  qDebug() <<"=== BAYES ESTIMATE ===";

  //Novel AP MACs in testing_scans.
  QSet<QString> novelApMacs = findNovelApMacs(potentialSpaces);

  QString maxSpace;
  double maxScore = -1.0;

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

  LEFT OFF HERE IN CONVERTING THIS TO NEW STRUCTURE...

  QMapIterator<QString,APDesc*> itMac (*m_fingerprint);

  while (itMac.hasNext()) {
      itMac.next();
      QString apMac = itMac.key();

      // Skip this reading if ap is novel.
      if (novelApMacs.contains(apMac))
        continue;

      QList<int> apRssiList = itMac.value()->rssiList();
      QListIterator<int> itRssi (apRssiList);

      while (itRssi.hasNext()) {
          int rssi = itRssi.next();
          qreal normalizer = 0.0;
          QMapIterator<QString, double> itBeliefs (beliefs);

          while (itBeliefs.hasNext()) {
              itBeliefs.next();
              QString apBeliefs = itBeliefs.key();
              SpaceDesc *space = potentialSpaces.value(apBeliefs);
              QMap<QString,Sig*> *sigs = space->sigs();
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
          while (itNormalBeliefs.hasNext()) {
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

      if (prob > maxScore) {
         maxScore = prob;
         maxSpace = itBeliefs.key();
      }
  }

  qDebug() <<"Bayes location estimate using Gaussian: " << maxSpace << maxScore;

}

void Localizer::makeBayesEstimateWithHist(QMap<QString,SpaceDesc*> &potentialSpaces)
{
  qDebug() <<"=== BAYES ESTIMATE USING HISTOGRAM (KERNEL) ===";

  //Novel AP MACs in testing_scans.
  QSet<QString> novelApMacs = findNovelApMacs(potentialSpaces);

  QString maxSpace;
  double maxScore = -1.0;

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

  QMapIterator<QString,MacDesc*> itMac (*m_fingerprint);

  while (itMac.hasNext()) {
      itMac.next();
      QString apMac = itMac.key();

      // Skip this reading if ap is novel.
      if (novelApMacs.contains(apMac))
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
              QMap<QString,Sig*> *sigs = space->sigs();
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
          while (itNormalBeliefs.hasNext()) {
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

      if (prob > maxScore) {
         maxScore = prob;
         maxSpace = itBeliefs.key();
      }
  }

  qDebug() <<"Bayes location estimate using kernel histogram: " << maxSpace << maxScore;

}

