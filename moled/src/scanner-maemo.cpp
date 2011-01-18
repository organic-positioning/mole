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

#include <icd/dbus_api.h>
#include "scanner-maemo.h"

Scanner::Scanner(QObject *parent, Localizer *_localizer, Binder *_binder, Mode scan_mode):
  QObject (parent),
  bus(QDBusConnection::connectToBus(QDBusConnection::SystemBus, "system")),
  localizer (_localizer), binder (_binder),
  mode(scan_mode),
  scanning(false)

{

  // TODO use something from QtMobility to access the local device/driver name
  binder->set_wifi_desc ("N900");

  readings = new QList<AP_Reading*> ();

  // uncomment to echo scan results
  //connect(this, SIGNAL(scannedAccessPoint(Scan *)),
  //this, SLOT(print(Scan *)));

  connect(this, SIGNAL(scannedAccessPoint(Scan *)),
	  this, SLOT(add_to_readings(Scan *)));


  emit_timer = new QTimer(this);
  connect(emit_timer, SIGNAL(timeout()), this, SLOT(emit_current_readings()));


  QTimer *start_timer = new QTimer(this);
  start_timer->setSingleShot (true);
  connect(start_timer, SIGNAL(timeout()), this, SLOT(start()));
  start_timer->start(2500);

}

Scanner::~Scanner()
{
    stop();
}

void Scanner::scanResultHandler(const QDBusMessage &msg)//const QList<QVariant> &msg)
{
    int i = 0;
    const QList<QVariant> args = msg.arguments();

    // parse the arguments received
    Scan *item = new Scan();

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
    this->scannedAccessPoint(item);

    delete item;
}

bool Scanner::start(void)
{
    QString *str = NULL;

    if(this->scanning)
    {
        qDebug() << "Already scanning!";
        return false;
    }

    // prepare the mode
    switch(this->mode)
    {
    case Scanner::Active:
        str = new QString(ICD_SCAN_REQUEST_ACTIVE);
        break;
    case Scanner::Active_saved:
        str = new QString(ICD_SCAN_REQUEST_ACTIVE_SAVED);
        break;
    case Scanner::Passive:
        str = new QString(ICD_SCAN_REQUEST_PASSIVE);
        break;
    default:
        return false;
    }

    /* the dbus message for activating scanning in Maemo 5

    $ dbus-send --system --dest=com.nokia.icd2 --print-reply /com/nokia/icd2 com.nokia.icd2.scan_req uint32:2 array:string:"WLAN_INFRA","WLAN_ADHOC"
        method return sender=:1.81 -> dest=:1.192 reply_serial=2
        array [
        string "WLAN_INFRA"
        string "WLAN_ADHOC"
        ]
     */

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
    QDBusMessage response = bus.call(m);

    // reply received
    if(response.type() == QDBusMessage::ReplyMessage)
    {
        // debugging
        qDebug() << "maemo scanner: network types to be scanned:";
        const QList<QVariant> args = response.arguments();
        QStringList list(args.first().toStringList());
        foreach(QString str, list)
        {
            qDebug() << str;
        }

        // we are scanning already now
        this->scanning = true;

        // connect to signal delivering scan results
        if(!bus.connect(ICD_DBUS_API_INTERFACE,
                        ICD_DBUS_API_PATH,
                        ICD_DBUS_API_INTERFACE,
                        ICD_DBUS_API_SCAN_SIG,
                        this, SLOT(scanResultHandler(const QDBusMessage &))))//const QList<QVariant> &))))
        {
            qDebug() << "maemo scanner: problem setting up the signal.";

            // issue stop request
            stop();
        }
        else
        {
            qDebug() << "maemo scanner: connected to scan result signals";
        }
    }
    // error received
    else if (response.type() == QDBusMessage::ErrorMessage)
    {
        qDebug() << response.errorName() << " : " << response.errorMessage();
    }

    // was little leak?
    delete str;

    return this->scanning;

}

bool Scanner::stop(void)
{

    if(!this->scanning)
    {
        qDebug() << "Already stopped.";
        return true;
    }

    /* the dbus message for activating scanning in Maemo 5

    $ dbus-send --system --dest=com.nokia.icd2 --print-reply /com/nokia/icd2 com.nokia.icd2.scan_stop

    */

    QDBusMessage m = QDBusMessage::createMethodCall(ICD_DBUS_API_INTERFACE,
                                                    ICD_DBUS_API_PATH,
                                                    ICD_DBUS_API_INTERFACE,
                                                    ICD_DBUS_API_SCAN_CANCEL);

    qDebug() << "Sending stop request";
    if(!bus.send(m))
    {
        qDebug() << "Problem sending the stop message";
        return false;
    }

    qDebug() << "Scanning stopped.";
    this->scanning = false;

    return true;
}

void Scanner::print(Scan *network)
{
    qDebug() << "WLAN AP:" << network->NetworkName();
    qDebug() << " signal strength" << network->SignalStrengthDb();
    qDebug() << " station id" << network->StationId();
    qDebug() << " net name" << network->NetworkName();
    qDebug() << " net type" << network->NetworkType();
    qDebug() << " net id" << network->NetworkId();
    qDebug() << " net attr" << network->NetworkAttributes();
}

void Scanner::add_to_readings(Scan *network)
{

  // TODO use timestamp?
  // TODO reject anything not WLAN_INFRA?  Are we getting anything not WLAN_INFRA?

  // flip the sign of each RSSI
  // rest of code works 0..100 (max->min)
  int strength = -1 * network->SignalStrengthDb();
  if (strength < 5) {
    qDebug () << "rejecting too strong a signal";
    return;
  }
  if (strength > 100) {
    qDebug () << "rejecting too weak a signal";
    return;
  }

  AP_Reading *reading = new AP_Reading 
    (network->StationId(), network->NetworkName(),
     network->NetworkAttributes(), strength);

  readings->append (reading);

  //qDebug() << "reading " << *reading;

  // wait until the current scan (set of results) is over before we
  // hand it to the other components
  emit_timer->start (2500);

}

void Scanner::emit_current_readings () {

  emit_timer->stop ();

  AP_Scan *scan = NULL;

  if (readings->size() > 0) {

    QDateTime *now = new QDateTime (QDateTime::currentDateTime().toUTC());
    scan = new AP_Scan (readings, now);

    readings = new QList<AP_Reading*> ();
  } else {
    qDebug () << "maemo scanner: emit_current_readings found no readings";
  }

  if (scan != NULL) {
    //qDebug () << "maemo scanner: readings " << *scan;
    binder->add_scan (scan);
    localizer->add_scan (scan);
  }

}
