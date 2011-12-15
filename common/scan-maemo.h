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

#ifndef SCAN_MAEMO_H
#define SCAN_MAEMO_H

#include <QtCore>

/*
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

class ICDScan : public QObject
{
  Q_OBJECT
 public:

  ICDScan() {};
  ~ICDScan() {};

  // print??
  void print(void) { qDebug() << "Scan details: to be implemented"; }

  // getters
  quint32 Status() const { return m_status; }
  quint32 Timestamp() const { return m_timestamp; }
  QString ServiceType() const { return m_serviceType; }
  QString ServiceName() const { return m_serviceName; }
  quint32 ServiceAttributes() const { return m_serviceAttributes; }
  QString ServiceId() const { return m_serviceID; }
  qint32  ServicePriority() const { return m_servicePriority; }
  QString NetworkType() const { return m_networkType; }
  QString NetworkName() const { return m_networkName; }
  quint32 NetworkAttributes() const { return m_networkAttributes; }
  QByteArray NetworkId() const { return m_networkID; }
  qint32 NetworkPriority() const { return m_networkPriority; }
  qint32 SignalStrength() const { return m_signalStrength; }
  QString StationId() const { return m_stationID; }
  qint32 SignalStrengthDb() const { return m_signalStrengthDB; }

  // setters
  void SetStatus(quint32 status) { m_status = status; }
  void SetTimestamp(quint32 timestamp) { m_timestamp = timestamp; }
  void SetServiceType(QString serviceType) { m_serviceType = serviceType; }
  void SetServiceName(QString serviceName) { m_serviceName = serviceName; }
  void SetServiceAttributes(quint32 serviceAttributes) { m_serviceAttributes = serviceAttributes; }
  void SetServiceId(QString serviceID) { m_serviceID = serviceID; }
  void SetServicePriority(qint32 servicePriority) { m_servicePriority = servicePriority; }
  void SetNetworkType(QString networkType) { m_networkType = networkType; }
  void SetNetworkName(QString networkName) { m_networkName = networkName; }
  void SetNetworkAttributes(quint32 networkAttributes) { m_networkAttributes = networkAttributes; }
  void SetNetworkId(QByteArray networkID) { m_networkID = networkID; }
  void SetNetworkPriority(qint32 networkPriority) { m_networkPriority = networkPriority; }
  void SetSignalStrength(qint32 signalStrength) { m_signalStrength = signalStrength; }
  void SetStationId(QString stationID) { m_stationID = stationID; }
  void SetSignalStrengthDb(qint32 signalStrengthDB) { m_signalStrengthDB = signalStrengthDB; }

private:
  quint32 m_status;
  quint32 m_timestamp;
  QString m_serviceType;
  QString m_serviceName;
  quint32 m_serviceAttributes;
  QString m_serviceID;
  qint32  m_servicePriority;
  QString m_networkType;
  QString m_networkName;
  quint32 m_networkAttributes;
  QByteArray m_networkID;
  qint32 m_networkPriority;
  qint32 m_signalStrength;
  QString m_stationID;
  qint32 m_signalStrengthDB;

};

#endif // SCAN_MAEMO_H
