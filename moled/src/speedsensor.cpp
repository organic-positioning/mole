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

//const qreal GRAVITY = 9.82;

bool SpeedSensor::accelerometerExists = true;
bool SpeedSensor::testedForAccelerometer = false;

void SpeedSensor::handleTimeout () {
  //qDebug () << "speedsensor handleTimeout scanning="<<on;
  timer.stop ();
  if (on) {
    sensor->removeFilter(this);
    sensor->stop();
    on = false;

    // sets 'motion' to current activity
    //Motion motion = updateSpeedShafer ();
    Motion motion = updateSpeedVariance ();

    // examples
    // MMS -> M
    // SMS -> M
    // SSM -> S
    // HSM -> H

    for (int i = MOTION_HISTORY_SIZE-2; i >= 0; i--) {
      motionHistory[i+1] = motionHistory[i];
    }
    motionHistory[0] = motion;

    /*
    for (int i = 0; i < MOTION_HISTORY_SIZE; i++) {
      qDebug () << "motionHistory " << i << motionHistory[i];
    }
    */

    // keep emitting moving as long as we are walking
    if (motionHistory[1] == MOVING || motionHistory[0] == MOVING) {
      emitMotion (MOVING);
    } else if (motionHistory[0] != motionHistory[1]) {
      emitMotion (motionHistory[0]);
    } else if (motionHistory[2] == MOVING &&
	       motionHistory[0] != MOVING &&
	       motionHistory[0] == motionHistory[1]) {
      // handle case *after* we emit the extra motion
      emitMotion (motionHistory[0]);
    }

    timer.start (dutyCycle);
  } else {
    //magSum = 0;
    readingCount = 0;
    /*
    for (int i = 0; i < MAX_READING_COUNT; i++) {
      for (int d = 0; d < ACC_READING_COLUMNS; d++) {
	reading[i][d] = 0.;
      }
      for (int d = 0; d < META_READING_COLUMNS; d++) {
	metaReading[i][d] = 0;
      }
    }
    */

    sensor->addFilter(this);
    sensor->start();
    on = true;
    timer.start (samplingPeriod);
  }
}

SpeedSensor::SpeedSensor(QObject* parent, ScanQueue *_scanQueue, int _samplingPeriod, int _dutyCycle) :
  QObject(parent), scanQueue (_scanQueue),
  samplingPeriod (_samplingPeriod), dutyCycle (_dutyCycle) {

  qDebug () << "speedsensor samplingPeriod="<<samplingPeriod
	    << "dutyCycle="<<dutyCycle;

  sensor = new QAccelerometer(this);
  readingCount = 0;

  connect (&timer, SIGNAL(timeout()), SLOT(handleTimeout()));
  on = false;
  for (int i = 0; i < MOTION_HISTORY_SIZE; i++) {
    motionHistory[i] = STATIONARY;
  }

  handleTimeout ();

  qDebug () << "starting speed sensor";
}

bool SpeedSensor::filter(QAccelerometerReading* accReading) {

  if (readingCount < MAX_READING_COUNT) {
    reading[readingCount][X] = accReading->x();
    reading[readingCount][Y] = accReading->y();
    reading[readingCount][Z] = accReading->z();
    readingCount++;
  }

  return false;
}

Motion SpeedSensor::updateSpeedShafer() {

  if (readingCount < 2) {
    return motionHistory[0];
  }

  qreal metric = 0.;
  for (int dim = 0; dim < 3; dim++) {
    qreal m2 = 0.;
    qreal mSum = 0.;

    for (int i = 0; i < readingCount; i++) {    
      m2 += (reading[i][dim] * reading[i][dim]);
      mSum += reading[i][dim];
    }

    mSum = (mSum * mSum) / readingCount;
    qreal var = (m2 - mSum) / (readingCount - 1);
    metric += var;
  }

  qDebug () << "motion metric " << metric
	    << "m/3" << (metric / 3.)
	    << "readings="<<readingCount;

  Motion motion;
  if (metric > 2) {
    motion = MOVING;
  } else if (metric < 0.1) {
    motion = HIBERNATE;
  } else {
    motion = STATIONARY;
  }
  return motion;

}

Motion SpeedSensor::updateSpeedVariance() {

  if (readingCount < 2) {
    return motionHistory[0];
  }

  qreal magMean = 0.;
  for (int i = 0; i < readingCount; i++) {
    reading[i][MAG] = sqrt 
      (reading[i][X]*reading[i][X] +
       reading[i][Y]*reading[i][Y] +
       reading[i][Z]*reading[i][Z]);
    magMean += reading[i][MAG];
  }
  magMean /= readingCount;

  qreal magVariance = 0.;
  for (int i = 0; i < readingCount; i++) {
    qreal variance = qAbs (magMean-reading[i][MAG]);
    magVariance += variance;
  }
  magVariance /= readingCount;
  
  qDebug () << "mean variance " << magVariance;

  Motion motion;
  if (magVariance > 0.8) {
    motion = MOVING;
  } else if (magVariance < 0.1) {
    motion = HIBERNATE;
  } else {
    motion = STATIONARY;
  }
  return motion;

}

/*
void SpeedSensor::updateSpeedStreak() {

  if (readingCount == 0) {
    qWarning () << "updateSpeed: no readings";
    return;
  }
  qDebug () << "updateSpeed readingCount=" << readingCount;

  for (int i = 0; i < readingCount; i++) {
    reading[i][MAG] = sqrt 
      (reading[i][X]*reading[i][X] +
       reading[i][Y]*reading[i][Y] +
       reading[i][Z]*reading[i][Z]);
  }

  // detect idleness here

  for (int i = 0; i < readingCount-1; i++) {
    if (reading[i+1][MAG] > reading[i][MAG]) {
      metaReading[i][DELTA] = RISING;
    } else if (reading[i+1][MAG] < reading[i][MAG]) {
      metaReading[i][DELTA] = FALLING;
    } else {
      metaReading[i][DELTA] = NO_CHG;
    }
  }

  for (int i = 0; i < readingCount-1; i++) {
    if (metaReading[i][DELTA] == metaReading[i+1][DELTA]) {
      metaReading[i+1][STREAK] = metaReading[i][STREAK];
    } else {
      metaReading[i+1][STREAK] = i+1;
    }
  }

  int maxStreakLength = 0;
  int maxStreakEnd = 0;
  for (int i = 1; i < readingCount; i++) {
    int streakLength = i - metaReading[i][STREAK] + 1;
    if (streakLength > maxStreakLength) {
      maxStreakLength = streakLength;
      maxStreakEnd = i;
    }
  }

  for (int i = 0; i < readingCount; i++) {
    qDebug () << "i " << i << reading[i][MAG]
      	      << metaReading[i][DELTA]
	      << metaReading[i][STREAK];
  }

  // 200ms
  const int MIN_STREAK_MS = 100;
  // cross-multiply e.g.: 500ms / 47 readings = 100ms / ? readings
  int minStreakForMovement = (int)((MIN_STREAK_MS * (double)readingCount) / (double)samplingPeriod);

  int maxStreakStart = metaReading[maxStreakEnd][STREAK];
  qreal magDelta = fabs(reading[maxStreakEnd][MAG]-
			reading[maxStreakStart][MAG]);
  qDebug () << "start " << maxStreakStart;

  qWarning () << "max len="<<maxStreakLength
	      << "index="<<maxStreakEnd
	      << "minStreakForMovement" << minStreakForMovement
	      << "magDelta" << magDelta;

  if (maxStreakLength > minStreakForMovement) {
    qWarning () << "walking **";
  } else {
    qWarning () << "not walking";
  }

  //QFeedbackEffect::playThemeEffect(QFeedbackEffect::ThemeBounceEffect);

  //qreal avgMag = magSum / readingCount;

  //qDebug () << "avg " << avgMag
  //<< "readings " << readingCount;

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


  // TODO
  return;

}
*/

#ifdef USE_MOLE_DBUS
void SpeedSensor::emitMotion (Motion motion) {
  QDBusMessage msg = QDBusMessage::createSignal("/", "com.nokia.moled", "MotionEstimate");
  msg << motion;
  scanQueue->handleMotionEstimate (motion);
  QDBusConnection::sessionBus().send(msg);
  qDebug () << "emitMotion" << motion;
}
#else
void SpeedSensor::emitMotion (Motion motion) {}
#endif

void SpeedSensor::shutdown() {
}


bool SpeedSensor::haveAccelerometer () {
  if (!testedForAccelerometer) {
    accelerometerExists = true;
    QAccelerometer sensor;
    //sensor.setActive (true);
    qDebug () << "haveAccelerometer" << sensor.connectToBackend();
    if (!sensor.connectToBackend()) {
      accelerometerExists = false;
    }
    testedForAccelerometer = true;
    //sensor.setActive (false);
  }
  return accelerometerExists;
}
