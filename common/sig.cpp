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

//float defaultKernel[5] = {0.2042, 0.1802, 0.1238, 0.0663, 0.0276};

const unsigned int kernelHalfWidth = 4;

const int MAX_HISTOGRAM_SIZE = 80;
const int MIN_HISTOGRAM_INDEX = 20;
const int MAX_HISTOGRAM_INDEX = 100;

QString dumpFloat (float *array, int length) {
  QString s;
  for (int i = 0; i < length; i++) {
    qDebug () << "i " << i << array[i];
    s.append (i);
    s.append ("=");
    s.append (QString::number(array[i]));
    s.append (" ");
  }
  return s;
}


inline bool Histogram::inBounds (int index) {
  if (index >= 0 && index < MAX_HISTOGRAM_SIZE)
    return true;
  return false;
}

void Histogram::addKernelizedValue (qint8 index, float *histogram) {
  index -= MIN_HISTOGRAM_INDEX;

  if (inBounds (index)) histogram[index] += 0.2042;
  if (inBounds (index-1)) histogram[index-1] += 0.1802;
  if (inBounds (index+1)) histogram[index+1] += 0.1802;
  if (inBounds (index-2)) histogram[index-2] += 0.1238;
  if (inBounds (index+2)) histogram[index+2] += 0.1238;
  if (inBounds (index-3)) histogram[index-3] += 0.0663;
  if (inBounds (index+3)) histogram[index+3] += 0.0663;
  if (inBounds (index-4)) histogram[index-4] += 0.0276;
  if (inBounds (index+4)) histogram[index+4] += 0.0276;

  //if (inBounds (index)) histogram[index] += 1;
}

void Histogram::removeKernelizedValue (qint8 index, float *histogram) {
  index -= MIN_HISTOGRAM_INDEX;

  if (inBounds (index)) histogram[index] -= 0.2042;
  if (inBounds (index-1)) histogram[index-1] -= 0.1802;
  if (inBounds (index+1)) histogram[index+1] -= 0.1802;
  if (inBounds (index-2)) histogram[index-2] -= 0.1238;
  if (inBounds (index+2)) histogram[index+2] -= 0.1238;
  if (inBounds (index-3)) histogram[index-3] -= 0.0663;
  if (inBounds (index+3)) histogram[index+3] -= 0.0663;
  if (inBounds (index-4)) histogram[index-4] -= 0.0276;
  if (inBounds (index+4)) histogram[index+4] -= 0.0276;

  //if (inBounds (index)) histogram[index] -= 1;
}

float Histogram::computeOverlap (Histogram *a, Histogram *b) {
  int min = a->pMin > b->pMin ? a->pMin : b->pMin;
  int max = a->pMax < b->pMax ? a->pMax : b->pMax;

  float sum = 0.;
  for (int i = min; i < max; i++) {
    float overlap = a->at(i);
    if (a->at(i) > b->at(i)) {
      overlap = b->at(i);
    }
    sum += overlap;
  }
  return sum;
}

float Sig::computeHistOverlap (Sig *a, Sig *b) {
  return Histogram::computeOverlap (a->pHistogram, b->pHistogram);
}

void Histogram::normalizeValues (float *inHistogram, float *outHistogram,
		      float factor) {
  if (factor == 0) {
    for (int i = 0; i < MAX_HISTOGRAM_SIZE; i++) {
      outHistogram[i] = 0.f;
    }
    qFatal ("invalid factor in normalizeValues");
    return;
  }
  for (int i = 0; i < MAX_HISTOGRAM_SIZE; i++) {
    outHistogram[i] = inHistogram[i] / factor;
  }

}

Sig::Sig()
  : pMean (100),
    pStddev (0),
    pWeight (0),
    pHistogram (new DynamicHistogram())
{
}

Sig::Sig(float _mean, float _stddev, float _weight, QString _histogram)
  : pMean (_mean),
    pStddev (_stddev),
    pWeight (_weight),
    pHistogram (new Histogram(_histogram))
{
}

Sig::Sig(Sig *userSig) {
  pMean = userSig->mean();
  pStddev = userSig->stddev();
  pWeight = userSig->weight();
  pHistogram = new Histogram ((DynamicHistogram*)(userSig->pHistogram));
}

Sig::~Sig()
{
  //qDebug () << "sig dtor";
  delete pHistogram;
}

void Sig::addSignalStrength (qint8 strength) { 
  Histogram::addKernelizedValue 
    (strength, ((DynamicHistogram*)pHistogram)->getKernelizedValues()); 
  ((DynamicHistogram*)pHistogram)->increment();
}

void Sig::removeSignalStrength (qint8 strength) { 
  Histogram::removeKernelizedValue 
    (strength, ((DynamicHistogram*)pHistogram)->getKernelizedValues()); 
  ((DynamicHistogram*)pHistogram)->decrement();
}

bool Sig::isEmpty () {
  return ((((DynamicHistogram*)pHistogram)->getCount()) == 0);
}

void Sig::normalizeHistogram () {
  Histogram::normalizeValues 
    (((DynamicHistogram*)pHistogram)->getKernelizedValues(),
     pHistogram->getNormalizedValues(),
     (float)(((DynamicHistogram*)pHistogram)->getCount()));
}

void Sig::setWeight (int totalHistogramCount) {
  int c = ((DynamicHistogram*)pHistogram)->getCount();
  if (c >= totalHistogramCount) {
    qFatal ("totalHistogramCount is larger than a single histogram");
  }
  pWeight = c / (float)totalHistogramCount;
}


Histogram::Histogram(QString histogramStr) {

  float kernelizedValues [MAX_HISTOGRAM_SIZE];
  float normalizedValues [MAX_HISTOGRAM_SIZE];

  // encountered weirdness when using memset here...?
  for (int i = 0; i < MAX_HISTOGRAM_SIZE; i++) {
    normalizedValues[i] = 0.f;
    kernelizedValues[i] = 0.f;
  }


  int count = 0;
  pMin = MAX_HISTOGRAM_INDEX;
  pMax = MIN_HISTOGRAM_INDEX;

  parseHistogram (histogramStr, kernelizedValues, count);

  qDebug () << "parsed" << histogramStr << "count" << count;
  normalizeValues (kernelizedValues, normalizedValues, count);

  pMin -= kernelHalfWidth;
  pMax += kernelHalfWidth;

  int length = pMax - pMin + 1;
  if (length > MAX_HISTOGRAM_INDEX) {
    length = MAX_HISTOGRAM_INDEX;
  }
  if (pMin < MIN_HISTOGRAM_INDEX) {
    pMin = MIN_HISTOGRAM_INDEX;
  }
  if (pMax >= MAX_HISTOGRAM_INDEX) {
    pMax = MAX_HISTOGRAM_INDEX-1;
  }

  //qDebug () << "histStr ctor min=" << pMin << "max=" << pMax << "length=" << length;

  pNormalizedValues = new float [length];

  // memcpy??
  for (int i = 0; i < length; i++) {
    pNormalizedValues[i] = normalizedValues[pMin-MIN_HISTOGRAM_INDEX+i];
  }
}

Histogram::Histogram(DynamicHistogram *h) {
  // copy the already normalized histogram from a dynamic histogram
  // into a constant histogram
  int length = h->pMax - h->pMin + 1;
  pNormalizedValues = new float [length];
  pMax = h->pMax;
  pMin = h->pMin;
  for (int i = 0; i < length; i++) {
    pNormalizedValues[i] = h->pNormalizedValues[pMin-MIN_HISTOGRAM_INDEX+1];
  }
}

Histogram::~Histogram()
{
  //qDebug () << "hist dtor";
  delete [] pNormalizedValues;
}

DynamicHistogram::DynamicHistogram()
{
  pKernelizedValues = new float[MAX_HISTOGRAM_SIZE];
  pNormalizedValues = new float[MAX_HISTOGRAM_SIZE];
  for (int i = 0; i < MAX_HISTOGRAM_SIZE; i++) {
    pNormalizedValues[i] = 0.f;
    pKernelizedValues[i] = 0.f;
  }

  pCount = 0;
  pMin = MIN_HISTOGRAM_INDEX;
  pMax = MAX_HISTOGRAM_INDEX;
}

DynamicHistogram::~DynamicHistogram()
{
  //qDebug () << "Dhist dtor";
  delete [] pKernelizedValues;
}

inline float Histogram::at (int index) const {
  if (index < pMin) {
    return 0.;
  }
  if (index > pMax) {
    return 0.;
  }
  index -= pMin;
  //qDebug () << "at index="<<index<< "pMin"<<pMin;
  return pNormalizedValues[index];
}

void Histogram::parseHistogram 
(const QString &histogram, 
 float *kernelizedValues, int &countTotal)
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
	//qDebug () << "parse level="<<level<<"count="<<count;
	for (int i = 0; i < count; i++) {
	  addKernelizedValue(level, kernelizedValues);
	}
      if (level < pMin) {
	pMin = level;
      }
      if (level > pMax) {
	pMax = level;
      }
      }
    }
  }
}

QDebug operator<<(QDebug dbg, const Histogram &histogram) {
  dbg.nospace() << "[";
  for (int i = histogram.min(); i <= histogram.max(); i++) {
    dbg.nospace() << i << "=" << histogram.at(i);
    if (i <= histogram.max() - 1) {
      dbg.nospace() << " ";
    }
  }
  dbg.nospace() << "]";    

  return dbg.space();
}

QDebug operator<<(QDebug dbg, const DynamicHistogram &histogram) {
  dbg.nospace() << "[c="<<histogram.pCount << ",";
  for (int i = 0; i < MAX_HISTOGRAM_SIZE; i++) {
    dbg.nospace() << i << "=" << histogram.pKernelizedValues[i] << "/" << histogram.pNormalizedValues[i];
    if (i <= MAX_HISTOGRAM_SIZE - 1) {
      dbg.nospace() << " ";
    }
  }
  dbg.nospace() << "]";    

  return dbg.space();
}


QDebug operator<<(QDebug dbg, const Sig &sig) {
  dbg.nospace() << "[mean=" << sig.mean() << ",std="<<sig.stddev()
		<< ",wt="<<sig.weight();
  
  DynamicHistogram *h = dynamic_cast<DynamicHistogram *> (sig.pHistogram);
  if (h) {
    dbg.nospace() << ",h="<<*h;
  } else {
    dbg.nospace() << ",h="<<*(sig.pHistogram);
  }
  dbg.nospace() << "]";    
  return dbg.space();
}


int mainTest(int /*argc*/, char */*argv[]*/)
{
  printf ("Histogram Testing\n");
  
  QString histogramStr ("78=1 80=2");
  //Histogram *h3 = new Histogram (histogramStr);
  //qDebug () << "h1 " << *h1;

  Sig *s3 = new Sig (80.0, 1.0, 0.2, histogramStr);
  Sig *s2 = new Sig ();
  //qDebug () << "s2A " << *s2;

  Sig *s1 = new Sig ();
  //qDebug () << "s1A " << *s1;

  s1->addSignalStrength (80);
  s1->addSignalStrength (80);
  s1->addSignalStrength (78);
  //s1->removeSignalStrength (80);
  s1->normalizeHistogram ();
  s1->setWeight (10);

  s2->addSignalStrength (79);
  s2->addSignalStrength (80);
  s2->addSignalStrength (78);
  //s2->removeSignalStrength (80);
  s2->normalizeHistogram ();
  s2->setWeight (10);


  //s1->addSignalStrength (78);
  //s1->normalizeHistogram ();
  //s1->setWeight (10);
  qDebug () << "s1 " << *s1;

  qDebug () << "overlap s1 s2" << Sig::computeHistOverlap (s1, s2);
  qDebug () << "overlap s1 s3" << Sig::computeHistOverlap (s1, s3);

  delete s1;
  delete s2;
  delete s3;
  return 0;
}
