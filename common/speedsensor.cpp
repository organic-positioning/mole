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

const int timer_ms = 250;
const qreal t_delta = 4.; // = timer_ms / 1000.0

const qreal GRAVITY = 9.82;
const qreal ALPHA = 0.05;

SpeedSensor::SpeedSensor(QObject* parent) : QObject(parent) {

  for (int i = 0; i < 3; i++) {
    accel_ewma[i] = 0.;
  }

  moving = false;
  startup = true;
  moving_count = 0;
  speed = 0.;
  inst = 0;

  m_sensor = new QAccelerometer(this);
  m_sensor->addFilter(this);

  /*
  qrangelist rl = m_sensor->availableDataRates();
  qDebug () << "size " << rl.size();
  foreach (const qrange &r, rl) {
    if (r.first == r.second)
      qDebug () << QString("%1 Hz").arg(r.first);
    else
      qDebug () << QString("%1-%2 Hz").arg(r.first).arg(r.second);
  }
  */

  //m_sensor->setDataRate (1000);
  m_sensor->start();

  update_timer = new QTimer ();
  connect (update_timer, SIGNAL(timeout()), SLOT(update_speed()));
  update_timer->start(timer_ms);

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

  //if (reading == NULL) {
    
  //}

  /*
  qreal mag = sqrt ((reading->x() * reading->x()) + (reading->y() * reading->y()) + (reading->z() * reading->z()));
  //stream << log_time.elapsed() << 
  int e = log_time.elapsed();

  //qDebug () << "e ";
  //qDebug () << e;

  stream << e << " " << reading->x() << " " << reading->y() << " " << reading->y() << " " << mag << "\n";
  //stream << "foo2\n";
  */

  if (inst >= 25) {
    return true;
  }

  accel_inst[inst][0] = reading->x();
  accel_inst[inst][1] = reading->y();
  accel_inst[inst][2] = reading->z();
  inst++;

  return true;
}


void SpeedSensor::update_speed() {

  // compute mean for past 250ms
  qreal accel_mean[3];
  qreal accel_diff[3];
  for (int i = 0; i < 3; i++) {
    accel_mean[i] = 0.;
  }

  for (int x = 0; x < inst; x++) {
    for (int i = 0; i < 3; i++) {
      accel_mean[i] += accel_inst[x][i];
    }
  }
  for (int i = 0; i < 3; i++) {
    accel_mean[i] = accel_mean[i] / inst;
  }
  inst = 0;

  //qDebug("Mean acceleration: x=%f y=%f z=%f",
  //accel_mean[0], accel_mean[1], accel_mean[2]);

  if (startup) {

    // first time through:
    // jump to a good starting value
    for (int i = 0; i < 3; i++) {
      accel_ewma[i] = accel_mean[i];
    }

    startup = false;
  } else {


    qreal accel_diff_sum = 0.;
    // sum positive difference from previous average
    for (int i = 0; i < 3; i++) {
      accel_diff[i] = accel_mean[i] - accel_ewma[i];
      accel_diff_sum += accel_diff[i];
    }

    //qDebug("Diff acceleration: x=%f y=%f z=%f sum=%f",
    //accel_diff[0], accel_diff[1], accel_diff[2], accel_diff_sum);



    // update each ewma
    for (int i = 0; i < 3; i++) {
      accel_ewma[i] = (ALPHA*accel_mean[i]) + ( (1.-ALPHA) * accel_mean[i]);
    }


    // is this diff "large"

    if (abs(accel_diff_sum) > 0.5) {
      moving_count++;
    } else {
      moving_count = 0;
    }

    // emit continuously when we are moving
    // emit once when we shift from moving to not moving

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
}

void SpeedSensor::shutdown() {
  //log_file.close ();
}
