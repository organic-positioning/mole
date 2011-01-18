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
 
  SpeedSensor(QObject* parent = 0);
  void shutdown();
 
  private slots:
 
  // Override of QAcclerometerFilter::filter(QAccelerometerReading*)
  bool filter(QAccelerometerReading* reading);
  void update_speed();
 
 private:
 
  QAccelerometer* m_sensor;
  // acceleration vector

  int inst;
  qreal accel_inst[25][3];
  qreal accel_ewma[3];

  // scalar
  qreal speed;

  bool moving;
  int moving_count;
  bool startup;

  QTimer *update_timer;

  QTime log_time;
  QTextStream stream;
  QFile log_file;
};


#endif /* SPEEDSENSOR_H_ */
