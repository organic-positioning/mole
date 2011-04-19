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

#include "speedsensor.h"

const qreal GRAVITY = 9.82;

void SpeedSensor::handleTimeout () {
  if (on) {
    sensor->removeFilter(this);
    sensor->stop();
    on = false;
    timer.start (offPeriodMS);
    updateSpeed ();
  } else {
    magSum = 0;
    magSumCount = 0;
    sensor->addFilter(this);
    sensor->start();
    on = true;
    timer.start (onPeriodMS);
  }
}

SpeedSensor::SpeedSensor(QObject* parent, int _onPeriodMS, int _offPeriodMS) :
  QObject(parent), onPeriodMS (_onPeriodMS), offPeriodMS (_offPeriodMS) {

  /*
  moving = false;
  startup = true;
  moving_count = 0;
  speed = 0.;
  inst = 0;
  */

  sensor = new QAccelerometer(this);


  connect (&timer, SIGNAL(timeout()), SLOT(handleTimeout()));
  on = false;
  handleTimeout ();

  qDebug () << "starting speed sensor";

  /*
  log_file.setFileName ("/tmp/accel.log");
  if (!log_file.open (QIODevice::WriteOnly | QIODevice::Truncate)) {
    qFatal ("Could not open file to log accelerometer");
    QCoreApplication::exit (-1);
  }
  log_time.start();
  stream.setDevice (&log_file);
  //stream << "foo\n";
  */
}

bool SpeedSensor::filter(QAccelerometerReading* reading) {

  qreal mag = sqrt ((reading->x() * reading->x()) + (reading->y() * reading->y()) + (reading->z() * reading->z()));

  /*
  //stream << log_time.elapsed() << 
  int e = log_time.elapsed();

  //qDebug () << "e ";
  //qDebug () << e;

  stream << e << " " << reading->x() << " " << reading->y() << " " << reading->y() << " " << mag << "\n";
  //stream << "foo2\n";
  */

  qDebug () << "mag" << mag;
  magSum += mag;
  magSumCount++;

  return false;
}

void SpeedSensor::updateSpeed() {

  if (magSumCount == 0) {
    qWarning () << "updateSpeed: no readings";
    return;
  }

  qreal avgMag = magSum / magSumCount;

  qDebug () << "avg " << avgMag
	    << "readings " << magSumCount;

    // emit continuously when we are moving
    // emit once when we shift from moving to not moving

  /*
    int emit_count = -1;
    if (moving_count > 5) {
      emit_count = moving_count;
      moving = true;
    } else if (moving == true) {
      emit_count = 0;
      moving = false;
    }


    if (emit_count >= 0) {
      QDBusMessage msg = QDBusMessage::createSignal("/", "com.nokia.moled", "SpeedEstimate");
      msg << emit_count;
      QDBusConnection::sessionBus().send(msg);
    }

  }
  */
}

void SpeedSensor::shutdown() {
  //log_file.close ();
}
