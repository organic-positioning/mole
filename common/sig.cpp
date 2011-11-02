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

#include "sig.h"

const unsigned int kernelHalfWidth = 4;

const int MAX_HISTOGRAM_SIZE = 80;
const int MIN_HISTOGRAM_INDEX = 20;
const int MAX_HISTOGRAM_INDEX = 100;

QString dumpFloat(float *array, int length)
{
  QString s;
  for (int i = 0; i < length; ++i) {
    qDebug() << "i " << i << array[i];
    s.append(i);
    s.append("=");
    s.append(QString::number(array[i]));
    s.append(" ");
  }
  return s;
}

inline bool Histogram::inBounds(int index)
{
  if (index >= 0 && index < MAX_HISTOGRAM_SIZE)
    return true;

  return false;
}

void Histogram::addKernelizedValue(qint8 index, float *histogram)
{
  index -= MIN_HISTOGRAM_INDEX;

  if (inBounds(index)) histogram[index] += 0.2042;
  if (inBounds(index-1)) histogram[index-1] += 0.1802;
  if (inBounds(index+1)) histogram[index+1] += 0.1802;
  if (inBounds(index-2)) histogram[index-2] += 0.1238;
  if (inBounds(index+2)) histogram[index+2] += 0.1238;
  if (inBounds(index-3)) histogram[index-3] += 0.0663;
  if (inBounds(index+3)) histogram[index+3] += 0.0663;
  if (inBounds(index-4)) histogram[index-4] += 0.0276;
  if (inBounds(index+4)) histogram[index+4] += 0.0276;
}

void Histogram::removeKernelizedValue(qint8 index, float *histogram)
{
  index -= MIN_HISTOGRAM_INDEX;

  if (inBounds(index)) histogram[index] -= 0.2042;
  if (inBounds(index-1)) histogram[index-1] -= 0.1802;
  if (inBounds(index+1)) histogram[index+1] -= 0.1802;
  if (inBounds(index-2)) histogram[index-2] -= 0.1238;
  if (inBounds(index+2)) histogram[index+2] -= 0.1238;
  if (inBounds(index-3)) histogram[index-3] -= 0.0663;
  if (inBounds(index+3)) histogram[index+3] -= 0.0663;
  if (inBounds(index-4)) histogram[index-4] -= 0.0276;
  if (inBounds(index+4)) histogram[index+4] -= 0.0276;
}

float Histogram::computeOverlap(Histogram *a, Histogram *b)
{
  int min = a->m_min > b->m_min ? a->m_min : b->m_min;
  int max = a->m_max < b->m_max ? a->m_max : b->m_max;

  float sum = 0.;
  for (int i = min; i < max; ++i) {
    float overlap = a->at(i);
    if (a->at(i) > b->at(i))
      overlap = b->at(i);
    sum += overlap;
  }
  return sum;
}

float Sig::computeHistOverlap(Sig *a, Sig *b)
{
  return Histogram::computeOverlap(a->m_histogram, b->m_histogram);
}

void Histogram::normalizeValues(float *inHistogram, float *outHistogram, float factor)
{
  if (factor == 0) {
    for (int i = 0; i < MAX_HISTOGRAM_SIZE; ++i)
      outHistogram[i] = 0.f;
    qFatal ("invalid factor in normalizeValues");
    return;
  }

  for (int i = 0; i < MAX_HISTOGRAM_SIZE; ++i)
    outHistogram[i] = inHistogram[i] / factor;
}

Sig::Sig()
  : m_mean(100)
  , m_stdDev(0)
  , m_weight(0)
  , m_histogram(new DynamicHistogram())
{
}

Sig::Sig(float _mean, float _stddev, float _weight, QString _histogram)
  : m_mean(_mean),
    m_stdDev(_stddev),
    m_weight(_weight),
    m_histogram(new Histogram(_histogram))
{
}

Sig::Sig(Sig *userSig)
{
  m_mean = userSig->mean();
  m_stdDev = userSig->stddev();
  m_weight = userSig->weight();
  m_histogram = new Histogram((DynamicHistogram*)(userSig->m_histogram));
}

Sig::~Sig()
{
  delete m_histogram;
}

void Sig::addSignalStrength(qint8 strength)
{
  Histogram::addKernelizedValue(strength,
              ((DynamicHistogram*)m_histogram)->getKernelizedValues());

  ((DynamicHistogram*)m_histogram)->increment();
  m_mean = 0.;
  m_stdDev = 0.;
}

void Sig::removeSignalStrength(qint8 strength)
{
  Histogram::removeKernelizedValue(strength,
              ((DynamicHistogram*)m_histogram)->getKernelizedValues());

  ((DynamicHistogram*)m_histogram)->decrement();
  m_mean = 0.;
  m_stdDev = 0.;
}

bool Sig::isEmpty()
{
  return ((((DynamicHistogram*)m_histogram)->getCount()) == 0);
}

void Sig::normalizeHistogram()
{
  Histogram::normalizeValues(((DynamicHistogram*)m_histogram)->getKernelizedValues(),
              m_histogram->getNormalizedValues(),
              (float)(((DynamicHistogram*)m_histogram)->getCount()));
}

void Sig::setWeight(int totalHistogramCount)
{
  int c = ((DynamicHistogram*)m_histogram)->getCount();
  if (c > totalHistogramCount) {
    qFatal("totalHistogramCount is larger than a single histogram total %d c %d", totalHistogramCount, c);
  }
  if (c == totalHistogramCount) {
    qDebug() << "totalHistogramCount equals single histogram total c" << c;
  }

  m_weight = c / (float)totalHistogramCount;

  //qDebug () << "setWeight" << m_weight;

}

float Sig::mean() {
  if (m_mean == 0.) {
    computeMeanAndStdDev();
  }
  Q_ASSERT (m_mean != 0.);
  return m_mean;
}

float Sig::stddev() {
  if (m_stdDev == 0.) {
    computeMeanAndStdDev();
  }
  Q_ASSERT (m_stdDev != 0.);
  return m_stdDev;
}

void Sig::computeMeanAndStdDev() {
  float avg = 0.;
  float count = 0.;
  for (int i = m_histogram->min(); i < m_histogram->max(); i++) {
    if (m_histogram->at(i) > 0.) {
      float increment = m_histogram->at(i) * (float)i;
      avg += increment;
      count += m_histogram->at(i);
    }
  }
  avg = avg / count;
  
  float meanSquare = 0.;
  for (int i = m_histogram->min(); i < m_histogram->max(); i++) {
    if (m_histogram->at(i) > 0.) {
      float sumSquare = (i*i) * m_histogram->at(i);
      meanSquare += sumSquare;
    }
  }
  meanSquare = meanSquare / count;
  float var = meanSquare - (avg*avg);
  m_mean = avg;
  m_stdDev = qSqrt (var);

  if (m_mean == 0. || m_stdDev == 0.) {
    qWarning () << "min" << m_histogram->min()
		<< "max" << m_histogram->max()
		<< "count" << count;

    for (int i = m_histogram->min(); i < m_histogram->max(); i++) {
      if (m_histogram->at(i) > 0.) {
	qWarning () << "i" << i << m_histogram->at(i);
      }
    }
  }
}

Histogram::Histogram(QString histogramStr)
{
  float kernelizedValues[MAX_HISTOGRAM_SIZE];
  float normalizedValues[MAX_HISTOGRAM_SIZE];

  // encountered weirdness when using memset here...?
  for (int i = 0; i < MAX_HISTOGRAM_SIZE; ++i) {
    normalizedValues[i] = 0.f;
    kernelizedValues[i] = 0.f;
  }

  int count = 0;
  m_min = MAX_HISTOGRAM_INDEX;
  m_max = MIN_HISTOGRAM_INDEX;

  parseHistogram(histogramStr, kernelizedValues, count);

  qDebug() << "parsed" << histogramStr << "count" << count;
  normalizeValues(kernelizedValues, normalizedValues, count);

  m_min -= kernelHalfWidth;
  m_max += kernelHalfWidth;

  int length = m_max - m_min + 1;
  if (length > MAX_HISTOGRAM_INDEX)
    length = MAX_HISTOGRAM_INDEX;

  if (m_min < MIN_HISTOGRAM_INDEX)
    m_min = MIN_HISTOGRAM_INDEX;

  if (m_max >= MAX_HISTOGRAM_INDEX)
    m_max = MAX_HISTOGRAM_INDEX-1;

  m_normalizedValues = new float [length];

  // memcpy??
  for (int i = 0; i < length; ++i)
    m_normalizedValues[i] = normalizedValues[m_min - MIN_HISTOGRAM_INDEX + i];
}

Histogram::Histogram(DynamicHistogram *h)
{
  // copy the already normalized histogram from a dynamic histogram
  // into a constant histogram
  int length = h->m_max - h->m_min + 1;
  m_normalizedValues = new float [length];
  m_max = h->m_max;
  m_min = h->m_min;
  for (int i = 0; i < length; ++i)
    m_normalizedValues[i] = h->m_normalizedValues[m_min - MIN_HISTOGRAM_INDEX + i];
}

Histogram::~Histogram()
{
  delete [] m_normalizedValues;
}

DynamicHistogram::DynamicHistogram()
{
  m_kernelizedValues = new float[MAX_HISTOGRAM_SIZE];
  m_normalizedValues = new float[MAX_HISTOGRAM_SIZE];
  for (int i = 0; i < MAX_HISTOGRAM_SIZE; ++i) {
    m_normalizedValues[i] = 0.f;
    m_kernelizedValues[i] = 0.f;
  }

  m_count = 0;
  m_min = MIN_HISTOGRAM_INDEX;
  m_max = MAX_HISTOGRAM_INDEX;
}

DynamicHistogram::~DynamicHistogram()
{
  delete [] m_kernelizedValues;
}

inline float Histogram::at(int index) const
{
  if (index < m_min)
    return 0.;

  if (index > m_max)
    return 0.;

  index -= m_min;

  return m_normalizedValues[index];
}

void Histogram::parseHistogram(const QString &histogram, float *kernelizedValues, int &countTotal)
{
  QStringList tk1 = histogram.split(" ");
  QStringListIterator i (tk1);
  while (i.hasNext()) {
    QString value = i.next();
    QStringList tk2 = value.split("=");
    QStringListIterator j (tk2);
    while (j.hasNext()) {
      float level = j.next().toFloat();
      int count = j.next().toInt();
      if (level >= MIN_HISTOGRAM_INDEX && level < MAX_HISTOGRAM_INDEX) {
        countTotal += count;
        for (int i = 0; i < count; ++i)
          addKernelizedValue(level, kernelizedValues);

        if (level < m_min)
          m_min = level;

        if (level > m_max)
          m_max = level;
      }
    }
  }
}

QDebug operator << (QDebug dbg, const Histogram &histogram)
{
  dbg.nospace() << "[";
  for (int i = histogram.min(); i <= histogram.max(); ++i) {
    if (histogram.at(i) > 0) {
      dbg.nospace() << i << "=" << histogram.at(i);
      if (i <= histogram.max() - 1)
	dbg.nospace() << " ";
    }
  }
  dbg.nospace() << "]";

  return dbg.space();
}

QDebug operator << (QDebug dbg, const DynamicHistogram &histogram)
{
  dbg.nospace() << "[c=" << histogram.m_count << ",";
  for (int i = 0; i < MAX_HISTOGRAM_SIZE; ++i) {
    dbg.nospace() << i << "=" << histogram.m_kernelizedValues[i] << "/" << histogram.m_normalizedValues[i];
    if (i <= MAX_HISTOGRAM_SIZE - 1)
      dbg.nospace() << " ";
  }
  dbg.nospace() << "]";

  return dbg.space();
}

QDebug operator << (QDebug dbg, Sig &sig)
{
  dbg.nospace() << "[mean=" << sig.mean() << ",std="<<sig.stddev() << ",wt="<<sig.weight();

  DynamicHistogram *h = dynamic_cast<DynamicHistogram *> (sig.m_histogram);
  if (h) {
    dbg.nospace() << ",h="<<*h;
  } else {
    dbg.nospace() << ",h="<<*(sig.m_histogram);
  }
  dbg.nospace() << "]";
  return dbg.space();
}

int mainTest(int /*argc*/, char */*argv[]*/)
{
  qDebug() << "Histogram Testing";

  QString histogramStr("78=1 80=2");

  Sig *s3 = new Sig(80.0, 1.0, 0.2, histogramStr);
  Sig *s2 = new Sig();
  Sig *s1 = new Sig();

  s1->addSignalStrength(80);
  s1->addSignalStrength(80);
  s1->addSignalStrength(78);
  s1->normalizeHistogram();
  s1->setWeight(10);

  s2->addSignalStrength(79);
  s2->addSignalStrength(80);
  s2->addSignalStrength(78);
  s2->normalizeHistogram();
  s2->setWeight(10);

  qDebug() << "s1 " << *s1;

  qDebug() << "overlap s1 s2" << Sig::computeHistOverlap(s1, s2);
  qDebug() << "overlap s1 s3" << Sig::computeHistOverlap(s1, s3);

  delete s1;
  delete s2;
  delete s3;
  return 0;
}

void Sig::serialize(QVariantMap &map) {
  QString weight;
  weight.setNum(m_weight);
  map["weight"] = weight;
  //qDebug () << "serialize weight" << m_weight;
  QVariantMap histMap;
    //QString histogramStr;
    //QTextStream hS (&histogramStr);
    //hS.setRealNumberPrecision (4);
  int count = 0;
  for (int i = m_histogram->min(); i < m_histogram->max(); ++i) {
    if (m_histogram->at(i) > 0) {
      //if (m_histogram->at(i) > 0 && count < 2) {
      QString key;
      key.setNum(i);
      QString value;
      value.setNum(m_histogram->at(i));
      histMap[key] = value;
      count++;
      //hS << i << "=" << m_histogram->at(i);
      //if (i <= m_histogram->max() - 1)
      //hS << " ";
    }
  }
  //qDebug () << "hist " << histogramStr;
  //map["histogram"] = histogramStr;
  map["histogram"] = histMap;
}
