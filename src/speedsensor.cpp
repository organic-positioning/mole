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

#include "speedsensor.h"

bool SpeedSensor::accelerometerExists = true;
bool SpeedSensor::testedForAccelerometer = false;

SpeedSensor::SpeedSensor(QObject* parent,
                         int _samplingPeriod, int _dutyCycle,
			 int _hibernationDelay)
  : QObject(parent)
  , m_lastHibernateMessage(false)
  , m_sensor(new QAccelerometer(this))
  , m_on(false)
  , m_readingCount(0)
  , samplingPeriod(_samplingPeriod)
  , dutyCycle(_dutyCycle)
  , hibernationDelay(_hibernationDelay)
{

  connect(&m_timer, SIGNAL(timeout()), SLOT(handleTimeout()));
  connect(&m_hibernateTimer, SIGNAL(timeout()), SLOT(handleHibernateTimeout()));

  for (int i = 0; i < MOTION_HISTORY_SIZE; ++i)
    m_motionHistory[i] = STATIONARY;

  m_hibernateTimer.start(hibernationDelay);
  handleTimeout();

  qWarning() << "starting speed sensor hibernationDelay" << hibernationDelay 
	     << "samplingPeriod=" <<samplingPeriod <<"dutyCycle="<<dutyCycle;
}

void SpeedSensor::handleTimeout()
{
  m_timer.stop();
  if (m_on) {
    m_sensor->removeFilter(this);
    m_sensor->stop();
    m_on = false;

    // sets 'motion' to current activity
    Motion motion = updateSpeedVariance();

    // examples
    // MMS -> M
    // SMS -> M
    // SSM -> S
    // HSM -> H

    for (int i = MOTION_HISTORY_SIZE-2; i >= 0; --i)
      m_motionHistory[i+1] = m_motionHistory[i];

    m_motionHistory[0] = motion;

    // keep emitting moving as long as we are walking
    if (m_motionHistory[1] == MOVING || m_motionHistory[0] == MOVING) {
      emitMotion(MOVING);
    } else if (m_motionHistory[0] != m_motionHistory[1]) {
      emitMotion(m_motionHistory[0]);
    } else if (m_motionHistory[2] == MOVING &&
               m_motionHistory[0] != MOVING &&
               m_motionHistory[0] == m_motionHistory[1]) {
      // handle case *after* we emit the extra motion
      emitMotion(m_motionHistory[0]);
    }

    m_timer.start(dutyCycle);
  } else {
    m_readingCount = 0;

    m_sensor->addFilter(this);
    m_sensor->start();
    m_on = true;
    m_timer.start(samplingPeriod);
  }
}

bool SpeedSensor::filter(QAccelerometerReading* accReading)
{
  if (m_readingCount < MAX_READING_COUNT) {
    m_reading[m_readingCount][X] = accReading->x();
    m_reading[m_readingCount][Y] = accReading->y();
    m_reading[m_readingCount][Z] = accReading->z();
    ++m_readingCount;
  }

  return false;
}

Motion SpeedSensor::updateSpeedShafer()
{
  if (m_readingCount < 2)
    return m_motionHistory[0];

  qreal metric = 0.;
  for (int dim = 0; dim < 3; ++dim) {
    qreal m2 = 0.;
    qreal mSum = 0.;

    for (int i = 0; i < m_readingCount; ++i) {
      m2 += (m_reading[i][dim] * m_reading[i][dim]);
      mSum += m_reading[i][dim];
    }

    mSum = (mSum * mSum) / m_readingCount;
    qreal var = (m2 - mSum) / (m_readingCount - 1);
    metric += var;
  }

  qDebug() << "motion metric " << metric
           << "m/3" << (metric / 3.)
           << "readings=" << m_readingCount;

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

Motion SpeedSensor::updateSpeedVariance()
{
  if (m_readingCount < 2)
    return m_motionHistory[0];

  qreal magMean = 0.;
  for (int i = 0; i < m_readingCount; ++i) {
    m_reading[i][MAG] = sqrt(m_reading[i][X]*m_reading[i][X] +
                           m_reading[i][Y]*m_reading[i][Y] +
                           m_reading[i][Z]*m_reading[i][Z]);
    magMean += m_reading[i][MAG];
  }
  magMean /= m_readingCount;

  qreal magVariance = 0.;
  for (int i = 0; i < m_readingCount; ++i) {
    qreal variance = qAbs(magMean-m_reading[i][MAG]);
    magVariance += variance;
  }
  magVariance /= m_readingCount;

  //qDebug() << "mean variance " << magVariance;

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


void SpeedSensor::emitMotion(Motion motion)
{
  if (motion == MOVING) {
    m_hibernateTimer.stop();
    m_lastHibernateMessage = false;
    emit hibernate(false);
    //m_scanQueue->hibernate(false);
  } else {
    if (!m_lastHibernateMessage) {
      if (!m_hibernateTimer.isActive()) {
	m_hibernateTimer.start(hibernationDelay);
      }
    }
  }

  emit motionChange(motion);
#ifdef USE_MOLE_DBUS
  QDBusMessage msg = QDBusMessage::createSignal("/", "com.nokia.moled", "MotionEstimate");
  msg << motion;
  QDBusConnection::systemBus().send(msg);
#endif
  qDebug() << "emitMotion" << motion;
}

void SpeedSensor::handleHibernateTimeout()
{
  qDebug () << "SpeedSensor handleHibernateTimeout";
  m_hibernateTimer.stop();
  m_lastHibernateMessage = true;
  emit hibernate(true);
  //m_scanQueue->hibernate(true);
}

void SpeedSensor::shutdown()
{
}

bool SpeedSensor::haveAccelerometer()
{
  if (!testedForAccelerometer) {
    accelerometerExists = true;
    QAccelerometer sensor;
    qDebug() << "haveAccelerometer" << sensor.connectToBackend();
    if (!sensor.connectToBackend())
      accelerometerExists = false;

    testedForAccelerometer = true;
  }
  return accelerometerExists;
}
