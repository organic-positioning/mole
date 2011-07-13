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

#ifndef SCANNER_SYMBIAN_H
#define SCANNER_SYMBIAN_H

#include <QObject>
#include <QString>
#include <QList>

class APReading
{
public:
    APReading(QString _name, QString _mac, short int _level);
    ~APReading();

    const QString name;
    const QString mac;
    const short int level;
};

class Scanner
{
public:
    Scanner() {};
    ~Scanner() {};
    QList<APReading*> availableAPs();

};
#endif // SCANNER_SYMBIAN_H
