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


#include "ground_truth.h"


GroundTruth::GroundTruth(QWidget *parent, QString groundTruthFilename) :
    QWidget(parent)
{

  QFile file (groundTruthFilename);

  if (!file.open (QIODevice::ReadOnly | QIODevice::Text)) {
    qCritical () << "Could not open " << groundTruthFilename;
    exit (-1);
  }

  build_ui (file);

}

void GroundTruth::handle_quit() {
  qDebug () << "GroundTruth: aboutToQuit";

}

GroundTruth::~GroundTruth() {
  qDebug () << "GroundTruth: destructor";
}

void GroundTruth::build_ui (QFile &file) {
  
  resize (QSize(UI_WIDTH, UI_HEIGHT).expandedTo(minimumSizeHint()));

  QGridLayout *layout = new QGridLayout ();

  int column = 0;
  int row = 0;
  QTextStream in(&file);
  while (!in.atEnd()) {
    QString place = in.readLine();
    place.trimmed();
    if (place.startsWith(" ") || 
	place.startsWith("#")) {
      qDebug () << "skipping " << place;
      continue;
    }

    qDebug () << "adding " << place;
    QPushButton *button = new QPushButton (place, this);
    connect (button, SIGNAL(clicked()),
	     SLOT(buttonClicked()));

    layout->addWidget (button, row, column);
    column++;
    const int COLUMNS = 3;
    if (column > COLUMNS) {
      column = 0;
      row++;
    }
    buttons.append (button);
  }


  setLayout (layout);

}

void GroundTruth::buttonClicked() {

  qDebug () << "buttonClicked";

  QListIterator<QPushButton *> i (buttons);
  while (i.hasNext()) {
    QPushButton *button = i.next();
    if (button->hasFocus()) {
      qWarning () << "GT " << button->text();
    } else {
      qDebug () << "no focus" << button->text();
    }
  }

}

