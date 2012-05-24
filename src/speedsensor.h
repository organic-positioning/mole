/*
 * Mole - Mobile Organic Localisation Engine
 * Copyright 2010-2012 Nokia Corporation.
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

#include <QtSensors/QAccelerometer>
//#include <qtm12/QtSensors/QAccelerometer>
#include <QtDBus>

// QTM_BEGIN_NAMESPACE

#include "motion.h"

const int X = 0;
const int Y = 1;
const int Z = 2;
const int MAG = 3;
const int ACC_READING_COLUMNS = 4;

const int MAX_READING_COUNT = 100;

const int MOTION_HISTORY_SIZE = 3; // must be >= 3

QTM_USE_NAMESPACE

class SpeedSensor : public QObject, public QAccelerometerFilter
{
  Q_OBJECT

 public:
  SpeedSensor(QObject* parent = 0, 
              int samplingPeriod = 250, int dutyCycle = 4500,
	      int _hibernationDelay = 300000);

  void shutdown();
  static bool haveAccelerometer();
  static bool HibernateWhenInactive;

 signals:
  void hibernate(bool goToSleep);
  //void motionChange(int motion);
  void motionChange(Motion motion);


 private slots:
  // Override of QAcclerometerFilter::filter(QAccelerometerReading*)
  bool filter(QAccelerometerReading* reading);
  Motion updateSpeedShafer();
  Motion updateSpeedVariance();
  void handleTimeout();
  void handleHibernateTimeout();
  void emitMotion(Motion motion);


 private:
  bool m_lastHibernateMessage;
  QAccelerometer *m_sensor;
  bool m_on;
  Motion m_motionHistory[MOTION_HISTORY_SIZE];
  QTimer m_timer;
  QTimer m_hibernateTimer;
  int m_readingCount;
  qreal m_reading[MAX_READING_COUNT][ACC_READING_COLUMNS];

  const int samplingPeriod;
  const int dutyCycle;
  const int hibernationDelay;

  static bool testedForAccelerometer;
  static bool accelerometerExists;


};

#endif /* SPEEDSENSOR_H_ */
