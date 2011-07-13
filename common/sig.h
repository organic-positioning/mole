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

    float at (int index) const ;

    int min () const { return pMin; }
    int max () const { return pMax; }
    float* getNormalizedValues() { return pNormalizedValues; } 

    static float computeOverlap (Histogram *a, Histogram *b);
    static void normalizeValues (float *inHistogram, float *outHistogram, float factor);
    static void addKernelizedValue (qint8 index, float *histogram);
    static void removeKernelizedValue (qint8 index, float *histogram);

  protected:
    float *pNormalizedValues;
    int pMin;
    int pMax;

    void parseHistogram(const QString &, float *kernelizedValues, int &countTotal);

    static bool inBounds (int index);

};

QDebug operator<<(QDebug dbg, const Histogram &histogram);

class DynamicHistogram : public Histogram {
 friend QDebug operator<<(QDebug dbg, const DynamicHistogram &histogram);
 public:
  DynamicHistogram ();
  ~DynamicHistogram ();
  float *getKernelizedValues () { return pKernelizedValues; }
  void increment () { pCount++; }
  void decrement () { pCount--; Q_ASSERT(pCount >= 0); }
  int getCount () const { return pCount; } 

 private:
  float *pKernelizedValues;
  int pCount;

};



class Sig {
friend QDebug operator<<(QDebug dbg, const Sig &sig);

  public:
    Sig();
    Sig(Sig *other);
    Sig(float, float, float, QString);
    ~Sig();

    void addSignalStrength (qint8 strength);
    void removeSignalStrength (qint8 strength);
    bool isEmpty ();
    void normalizeHistogram ();
    void setWeight (int totalHistogramCount);

    int loudest () const { return pHistogram->min (); }

    float mean () const { return pMean; }
    float stddev () const { return pStddev; }
    float weight () const { return pWeight; }

    static float computeHistOverlap (Sig *a, Sig *b);

  private:
    float pMean;
    float pStddev;
    float pWeight;
    Histogram *pHistogram;

};



#endif /* SIG_H_ */
