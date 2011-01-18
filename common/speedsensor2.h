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

#ifndef SPEEDSENSOR2_H_
#define SPEEDSENSOR2_H_

#include <QAccelerometer>
//#include <QRotationReading>
// #include <QCompassReading>
#include <QtGui>
#include <QtDBus>
//#include "common.h"

// Neccessary for Qt Mobility API usage
QTM_USE_NAMESPACE

enum MovementState { IDLE, NOT_IDLE, WALKING};
 
class SpeedSensor2 : public QObject
{
  Q_OBJECT
 
    public:
 
  SpeedSensor2(QObject* parent = 0);
  void shutdown();
 
private slots:

  void poll_sensor();

  void stop_sensor (); 
  void emit_walking_alert ();

 private:
 
  QAccelerometer* sensor;

  MovementState state;
  
  bool startup;
  int insert_index;
  qreal *mags;
  //qreal p_x, p_y, p_z;

  QTimer poll_timer;
  //QTimer sleep_timer;
  //QTimer walking_timer;
  void start_sensor (); 
};


#endif /* SPEEDSENSOR2_H_ */
