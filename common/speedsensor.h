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
//#include <QRotationReading>
// #include <QCompassReading>
#include <QtGui>
#include <QtDBus>
//#include "common.h"

// http://wiki.forum.nokia.com/index.php/Qt_Mobility_example_application:_Fall_Detector

// Neccessary for Qt Mobility API usage
QTM_USE_NAMESPACE
 
class SpeedSensor : public QObject, public QAccelerometerFilter
{
  Q_OBJECT
 
    public:
 
  SpeedSensor(QObject* parent = 0, int onPeriodMS = 0, int offPeriodMS = 0);
  void shutdown();
 
  private slots:
 
  // Override of QAcclerometerFilter::filter(QAccelerometerReading*)
  bool filter(QAccelerometerReading* reading);
  void updateSpeed();
  void handleTimeout ();
 
 private:
 
  QAccelerometer* sensor;

  bool on;

  QTimer timer;
  qreal magSum;
  int magSumCount;

  const int onPeriodMS;
  const int offPeriodMS;

  //QTime log_time;
  //QTextStream stream;
  //QFile log_file;
};


#endif /* SPEEDSENSOR_H_ */
