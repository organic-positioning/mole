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

#ifndef SPEEDSENSOR_H_
#define SPEEDSENSOR_H_

#include <QAccelerometer>
//#include <QFeedbackEffect>
//#include <QRotationReading>
// #include <QCompassReading>
#include <QtGui>
#include <QtDBus>

#include "localizer.h"
#include "binder.h"

//#include <qmobilityglobal.h>

// #include <qfeedbackactuator.h>
// #include <qfeedbackeffect.h>

// Neccessary for Qt Mobility API usage
QTM_USE_NAMESPACE

//#include <QFeedback>
//

#define X                   0
#define Y                   1
#define Z                   2
#define MAG                 3
#define ACC_READING_COLUMNS 4

#define DELTA               0
#define STREAK              1
#define META_READING_COLUMNS 2

#define RISING              1
#define NO_CHG              0
#define FALLING            -1

#define MAX_READING_COUNT 100

class SpeedSensor : public QObject, public QAccelerometerFilter
{
  Q_OBJECT
 
    public:
 
  SpeedSensor(QObject* parent = 0, Localizer *localizer = 0,
	      Binder *binder = 0,
	      int samplingPeriod = 500, int dutyCycle = 9500);
  void shutdown();
 
  private slots:
 
  // Override of QAcclerometerFilter::filter(QAccelerometerReading*)
  bool filter(QAccelerometerReading* reading);
  void updateSpeedStreak();
  void updateSpeedShafer();
  void handleTimeout ();
  void emitMotion ();
 
 private:
  QAccelerometer* sensor;
  Localizer *localizer;
  Binder *binder;
  bool on;
  Motion motion;
  QTimer timer;
  int readingCount;
  qreal reading[MAX_READING_COUNT][ACC_READING_COLUMNS];
  int   metaReading[MAX_READING_COUNT][META_READING_COLUMNS];

  const int samplingPeriod;
  const int dutyCycle;

};


class MotionLogger : public QObject, public QAccelerometerFilter
{
  Q_OBJECT
 
    public:
 
  MotionLogger(QObject* parent = 0);
 
 private slots:
 
  bool filter(QAccelerometerReading* reading);
 
 private:
 
  QAccelerometer* sensor;
  //QTextStream logStream;
  //QFile *logFile;
};


#endif /* SPEEDSENSOR_H_ */
