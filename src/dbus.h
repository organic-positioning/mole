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

#ifndef MOLE_DBUS_H_
#define MOLE_DBUS_H_

#ifdef Q_OS_SYMBIAN

#endif

// TODO fix this b/c maemo is linux
// The idea is that we should be able to exclude D-Bus on Linux

#ifdef Q_WS_MAEMO_5
#include <QtDBus>
#define USE_MOLE_DBUS 1
#endif

#ifdef Q_OS_LINUX
#include <QtDBus>
#define USE_MOLE_DBUS 1
#endif


#endif /* MOLE_DBUS_H_ */
