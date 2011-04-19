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

#ifndef CORE_H_
#define CORE_H_

#include "localizer.h"
//#include "whereami.h"
//#include "../common/speedsensor2.h"
#include "../../common/speedsensor.h"

#ifdef Q_WS_MAEMO_5
#include "scanner-maemo.h"
#else
#include "scanner.h"
#endif

#include "binder.h"


//using namespace QtMobility;
using namespace std;



class Core : public QCoreApplication {
  Q_OBJECT

public:
  Core (int argc = 0, char *argv[] = 0);
  ~Core ();
  int run();

public slots:
  void handle_quit ();


private:
  Localizer *localizer;
  //WhereAmI *whereAmI;
  Binder *binder;
  Scanner *scanner;

#ifdef Q_WS_MAEMO_5
  SpeedSensor *speedsensor;
  //SpeedSensor2 *speedsensor;
#endif

};


#endif /* CORE_H_ */
