/*
 * Mole - Mobile Organic Localisation Engine
 * Copyright 2010-2012 Nokia Corporation.
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

#ifndef MOLE_UTIL_H
#define MOLE_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fcntl.h>
#include <csignal>
#include <sys/types.h>
#include <sys/stat.h>

#include <QCoreApplication>
#include <QDebug>
#include <QStringList>
#include <QTextStream>
#include <QDateTime>
#include <QFile>

extern bool debug;
extern FILE *logStream;
extern void outputHandler(QtMsgType type, const char *msg);
extern void daemonize(QString exec);
extern void initLogger(const char* logFilename);

#endif // MOLE_UTIL_H
