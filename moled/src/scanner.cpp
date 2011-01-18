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

#include <QtCore>
#include <NetworkManager/NetworkManager.h>

#include "scanner.h"

// TODO move this into scanner-linux.cpp

int scan_interval_msec = 2500;
const int min_scan_interval_msec = 10000;
const int max_scan_interval_msec = 120000;

ulong new_scan_found_count = 0;
ulong total_scan_count = 0;

Scanner::Scanner (QObject *parent, Localizer *_localizer, Binder *_binder) :
  QObject (parent),
  localizer (_localizer), binder (_binder) {

  have_set_driver = false;

  timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(scan_access_points()));
  timer->start (scan_interval_msec);
  previous_scan = NULL;
  previous_scan_time = QTime::currentTime();
}

Scanner::~Scanner () {
  if (previous_scan != NULL) {
    delete previous_scan;
  }
}

void Scanner::scan_access_points () {

  QList<AP_Reading*> *readings = new QList<AP_Reading*> ();

  scan_access_points (readings);

  AP_Scan *scan = NULL;

  total_scan_count++;
  int readings_size = readings->size();
  bool is_new_scan = false;

  if (readings_size > 0) {
    //qDebug() << "scanner found " << readings->size() << " readings";

    QDateTime *now = new QDateTime (QDateTime::currentDateTime().toUTC());
    scan = new AP_Scan (readings, now);

    if (previous_scan_time.elapsed() > 10000) {
      qDebug() << "OLD " << previous_scan_time.elapsed();
    }

    //qDebug () << "scanner comparing to previous scan";
    if (previous_scan == NULL || !previous_scan->is_same_scan (scan)
	|| previous_scan_time.elapsed() > 60000) {
      is_new_scan = true;
      new_scan_found_count++;
      previous_scan_time = QTime::currentTime();
      //qDebug () << "assigned previous scan";
      if (previous_scan != NULL) {
	delete previous_scan;
      }
      previous_scan = new AP_Scan (scan);
      scan_interval_msec -= 1000;

    } else {
      //qDebug () << "scanner ignoring duplicate scan";
      delete scan;
      scan = NULL;
      scan_interval_msec += 1000;
    }


  } else {
    //qDebug() << "scanner found 0 readings";
  }


  if (scan_interval_msec < min_scan_interval_msec) {
    scan_interval_msec = min_scan_interval_msec;
  } else if (scan_interval_msec > max_scan_interval_msec) {
    scan_interval_msec = max_scan_interval_msec;
  }

  qDebug() << "scanner found " << readings_size << " readings"
	   << " new rate=" << (new_scan_found_count/(double)total_scan_count)
	   << " scan_interval_msec=" << scan_interval_msec
	   << " is_new_scan=" << is_new_scan;

  timer->start (scan_interval_msec);

  //qDebug () << "scanner notifing binder and localizer";
  if (scan != NULL) {
    binder->add_scan (scan);
    localizer->add_scan (scan);
  }

}

/*
//void dump_system_network_info (QObject *parent);
//void init_scan  (QObject *parent);
//AP_Scan* scanForAccessPointsDirect (QObject *parent);
*/

/*
void dump_system_network_info (QObject *parent) {

  QSystemDeviceInfo device (parent);
  qDebug () << "imei  " << device.imei();
  qDebug () << "manu  " << device.manufacturer();
  qDebug () << "model " << device.model ();
  qDebug () << "name  " << device.productName();
  qDebug () << "sim   " << device.simStatus();

  QSystemNetworkInfo info (parent);

  qDebug() << "wlan" << info.networkName (QSystemNetworkInfo::WlanMode);
  qDebug() << "ethernet" << info.networkName (QSystemNetworkInfo::EthernetMode);

  qDebug() << "wlan" << info.networkSignalStrength (QSystemNetworkInfo::WlanMode);
  qDebug() << "ethernet" << info.networkSignalStrength (QSystemNetworkInfo::EthernetMode);

  qDebug() << "wlan" << info.macAddress (QSystemNetworkInfo::WlanMode);
  qDebug() << "ethernet" << info.macAddress (QSystemNetworkInfo::EthernetMode);

  return;

}
*/

/*

void init_scan  (QObject *parent) {

  QNetworkConfigurationManager *mgr = new QNetworkConfigurationManager (parent);

  QList<QNetworkConfiguration> config_list = mgr->allConfigurations ();

  for (int i = 0; i < config_list.size(); i++) {
    QNetworkConfiguration config = config_list.at(i);
    qDebug() << i << " id " << config.identifier();
    qDebug() << i << " name " << config.name();
    qDebug() << i << " valid " << config.isValid();
    qDebug() << i << " purpose " << config.purpose();
    qDebug() << i << " type " << config.type();
  }

  mgr->updateConfigurations();
}

*/

/*
void scanForAccessPoints(QObject *parent) {

  QMap<QString,QDBusObjectPath> availableAccessPoints;

  QNetworkManagerInterface *iface = new QNetworkManagerInterface(parent);

  availableAccessPoints.clear();
    QList<QDBusObjectPath> list = iface->getDevices();

    QScopedPointer<QNetworkManagerInterfaceDevice> devIface;
    QScopedPointer<QNetworkManagerInterfaceDeviceWireless> devWirelessIface;
    QScopedPointer<QNetworkManagerInterfaceAccessPoint> accessPointIface;
    foreach(QDBusObjectPath path, list) {
        devIface.reset(new QNetworkManagerInterfaceDevice(path.path()));

        if(devIface->deviceType() == DEVICE_TYPE_802_11_WIRELESS) {

            devWirelessIface.reset(new QNetworkManagerInterfaceDeviceWireless(devIface->connectionInterface()->path()));
            ////////////// AccessPoints
            QList<QDBusObjectPath> apList = devWirelessIface->getAccessPoints();

            foreach(QDBusObjectPath path, apList) {
                accessPointIface.reset(new QNetworkManagerInterfaceAccessPoint(path.path()));
                QString ssid = accessPointIface->ssid();
		quint32 rssi = accessPointIface->strength();
		quint32 frequency = accessPointIface->frequency();
                availableAccessPoints.insert(ssid, path);
            }
        }
    }
}
*/

/*

// JLs start of scanning code...

static QDBusConnection bus = QDBusConnection::systemBus();

void Scanner::scan_access_points (QList<AP_Reading*> *readings) {

  qDebug () << "not implemented";

  QDBusInterface interface = new QDBusInterface
    (ICD_DBUS_API_INTERFACE,
     ICD_DBUS_API_PATH,
     ICD_DBUS_API_INTERFACE,
     bus);

  // TODO at some point this should switch to:
  // ICD_SCAN_REQUEST_PASSIVE

  bus.connect (ICD_DBUS_API_INTERFACE, ICD_DBUS_API_PATH,
	       ICD_DBUS_API_INTERFACE, ICD_DBUS_API_SCAN_SIG,
	       this,
	       SLOT (handle_scan_result (const QBBusMessage&)));
						 

}
*/


void Scanner::scan_access_points (QList<AP_Reading*> *readings) {


  QDBusInterface interface (NM_DBUS_SERVICE,
			    NM_DBUS_PATH,
			    NM_DBUS_INTERFACE,
			    QDBusConnection::systemBus());

  QDBusReply<QList<QDBusObjectPath> > reply = interface.call ("GetDevices");

  foreach(QDBusObjectPath path, reply.value()) {

    //qDebug () << "path= " << path.path();

    QDBusInterface device (NM_DBUS_SERVICE,
			   path.path(),
			   NM_DBUS_INTERFACE_DEVICE,
			   QDBusConnection::systemBus());
    if (device.isValid()) {

      if (device.property("DeviceType").toUInt() == DEVICE_TYPE_802_11_WIRELESS) {
	
	//qDebug () << "wifi ";
	//qDebug () << "interface " << device->property("Interface");
	//qDebug () << "driver " << device->property("Driver");
	//qDebug () << "udi " << device->property("Udi");
	//qDebug () << "state " << device->property("State");

	if (!have_set_driver) {
	  binder->set_wifi_desc (device.property("Driver").toString());
	  have_set_driver = true;
	}


	// parent?
	QDBusInterface wifi (NM_DBUS_SERVICE,
			     path.path(),
			     NM_DBUS_INTERFACE_DEVICE_WIRELESS,
			     QDBusConnection::systemBus());
	//						  QDBusConnection::systemBus(), parent);
	

	QDBusReply<QList<QDBusObjectPath> > reply = wifi.call("GetAccessPoints");

	foreach(QDBusObjectPath path, reply.value()) {

	  QDBusInterface ap (NM_DBUS_SERVICE,
			     path.path(),
			     NM_DBUS_INTERFACE_ACCESS_POINT,
			     QDBusConnection::systemBus());

	  if (ap.isValid()) {

//  	    qDebug () << ap.property("Ssid").toString();
//  	    qDebug () << ap.property("Strength").toUInt();
//  	    qDebug () << ap.property("Frequency").toUInt();
//  	    qDebug () << ap.property("HwAddress").toString();

	    AP_Reading *reading = new AP_Reading 
	      (ap.property("HwAddress").toString(),
	       ap.property("Ssid").toString(),
	       ap.property("Frequency").toUInt(),
	       ap.property("Strength").toUInt());

	    readings->append (reading);

	  } else {

	    qWarning() << " got invalid AP " << path.path();

	  }

	}	

      } else {
	//qDebug () << "not wifi";
      }

    } else {
      //qDebug () << "not valid";
    }
  }

  // allocated on stack: interface, device, wifi, ap

}


