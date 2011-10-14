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

#ifndef SIG_H_
#define SIG_H_

#include <QtCore>

class DynamicHistogram;

class Histogram {

 public:
  Histogram() {}
  Histogram(QString);
  Histogram(DynamicHistogram *h);
  virtual ~Histogram();

  float at(int index) const;
  int min() const { return m_min; }
  int max() const { return m_max; }
  float* getNormalizedValues() { return m_normalizedValues; }

  static float computeOverlap(Histogram *a, Histogram *b);
  static void normalizeValues(float *inHistogram, float *outHistogram, float factor);
  static void addKernelizedValue(qint8 index, float *histogram);
  static void removeKernelizedValue(qint8 index, float *histogram);

 protected:
  float *m_normalizedValues;
  int m_min;
  int m_max;

  void parseHistogram(const QString &, float *kernelizedValues, int &countTotal);
  static bool inBounds(int index);

};

QDebug operator << (QDebug dbg, const Histogram &histogram);

class DynamicHistogram : public Histogram
{
 friend QDebug operator << (QDebug dbg, const DynamicHistogram &histogram);

 public:
  DynamicHistogram();
  ~DynamicHistogram();
  float* getKernelizedValues() { return m_kernelizedValues; }
  void increment() { ++m_count; }
  void decrement() { --m_count; Q_ASSERT(m_count >= 0); }
  int getCount() const { return m_count; }

 private:
  float *m_kernelizedValues;
  int m_count;

};


class Sig
{
 friend QDebug operator << (QDebug dbg, const Sig &sig);

 public:
  Sig();
  Sig(Sig *other);
  Sig(float, float, float, QString);
  ~Sig();

  void addSignalStrength(qint8 strength);
  void removeSignalStrength(qint8 strength);
  bool isEmpty();
  void normalizeHistogram();
  void setWeight(int totalHistogramCount);

  int loudest() const { return m_histogram->min(); }

  float mean() const { return m_mean; }
  float stddev() const { return m_stdDev; }
  float weight() const { return m_weight; }

  void serialize(QVariantMap &map);

  static float computeHistOverlap(Sig *a, Sig *b);

 private:
  float m_mean;
  float m_stdDev;
  float m_weight;
  Histogram *m_histogram;

};

#endif /* SIG_H_ */
