/*
 * Mole - Mobile Organic Localisation Engine
 * Copyright 2010-2011 Nokia Corporation.
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

#ifndef BINDER_H_
#define BINDER_H_

#include <QDateTime>
#include <QDir>
#include <QNetworkReply>
#include <QObject>
#include <QTimer>

class Localizer;
class ScanQueue;

class Binder : public QObject
{
  Q_OBJECT

 public:
  Binder(QObject *parent = 0, Localizer *localizer = 0, ScanQueue *scanQueue = 0);
  ~Binder();

  void setLocationEstimate(QString estimatedSpaceName, double estimatedSpaceScore);
  void setWiFiDesc(QString wiFiDesc);

 public slots:
  QString handleBindRequest(QString source, QString country, QString region,
                            QString city, QString area, int floor,
                            QString space_name, QString tags);

 private slots:
  void handleBindResponse();
  void xmitBind();
  void onlineStateChanged(bool);

 private:
  QDir *m_bindsDir;
  QString m_bindDirName;

  QString m_wifiDesc;
  QString m_deviceDesc;

  QTimer m_xmitBindTimer;
  QDateTime m_oldestValidScan;

  QMap<QString,QString> m_bfn2area;
  Localizer *m_localizer;
  ScanQueue *m_scanQueue;
};

#endif /* BINDER_H_ */
