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

#include "places.h"

// yes, this could be done recursively...
QVector<QString> places(5);

LineValidator::LineValidator (QObject *parent) :
  QRegExpValidator (parent) {

}

QValidator::State LineValidator::validate ( QString & input, int & pos ) const {
  qDebug () << "validate " << input << " pos " << pos;
  if (input.contains ('/')) {
    return QValidator::Invalid;
  }
  return QValidator::Acceptable;
}


void LineValidator::fixup ( QString & input ) const {
  input.replace (QChar('/'), QChar());
}

PlaceEdit::PlaceEdit(PlaceEdit *_parent, int _index, 
		     QString placeholder_text, QWidget *qparent) :
  QLineEdit(qparent) {
  qDebug() << "creating place edit";
  setMaxLength (40);
  
  setPlaceholderText (placeholder_text);

  model = new PlaceModel (_index, this);
  completer = new QCompleter (model, this);
  completer->setCaseSensitivity (Qt::CaseInsensitive);

  // TODO put in mask
  // TODO prevent slashes

  completer->setModel (model);
  setCompleter (completer);

  setValidator (new LineValidator (this));

  /*
    // delay or otherwise act as if the user hit return after 500ms

  timer = new QTimer (this);
  timer->setSingleShot(true);
  timer->setInterval(500);


  connect(timer, SIGNAL(timeout()), SLOT(handle_check_username()));

  connect(username, SIGNAL(textEdited(QString)), username_timer, SLOT(start()));

  connect (this, SIGNAL (textChanged (QString)),
	   model, SLOT (handle_text_changed (QString)));
  */

  if (_parent != NULL) {
    connect (_parent->get_model(), SIGNAL(value_changed(QString)),
	     model, SLOT(handle_parent_text_changed(QString)));
    connect (_parent->get_model(), SIGNAL(value_changed(QString)),
	     this, SLOT(handle_parent_text_changed(QString)));
  }

}

void PlaceEdit::handle_parent_text_changed (QString text) {
  qDebug () << "place_edit: handle_parent_text_changed " << text;

  setText ("");
}


PlaceModel::PlaceModel(int _index, QObject *parent) :
  UpdatingModel(parent), index(_index)  {
  qDebug() << "creating area model";

  settings_name = "places-";
  settings_name.append (QString::number(index));

  qDebug () << "settings_name "<< settings_name;

  if (index == 0) {
    if (!settings->contains(settings_name)) {
      settings->setValue(settings_name, "USA");
    }
  }

  if (settings->contains(settings_name)) {
    places[index] = settings->value(settings_name).toString();
  } else {
    places[index] = "";
  }

  qDebug () << "index " << index << " places " << places[index];

  // TODO memory leak?
  appendRow (new QStandardItem (places[index]));

  set_url ();

}

PlaceModel::~PlaceModel () {
  //delete settings;
  //delete refill_timer;



}

void PlaceModel::set_url () {

  for (int i = 0; i < index; i++) {
    if (places[i].isEmpty()) {
      qDebug () << "places " << index << " not setting url "
		<< "with empty parent " << i;
      stop_refill ();
      return;
    }
  }

  
  QString url_str(mapServerURL);
  url_str.append ("/map/");
  
  for (int i = 0; i < index; i++) {
    url_str.append (places[i]);
    url_str.append ("/");
  }
  url_str.append ("places.txt");

  qDebug () << "place url " << url_str;

  UpdatingModel::set_url (QUrl(url_str));
}

void PlaceModel::handle_text_changed (QString text) {
  qDebug () << "index " << index << " handle_text_changed " << text;

  //username_timer->stop();

  places[index] = text;
  settings->setValue(settings_name, places[index]);

  // add this new place into the list associated with our current parent

  set_url ();

  /*
    // what is this doing?

    if (url2list_cache->contains(url)) {
      QList<QString> *item_list = url2list_cache->value(url);

      // yuck
      QListIterator<QString> i (*item_list);
      bool found = false;
      while (i.hasNext() && !found) {
	if (i.next() == text) {
	  found = true;
	}
      }
      if (!found) {
	place_list->append(text);
	sort (0);
      }
    }
  */

  emit value_changed (text);

}

void PlaceModel::handle_parent_text_changed (QString text) {
  qDebug () << "index " << index << " handle_parent_text_changed " << text;

  places[index] = "";
  settings->setValue(settings_name, places[index]);
  
  set_url ();

  emit value_changed ("");

}
