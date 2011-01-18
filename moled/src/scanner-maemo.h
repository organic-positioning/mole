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
#include "binder.h"
#include "localizer.h"

class Scanner : public QObject {

    Q_OBJECT
    Q_ENUMS(Mode)


public:
    enum Mode { Active, Active_saved, Passive };

    Scanner(QObject *parent = 0, Localizer *localizer = 0, Binder *binder = 0, 
	    Mode scan_mode = Scanner::Passive);
    ~Scanner ();


    bool stop(void);

signals:
    void scannedAccessPoint(Scan *network);

private:

    QDBusConnection bus;

    Localizer *localizer;
    Binder *binder;

    Mode mode;
    bool scanning;

    QTimer *emit_timer;
    QList<AP_Reading*> *readings;

public slots:
    bool start(void);

    // void scanResultHandler(const QList<QVariant> &);
    void scanResultHandler(const QDBusMessage&);
    void print(Scan *network);

private slots:
    void add_to_readings(Scan *network);
    void emit_current_readings();

 };

#endif // SCANNER_H
