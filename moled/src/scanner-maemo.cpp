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

#include "scanner-maemo.h"

#include "binder.h"
#include "scan-maemo.h"
#include "scanQueue.h"

#include <QDBusMessage>

#include <icd/dbus_api.h>

Scanner::Scanner(QObject *parent, ScanQueue *scanQueue, Binder *binder, Mode scanMode)
  : QObject(parent)
  , m_bus(QDBusConnection::connectToBus(QDBusConnection::SystemBus, "system"))
  , m_scanQueue(scanQueue)
  , m_hibernating(false)
  , m_mode(scanMode)
  , m_scanning(false)


{
  binder->setWifiDesc("N900");

  //m_interarrival.start();

  connect(this, SIGNAL(scannedAccessPoint(ICDScan *)), this, SLOT(addReadings(ICDScan *)));

  m_startTimer.setSingleShot(true);
  connect(&m_startTimer, SIGNAL(timeout()), this, SLOT(start()));
  m_startTimer.start(2500);

}

Scanner::~Scanner()
{
  stop();
}

void Scanner::handleHibernate(bool goToSleep)
{
  qDebug () << "Scanner handleHibernate" << goToSleep;
  m_hibernating = goToSleep;
  if (goToSleep) {
    // stop scanning now
    stop();
    // scan again after hibernate timeout
    m_startTimer.start(SCAN_INTERVAL_MSEC_HIBERNATE);
  } else {
    // stop any pending timers from hibernating
    m_startTimer.stop();
    // start scanning now
    start();
  }
}

void Scanner::scanResultHandler(const QDBusMessage &msg)//const QList<QVariant> &msg)
{
  int i = 0;
  const QList<QVariant> args = msg.arguments();

  // parse the arguments received
  ICDScan *item = new ICDScan();

  item->SetStatus(args.at(i++).toUInt());
  item->SetTimestamp(args.at(i++).toUInt());
  item->SetServiceType(args.at(i++).toString());
  item->SetServiceName(args.at(i++).toString());
  item->SetServiceAttributes(args.at(i++).toUInt());
  item->SetServiceId(args.at(i++).toString());
  item->SetServicePriority(args.at(i++).toInt());
  item->SetNetworkType(args.at(i++).toString());
  item->SetNetworkName(args.at(i++).toString());
  item->SetNetworkAttributes(args.at(i++).toUInt());
  item->SetNetworkId(args.at(i++).toByteArray());
  item->SetNetworkPriority(args.at(i++).toInt());
  item->SetSignalStrength(args.at(i++).toInt());
  item->SetStationId(args.at(i++).toString());
  item->SetSignalStrengthDb(args.at(i++).toInt());

  // send signal containing the network
  scannedAccessPoint(item);

  delete item;
}

bool Scanner::start()
{
  QString *str = 0;

  if(m_scanning) {
      qDebug() << "Already scanning!";
      return false;
  }

  // prepare the mode
  switch (m_mode) {
  case Scanner::Active:
      str = new QString(ICD_SCAN_REQUEST_ACTIVE);
      break;
  case Scanner::ActiveSaved:
      str = new QString(ICD_SCAN_REQUEST_ACTIVE_SAVED);
      break;
  case Scanner::Passive:
      str = new QString(ICD_SCAN_REQUEST_PASSIVE);
      break;
  default:
      return false;
  }

  QDBusMessage m = QDBusMessage::createMethodCall(ICD_DBUS_API_INTERFACE,
                                                  ICD_DBUS_API_PATH,
                                                  ICD_DBUS_API_INTERFACE,
                                                  ICD_DBUS_API_SCAN_REQ);

  // method call arguments
  QList<QVariant> args;

  // add scan type (uint32)
  args.append(str->toAscii());
  args.last().convert(QVariant::UInt); // to make sure the type is correct

  // add list of network types to scan (string list)
  QStringList list;
  list.append("WLAN_INFRA"); // WLAN_ADHOC, GPRS
  args.append(list);
  args.last().convert(QVariant::StringList);

  // add arguments to the message
  m.setArguments(args);

  // send the message
  qDebug() << "maemo scanner sending scan request";
  QDBusMessage response = m_bus.call(m);

  // reply received
  if(response.type() == QDBusMessage::ReplyMessage) {
    // debugging
    qDebug() << "maemo scanner: network types to be scanned:";
    const QList<QVariant> args = response.arguments();
    QStringList list(args.first().toStringList());
    foreach(QString str, list)
      qDebug() << str;

    // we are scanning already now
    m_scanning = true;

    // connect to signal delivering scan results
    if (!m_bus.connect(ICD_DBUS_API_INTERFACE,
                       ICD_DBUS_API_PATH,
                       ICD_DBUS_API_INTERFACE,
                       ICD_DBUS_API_SCAN_SIG,
                       this, SLOT(scanResultHandler(const QDBusMessage &)))) {
        qDebug() << "maemo scanner: problem setting up the signal.";

        // issue stop request
        stop();
    } else {
        qDebug() << "maemo scanner: connected to scan result signals";
    }
  } else if (response.type() == QDBusMessage::ErrorMessage) {
      qDebug() << response.errorName() << " : " << response.errorMessage();
  }

  // was little leak?
  delete str;

  return m_scanning;

}

bool Scanner::stop()
{
  if (!m_scanning) {
      qDebug() << "Already stopped.";
      return true;
  }

  QDBusMessage m = QDBusMessage::createMethodCall(ICD_DBUS_API_INTERFACE,
                                                  ICD_DBUS_API_PATH,
                                                  ICD_DBUS_API_INTERFACE,
                                                  ICD_DBUS_API_SCAN_CANCEL);

  qDebug() << "Sending stop request";

  if (!m_bus.send(m)) {
      qDebug() << "Problem sending the stop message";
      return false;
  }

  qDebug() << "Scanning stopped.";
  m_scanning = false;

  return true;
}

void Scanner::print(ICDScan *network)
{
  qDebug() << "WLAN AP:" << network->NetworkName();
  qDebug() << " signal strength" << network->SignalStrengthDb();
  qDebug() << " station id" << network->StationId();
  qDebug() << " net name" << network->NetworkName();
  qDebug() << " net type" << network->NetworkType();
  qDebug() << " net id" << network->NetworkId();
  qDebug() << " net attr" << network->NetworkAttributes();
}

void Scanner::addReadings(ICDScan *scan)
{
  // We do not seem to be getting anything WLAN_INFRA
  // so we do not need to exclude them here.
  if (scan->SignalStrengthDb() == 0 && scan->NetworkAttributes() == 0) {
    if (m_hibernating) {
      // scan again in a while, but not right now
      stop();
      m_startTimer.start(SCAN_INTERVAL_MSEC_HIBERNATE);
    }
    m_scanQueue->scanCompleted();
    return;
  }

  // flip the sign of each RSSI
  // rest of code works 0..100 (max->min)
  int strength = -1 * scan->SignalStrengthDb();

  m_scanQueue->addReading(scan->StationId(), scan->NetworkName(),
                          (qint16)(scan->NetworkAttributes()), (qint8)strength);

  //m_interarrival.restart();

}

