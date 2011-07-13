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

#include "scanner-nm.h"
#include <NetworkManager/NetworkManager.h>


int scan_interval_msec = 2500;
const int min_scan_interval_msec = 10000;
const int max_scan_interval_msec = 120000;

ulong new_scan_found_count = 0;
ulong total_scan_count = 0;

Scanner::Scanner (QObject *parent, ScanQueue *_scanQueue, Binder *_binder) :
  QObject (parent), scanQueue (_scanQueue), binder (_binder) {

  have_set_driver = false;
  wifi = NULL;

  connect(&timer, SIGNAL(timeout()), this, SLOT(scanAccessPoints()));
  timer.start (scan_interval_msec);
}

Scanner::~Scanner () {
}

void Scanner::scanAccessPoints () {

  timer.stop ();

  if (wifi == NULL) {
    initWiFi();
  }

  int readingCount = 0;
  QDBusReply<QList<QDBusObjectPath> > reply = wifi->call("GetAccessPoints");

  foreach(QDBusObjectPath path, reply.value()) {

    QDBusInterface ap (NM_DBUS_SERVICE,
		       path.path(),
		       NM_DBUS_INTERFACE_ACCESS_POINT,
		       QDBusConnection::systemBus());

    if (ap.isValid()) {

      scanQueue->addReading 
	(ap.property("HwAddress").toString(),
	 ap.property("Ssid").toString(),
	 (qint16)(ap.property("Frequency").toUInt()),
	 (qint8)(ap.property("Strength").toUInt()));
      readingCount++;

      //  	    qDebug () << ap.property("Ssid").toString();
      //  	    qDebug () << ap.property("Strength").toUInt();
      //  	    qDebug () << ap.property("Frequency").toUInt();
      //  	    qDebug () << ap.property("HwAddress").toString();


    } else {
      qWarning() << " got invalid AP " << path.path();
    }

  }

  if (readingCount > 0) {

    if (scanQueue->scanCompleted()) {
      scan_interval_msec -= 1000;
    } else {
      scan_interval_msec += 1000;
    }

    if (scan_interval_msec < min_scan_interval_msec) {
      scan_interval_msec = min_scan_interval_msec;
    } else if (scan_interval_msec > max_scan_interval_msec) {
      scan_interval_msec = max_scan_interval_msec;
    }

  } else {
    if (wifi != NULL) {
      delete wifi;
    }
    initWiFi ();
  }

  timer.start (scan_interval_msec);

}

void Scanner::initWiFi () {

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
	wifi = new QDBusInterface (NM_DBUS_SERVICE,
				   path.path(),
				   NM_DBUS_INTERFACE_DEVICE_WIRELESS,
				   QDBusConnection::systemBus());
	// QDBusConnection::systemBus(), parent);

      } else {
	//qDebug () << "not wifi";
      }

    } else {
      //qDebug () << "not valid";
    }
  }

  // allocated on stack: interface, device, ap

}
	
