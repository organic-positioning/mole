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

#ifndef SCAN_H
#define SCAN_H

#include <QtCore>
#include <QObject>
#include <QString>
#include <QByteArray>

/* #define ICD_DBUS_API_SCAN_SIG   "scan_result_sig"
Scan results signal.

Arguments:

DBUS_TYPE_UINT32              status, see icd_scan_status
DBUS_TYPE_UINT32              timestamp when last seen
DBUS_TYPE_STRING              service type
DBUS_TYPE_STRING              service name
DBUS_TYPE_UINT32              service attributes, see Service Provider API
DBUS_TYPE_STRING              service id
DBUS_TYPE_INT32               service priority within a service type
DBUS_TYPE_STRING              network type
DBUS_TYPE_STRING              network name
DBUS_TYPE_UINT32              network attributes, see Network module API
DBUS_TYPE_ARRAY (BYTE)        network id
DBUS_TYPE_INT32               network priority for different network types
DBUS_TYPE_INT32               signal strength/quality, 0 (none) - 10 (good)
DBUS_TYPE_STRING              station id, e.g. MAC address or similar id
                           you can safely ignore this argument
DBUS_TYPE_INT32               signal value in dB; use signal strength above
                           unless you know what you are doing
*/

class Scan : public QObject
{
    Q_OBJECT
public:

    Scan() {};
    ~Scan() {};

    // print??
    void print(void) { qDebug() << "Scan details: to be implemented";}

    // getters
    quint32 Status(void){return status;}
    quint32 Timestamp(void){return timestamp;}
    QString ServiceType(void){return service_type;}
    QString ServiceName(void){return service_name;}
    quint32 ServiceAttributes(void){return service_attributes;}
    QString ServiceId(void){return service_id;}
    qint32  ServicePriority(void){return service_priority;}
    QString NetworkType(void){return network_type;}
    QString NetworkName(void){return network_name;}
    quint32 NetworkAttributes(void){return network_attributes;}
    QByteArray NetworkId(void){return network_id;}
    qint32 NetworkPriority(void){return network_priority;}
    qint32 SignalStrength(void){return signal_strength;}
    QString StationId(void){return station_id;}
    qint32 SignalStrengthDb(void){return signal_strength_db;}

    // setters

    void SetStatus(quint32 status){this->status = status;}
    void SetTimestamp(quint32 timestamp){this->timestamp = timestamp;}
    void SetServiceType(QString service_type){this->service_type = service_type;}
    void SetServiceName(QString service_name){this->service_name = service_name;}
    void SetServiceAttributes(quint32 service_attributes){this->service_attributes = service_attributes;}
    void SetServiceId(QString service_id){this->service_id = service_id;}
    void SetServicePriority(qint32 service_priority){this->service_priority = service_priority;}
    void SetNetworkType(QString network_type){this->network_type = network_type;}
    void SetNetworkName(QString network_name){this->network_name = network_name;}
    void SetNetworkAttributes(quint32 network_attributes){this->network_attributes = network_attributes;}
    void SetNetworkId(QByteArray network_id){this->network_id = network_id;}
    void SetNetworkPriority(qint32 network_priority){this->network_priority = network_priority;}
    void SetSignalStrength(qint32 signal_strength){this->signal_strength = signal_strength;}
    void SetStationId(QString station_id){this->station_id = station_id;}
    void SetSignalStrengthDb(qint32 signal_strength_db){this->signal_strength_db = signal_strength_db;}


private:

    quint32 status;
    quint32 timestamp;
    QString service_type;
    QString service_name;
    quint32 service_attributes;
    QString service_id;
    qint32  service_priority;
    QString network_type;
    QString network_name;
    quint32 network_attributes;
    QByteArray network_id;
    qint32 network_priority;
    qint32 signal_strength;
    QString station_id;
    qint32 signal_strength_db;

};

#endif // SCAN_H
