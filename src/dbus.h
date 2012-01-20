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
// Exclude D-Bus on Symbian
#warning symbian: no d-bus
#else
#include <QtDBus>
#define USE_MOLE_DBUS 1
//#warning not symbian: using d-bus
#endif

#endif /* MOLE_DBUS_H_ */
