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

void SpeedSensor::handleTimeout () {
  qDebug () << "handleTimeout on="<<on;
  timer.stop ();
  if (on) {
    sensor->removeFilter(this);
    sensor->stop();
    on = false;
    updateSpeedShafer ();
    timer.start (dutyCycle);
  } else {
    //magSum = 0;
    readingCount = 0;
    for (int i = 0; i < MAX_READING_COUNT; i++) {
      for (int d = 0; d < ACC_READING_COLUMNS; d++) {
	reading[i][d] = 0.;
      }
      for (int d = 0; d < META_READING_COLUMNS; d++) {
	metaReading[i][d] = 0;
      }
    }

    sensor->addFilter(this);
    sensor->start();
    on = true;
    timer.start (samplingPeriod);
  }
}

SpeedSensor::SpeedSensor(QObject* parent, int _samplingPeriod, int _dutyCycle) :
  QObject(parent), samplingPeriod (_samplingPeriod), dutyCycle (_dutyCycle) {

  qDebug () << "speedsensor samplingPeriod="<<samplingPeriod
	    << "dutyCycle="<<dutyCycle;

  sensor = new QAccelerometer(this);
  readingCount = 0;

  connect (&timer, SIGNAL(timeout()), SLOT(handleTimeout()));
  on = false;
  motion = STATIONARY;
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

void SpeedSensor::updateSpeedShafer() {

  if (readingCount < 2) {
    return;
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

  qDebug () << "metric " << metric
	    << "m/3" << (metric / 3.);

  Motion oldMotion = motion;
  if (metric > 7) {
    // moving
    motion = MOVING;
  } else if (metric < 0.1) {
    // hibernate
    motion = HIBERNATE;
  } else {
    // stationary
    motion = STATIONARY;
  }

  //if (motion != oldMotion) {
    emitMotion ();
    motion = oldMotion;
    //}

}

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

void SpeedSensor::emitMotion () {
  QDBusMessage msg = QDBusMessage::createSignal("/", "com.nokia.moled", "MotionEstimate");
  msg << motion;
  QDBusConnection::sessionBus().send(msg);
}

void SpeedSensor::shutdown() {
}

MotionLogger::MotionLogger(QObject* parent) :
  QObject(parent) {

  qDebug () << "motionLogger";

  /*
  if (logFilename == NULL || logFilename.isEmpty()) {
    fprintf (stderr, "Invalid motion log filename.\n\n");
    exit (-1);
  }

  logFile = new QFile (logFilename);
  if (logFile->open (QFile::WriteOnly | QFile::Append)) {
    logStream = new QTextStream (logFile);

  } else {
    fprintf (stderr, "Could not open motion log file %s.\n\n", 
	     logFilename.toAscii().data());
    exit (-1);
  }
  */

  sensor = new QAccelerometer(this);
  sensor->addFilter(this);
  sensor->start();
}

bool MotionLogger::filter(QAccelerometerReading* reading) {

  if (reading != NULL) {
    //qint64 stamp = QDateTime::currentMSecsSinceEpoch ();
    qreal mag = sqrt ((reading->x() * reading->x()) + (reading->y() * reading->y()) + (reading->z() * reading->z()));

    qDebug () << "acc x=" << reading->x() 
	      << "y=" << reading->y() 
	      << "z=" << reading->z() 
	      << "mag=" << mag;
  }
  return false;
}

//MotionLogger::~MotionLogger () {
//  logFile->close ();
//}
