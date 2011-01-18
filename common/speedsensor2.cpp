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

#include "speedsensor2.h"

const int poll_timer_ms = 100;
//const int sleep_timer_ms = 3000;
//const int walking_min_ms = 8000;

#define MAGS_COUNT 50
qreal gravity = 9.807;
#define HIGH_LOW_MAG_THRESHOLD 2.0
#define IDLE_MAG_THRESHOLD 0.1

SpeedSensor2::SpeedSensor2(QObject* parent) : QObject(parent) {

  startup = true;
  insert_index = 0;
  state = NOT_IDLE;
  mags = new qreal[MAGS_COUNT];
  sensor = new QAccelerometer(this);

  connect (&poll_timer, SIGNAL(timeout()), SLOT(poll_sensor()));

  /*
  sleep_timer.setSingleShot (true);
  connect (&sleep_timer, SIGNAL(timeout()), SLOT (stop_sensor()));

  walking_timer.setSingleShot (true);
  connect (&walking_timer, SIGNAL(timeout()), SLOT (emit_walking_alert()));
  */

  start_sensor ();
  qDebug () << "starting speed sensor";

}


void SpeedSensor2::poll_sensor() {

  // do not delete reading.  It's owned by the sensor.
  QAccelerometerReading* reading = sensor->reading ();
  qreal x = 0.0f;
  qreal y = 0.0f;
  qreal z = 0.0f;

  if (reading) {
    x = reading->x();
    y = reading->y();
    z = reading->z();

    qreal mag = (sqrt ((x * x) + (y * y) + (z * z))) - gravity;

    // circular queue of magnitudes of past s seconds
    mags[insert_index] = mag;
    insert_index++;

    if (insert_index >= MAGS_COUNT) {
      insert_index = 0;


      qreal min = 1000.;
      qreal max = -1000.;
      qreal sum = 0.;
    
      for (int i = 0; i < MAGS_COUNT; i++) {
	//qDebug () << "i " << i << " " << mags[i];

	if (mags[i] > max) {
	  max = mags[i];
	}
	if (mags[i] < min) {
	  min = mags[i];
	}
	sum += mags[i];

      }

      if (max - min < 1.) {
	qreal avg = sum / MAGS_COUNT;
	qreal new_gravity = gravity + avg;
	/*
	qDebug () << "gravity old " << gravity 
		  << " new " << new_gravity
		  << " avg " << avg;
	*/
	if (startup) {
	  gravity = new_gravity;
	} else {
	  gravity = 0.9*gravity + (0.1 * new_gravity);
	}
      }
      startup = false;
    }

    if (startup) {
      qDebug () << "not enough sensor values";
      return;
    }

    int high_count = 0;
    int low_count = 0;
    int idle_count = 0;

    qreal min = 1000.;
    qreal max = -1000.;

    for (int i = 0; i < MAGS_COUNT; i++) {
      //qDebug () << "i " << i << " " << mags[i];

      if (mags[i] > max) {
	max = mags[i];
      }
      if (mags[i] < min) {
	min = mags[i];
      }


      if (mags[i] > HIGH_LOW_MAG_THRESHOLD) {
	high_count++;
      } else if (mags[i] < -HIGH_LOW_MAG_THRESHOLD) {
	low_count++;
      } else if (mags[i] < IDLE_MAG_THRESHOLD && mags[i] > -IDLE_MAG_THRESHOLD) {
	idle_count++;
      }
    }

    float high_fraq = high_count / (float) MAGS_COUNT;
    float low_fraq = low_count / (float) MAGS_COUNT;
    float idle_fraq = idle_count / (float) MAGS_COUNT;

    MovementState current_state = NOT_IDLE;

    /*
    qDebug () << "high " << high_fraq
	      << " low " << low_fraq
	      << " idle " << idle_fraq
	      << " state " << state;
    */

    if (idle_fraq > 0.25 || (max - min < 1.)) {
      current_state = IDLE;
    } else if (high_fraq > 0.10 && low_fraq > 0.10 && idle_fraq < 0.10) {
      current_state = WALKING;
    }

    if (state != current_state) {
      switch (current_state) {
      case IDLE:
	qDebug () << "speed state change IDLE";
	break;
      case NOT_IDLE:
	qDebug () << "speed state change NOT_IDLE";
	break;
      case WALKING:
	qDebug () << "speed state change WALKING";
	break;
      }

      state = current_state;
    }

  }
}

void SpeedSensor2::start_sensor () {
  if (!poll_timer.isActive()) {
    poll_timer.start (poll_timer_ms);
  }

  if (!sensor->isActive()) {
    sensor->start ();
  }
}

void SpeedSensor2::stop_sensor () {
  //poll_timer.stop ();
  //sensor->stop ();

  //walking_timer.stop ();

}

void SpeedSensor2::emit_walking_alert () {
  qDebug() << "WALKING";

  QDBusMessage msg = QDBusMessage::createSignal("/", "com.nokia.moled", "SpeedEstimate");
  msg << 1;
  QDBusConnection::sessionBus().send(msg);


}

void SpeedSensor2::shutdown() {
  qDebug () << "speedsensor shutdown";
  //log_file.close ();
}
