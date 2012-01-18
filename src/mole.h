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

#ifndef MOLE_H_
#define MOLE_H_

#define QT_FATAL_ASSERT 1

#include "version.h"
#include "dbus.h"
#include "util.h"
#include "ports.h"

#include <QtCore>
#include <QNetworkAccessManager>
#include <QNetworkConfigurationManager>

extern QString MOLE_ORGANIZATION;
extern QString MOLE_APPLICATION;
extern QString MOLE_DOMAIN;

extern QString MOLE_ICON_PATH;
extern QString DEFAULT_MAP_SERVER_URL;
extern QString DEFAULT_STATIC_SERVER_URL;
extern QString DEFAULT_ROOT_PATH;
extern QSettings *settings;
extern QDir rootDir;
extern bool verbose;
extern QString mapServerURL;
extern QString staticServerURL;
extern QString rootPathname;
extern QNetworkAccessManager *networkAccessManager;
extern QNetworkConfigurationManager *networkConfigurationManager;

void initSettings();
void initCommon(QObject *parent, QString logFilename);

#endif /* MOLE_H_ */
