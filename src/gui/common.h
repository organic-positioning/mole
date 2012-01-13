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

#ifndef COMMON_H_
#define COMMON_H_

#include "../completer.h"
#include "../models.h"
#include "../mole.h"
#include "../motion.h"
#include "../network.h"
#include "../qt-utils.h"
#include "../settings_access.h"
#include "../timer.h"

#include <qjson/parser.h>
#include <qjson/serializer.h>


#ifdef Q_WS_MAEMO_5
const int UI_WIDTH = 800;
const int UI_HEIGHT = 400;
const int ICON_SIZE_REG = 64;
const int ICON_SIZE_SMALL = 40;
#else
const int UI_WIDTH = 540;
const int UI_HEIGHT = 240;
const int ICON_SIZE_REG = 32;
const int ICON_SIZE_SMALL = 20;
#endif

#endif /* COMMON_H_ */
