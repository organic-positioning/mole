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

#ifndef SCANNER_H
#define SCANNER_H

#include <QtCore>
#include <QtDBus>
#include <QObject>
#include "scan-maemo.h"

#include "scanQueue.h"
#include "binder.h"

class Scanner : public QObject {

    Q_OBJECT
    Q_ENUMS(Mode)


public:
    enum Mode { Active, Active_saved, Passive };

    Scanner(QObject *parent = 0, ScanQueue *scanQueue = 0, 
	    Binder *binder = 0, Mode scan_mode = Scanner::Passive);
    ~Scanner ();


    bool stop(void);

signals:
    void scannedAccessPoint(ICDScan *network);

private:

    QDBusConnection bus;
    ScanQueue *scanQueue;

    Mode mode;
    bool scanning;

    QTime interarrival;
    QTimer start_timer;

public slots:
    bool start(void);

    // void scanResultHandler(const QList<QVariant> &);
    void scanResultHandler(const QDBusMessage&);
    void print(ICDScan *network);

private slots:
    void addReadings(ICDScan *network);
    //void emit_current_readings();

 };

#endif // SCANNER_H
