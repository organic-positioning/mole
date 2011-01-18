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


#include "binder.h"

// TODO show coverage of local area in color
// TODO have tags come from server

QString display_str = "Incorrect Estimate?";
QString correct_str = "Submit Correction";


Binder::Binder(QWidget *parent) :
    QWidget(parent)
{
  online = true;
  request_location_estimate_counter = 0;
  submit_state = DISPLAY;

  build_ui ();

  bind_timer = new QTimer ();
  connect (bind_timer, SIGNAL(timeout()), SLOT(handle_bind_timer()));

  set_submit_state (DISPLAY);

  settings_dialog = new Settings (this);
  settings_dialog->hide();


  request_location_timer = new QTimer ();
  connect (request_location_timer, SIGNAL(timeout()), SLOT(request_location_estimate()));
  request_location_timer->start(2000);

  walking_timer = new QTimer ();
  connect (walking_timer, SIGNAL(timeout()), SLOT(handle_walking_timer()));

  QDBusConnection::sessionBus().connect
    (QString(), QString(), "com.nokia.moled", "LocationStats", this, 
     SLOT(handle_location_stats(QString,QDateTime,int,int,int,int,int,int,int,double,double,double,double,double,double)));


  QDBusConnection::sessionBus().connect(QString(), QString(), "com.nokia.moled", "LocationEstimate", this, SLOT(handle_location_estimate(QString, bool)));

  QDBusConnection::sessionBus().connect(QString(), QString(), "com.nokia.moled", "SpeedEstimate", this, SLOT(handle_speed_estimate(int)));

}

void Binder::handle_quit() {
  qDebug () << "Binder: aboutToQuit";

}

Binder::~Binder() {
  qDebug () << "Binder: destructor";
}

void Binder::build_ui () {
  
  // LINUX
  // 
  resize (QSize(UI_WIDTH, UI_HEIGHT).expandedTo(minimumSizeHint()));


  QGridLayout *layout = new QGridLayout ();

  QGroupBox *button_box = new QGroupBox ("", this);
  //button_box->setStyleSheet("QGroupBox { border: 1px; } QToolButton { border: 0px; padding 0px; }");
  QGridLayout *button_layout = new QGridLayout ();


  // about
  about_button = new QToolButton (button_box);
  about_button->setIcon (QIcon(icon_root+"about.png"));
  about_button->setToolTip ("About");
  about_button->setIconSize (QSize(icon_size_reg,icon_size_reg));
  connect (about_button, SIGNAL(clicked()), SLOT(handle_clicked_about()));
  

  // feedback
  feedback_button = new QToolButton (button_box);
  feedback_button->setIcon (QIcon(icon_root+"email.png"));
  feedback_button->setToolTip ("Send Feedback");
  feedback_button->setIconSize (QSize(icon_size_reg,icon_size_reg));
  connect (feedback_button, SIGNAL(clicked()), SLOT(handle_clicked_feedback()));

  // help
  help_button = new QToolButton (button_box);
  help_button->setIcon (QIcon(icon_root+"help.png"));
  help_button->setToolTip ("Help");
  help_button->setIconSize (QSize(icon_size_reg,icon_size_reg));
  connect (help_button, SIGNAL(clicked()), SLOT(handle_clicked_help()));

  // settings
  settings_button = new QToolButton (button_box);
  settings_button->setIcon (QIcon(icon_root+"settings.png"));
  settings_button->setToolTip ("Settings");
  settings_button->setIconSize (QSize(icon_size_reg,icon_size_reg));
  connect (settings_button, SIGNAL(clicked()), SLOT(handle_clicked_settings()));

  //////////////////////////////////////////////////////////

  button_layout->setRowStretch (5, 2);
  button_layout->addWidget (about_button, 1, 0);
  button_layout->addWidget (settings_button, 2, 0);
  button_layout->addWidget (help_button, 3, 0);
  button_layout->addWidget (feedback_button, 4, 0);
  button_box->setLayout (button_layout);
  
  QString estimate_title = tr ("Estimate");
  QGroupBox *estimate_box = new QGroupBox (estimate_title, this);
  country_edit = new PlaceEdit (NULL, 0, "Country", estimate_box);
  country_edit->setMaxLength (3);
  country_edit->setInputMask (">AAA");


  region_edit = new PlaceEdit (country_edit, 1, "Region/State", estimate_box);
  city_edit = new PlaceEdit (region_edit, 2, "City", estimate_box);
  area_edit = new PlaceEdit (city_edit, 3, "Area/Blg", estimate_box);
  space_name_edit = new PlaceEdit (area_edit, 4, "Room", estimate_box);

  tags_edit = new QLineEdit (estimate_box);
  MultiCompleter *tags_completer = new MultiCompleter(this);
  tags_completer->setSeparator (QLatin1String(","));
  tags_completer->setCaseSensitivity (Qt::CaseInsensitive);

  country_edit->setToolTip ("Country");
  region_edit->setToolTip ("Region");
  city_edit->setToolTip ("City");
  area_edit->setToolTip ("Area");
  space_name_edit->setToolTip ("Space");
  tags_edit->setToolTip ("Tags");

  QString tags_file = app_root_dir->absolutePath().append ("/map/tags.txt");
  QString url_str = setting_mapserver_url_value;
  QUrl tags_url (url_str.append("/map/tags.txt"));
  UpdatingFileModel *tag_model = new UpdatingFileModel (tags_file, tags_url, tags_completer);

  tags_completer->setModel (tag_model);
  tags_completer->setCaseSensitivity(Qt::CaseInsensitive);
  tags_edit->setCompleter(tags_completer);  

  tags_edit->setMaxLength (80);
  
  tags_edit->setPlaceholderText ("Tags...");


  connect (country_edit, SIGNAL(textChanged(QString)),
	   SLOT (handle_place_changed()));
  connect (region_edit, SIGNAL(textChanged(QString)),
	   SLOT (handle_place_changed()));
  connect (city_edit, SIGNAL(textChanged(QString)),
	   SLOT (handle_place_changed()));
  connect (area_edit, SIGNAL(textChanged(QString)),
	   SLOT (handle_place_changed()));
  connect (space_name_edit, SIGNAL(textChanged(QString)),
	   SLOT (handle_place_changed()));
  connect (tags_edit, SIGNAL(textChanged(QString)),
	   SLOT (handle_place_changed()));
  
  // TODO focus
  //connect (country_edit, SIGNAL(focusInEvent()),
  //SLOT (handle_place_changed()));

  connect (country_edit, SIGNAL(cursorPositionChanged(int,int)),
	   SLOT (handle_place_changed()));
  connect (region_edit, SIGNAL(cursorPositionChanged(int,int)),
	   SLOT (handle_place_changed()));
  connect (city_edit, SIGNAL(cursorPositionChanged(int,int)),
	   SLOT (handle_place_changed()));
  connect (area_edit, SIGNAL(cursorPositionChanged(int,int)),
	   SLOT (handle_place_changed()));
  connect (space_name_edit, SIGNAL(cursorPositionChanged(int,int)),
	   SLOT (handle_place_changed()));
  connect (tags_edit, SIGNAL(cursorPositionChanged(int,int)),
	   SLOT (handle_place_changed()));

  connect (country_edit->get_model(), SIGNAL(network_change(bool)),
	   SLOT (handle_online_change(bool)));


  submit_button = new QPushButton (display_str, estimate_box);
  connect (submit_button, SIGNAL(clicked()),
	   SLOT(handle_clicked_submit()));
  submit_button->setToolTip ("Correct the current position estimate if it is not showing where you are.");

  QGridLayout *estimate_layout = new QGridLayout ();


  QLabel *slashA = new QLabel ("/", estimate_box);
  QLabel *slashB = new QLabel ("/", estimate_box);
  QLabel *slashC = new QLabel ("/", estimate_box);

  estimate_layout->addWidget (area_edit, 1, 1);
  estimate_layout->addWidget (slashA, 1, 2);
  estimate_layout->addWidget (space_name_edit, 1, 3, 1, 3);

  estimate_layout->addWidget (city_edit, 2, 1);
  estimate_layout->addWidget (slashB, 2, 2);
  estimate_layout->addWidget (region_edit, 2, 3);
  estimate_layout->addWidget (slashC, 2, 4);
  estimate_layout->addWidget (country_edit, 2, 5);
 
  estimate_layout->addWidget (tags_edit, 3, 1);

  estimate_layout->addWidget (submit_button, 3, 3, 1, 3);

  //estimate_layout->setColumnStretch (0,8);
  estimate_layout->setColumnStretch (1,16);
  estimate_layout->setColumnStretch (2,0);
  estimate_layout->setColumnStretch (3,12);
  estimate_layout->setColumnStretch (4,0);
  estimate_layout->setColumnStretch (5,6);

  estimate_box->setLayout (estimate_layout);

  //////////////////////////////////////////////////////////
  QString statistics_title = tr ("Statistics");
  QGroupBox *stats_box = new QGroupBox (statistics_title, this);
  QGridLayout *stats_layout = new QGridLayout ();
  // grey  235 233 216  EB E9 D8
  // blue  000 051 204  00 33 CC
  // green 068 165 028  44 A5 1C
  // border: 0px; padding 0px; 

  // network connectivity
  QLabel *net_info_label = new QLabel ("Net:", stats_box);
  net_label = new ColoredLabel ("OK", stats_box);

  QString net_info_tooltip ("Network Connectivity Status.\nCan we talk to the network?");
  net_info_label->setToolTip (net_info_tooltip);
  net_label->setToolTip (net_info_tooltip);

  // daemon connectivity
  QLabel *daemon_info_label = new QLabel ("Daemon:", stats_box);
  daemon_label = new ColoredLabel ("OK", stats_box);
  QString daemon_info_tooltip ("Daemon Status.  Is the positioning daemon started?");
  daemon_info_label->setToolTip (daemon_info_tooltip);
  daemon_label->setToolTip (daemon_info_tooltip);


  // scans used
  QLabel *scan_count_info_label = new QLabel ("Scans/APs:", stats_box);
  scan_count_label = new QLabel ("0/0   ", stats_box);
  //scan_count_label->setFixedWidth (scan_count_label->width());
  QString scan_count_tooltip ("Number of scans and access points the daemon is using to compute your position.");
  scan_count_info_label->setToolTip (scan_count_tooltip);
  scan_count_label->setToolTip (scan_count_tooltip);


  // scans rate
  QLabel *scan_rate_info_label = new QLabel ("Scan Rate:", stats_box);
  scan_rate_label = new QLabel ("   0s", stats_box);
  QString scan_rate_tooltip ("How often is the daemon using a new scan?");
  scan_rate_info_label->setToolTip (scan_rate_tooltip);
  scan_rate_label->setToolTip (scan_rate_tooltip);


  // walking?
  QLabel *walking_info_label = new QLabel ("Moving?", stats_box);
  walking_label = new QLabel ("No", stats_box);
  set_walking_label (false);
  QString walking_tooltip ("Does the daemon think you are moving?");
  walking_info_label->setToolTip (walking_tooltip);
  walking_label->setToolTip (walking_tooltip);

  // gps
  //QLabel *gps_info_label = new QLabel ("GPS:", stats_box);
  //QLabel *gps_label = new QLabel ("Off", stats_box);
  //gps_label->setStyleSheet ("QLabel { color: gray }");

  // cache / memory usage?
  //QLabel *cache_info_label = new QLabel ("<b>Cache</b>");
  //QLabel *cache_size_info_label = new QLabel ("Cache Size:", stats_box);
  //cache_size_label = new QLabel ("0 Kb");
  QLabel *cache_spaces_info_label = new QLabel ("Bld/Room:", stats_box);
  cache_spaces_label = new QLabel ("0/0   ");
  //cache_spaces_label->setFixedWidth (cache_spaces_label->width());
  QString cache_spaces_tooltip ("Number of buildings locally cached /\nNumber of resulting candidate rooms");
  cache_spaces_info_label->setToolTip (cache_spaces_tooltip);
  cache_spaces_label->setToolTip (cache_spaces_tooltip);


  // overlap algorithm
  QLabel *overlap_max_info_label = new QLabel ("Overlap Max:", stats_box);
  overlap_max_label = new QLabel ("0.0  ", stats_box);
  //overlap_max_label->setFixedWidth (overlap_max_label->width());
  QString overlap_max_tooltip ("Internal score of top estimated space (1=max)");
  overlap_max_info_label->setToolTip (overlap_max_tooltip);
  overlap_max_label->setToolTip (overlap_max_tooltip);


  QLabel *churn_info_label = new QLabel ("Churn:", stats_box);
  churn_label = new QLabel ("0s  ", stats_box);
  QString churn_tooltip ("Rate at which the room estimate is changing");
  churn_info_label->setToolTip (churn_tooltip);
  churn_label->setToolTip (churn_tooltip);


  stats_layout->addWidget (net_info_label, 0, 1);
  stats_layout->addWidget (net_label, 0, 2);

  stats_layout->addWidget (scan_count_info_label, 1, 1);
  stats_layout->addWidget (scan_count_label, 1, 2);

  stats_layout->addWidget (scan_rate_info_label, 2, 1);
  stats_layout->addWidget (scan_rate_label, 2, 2);

  stats_layout->addWidget (cache_spaces_info_label, 0, 4);
  stats_layout->addWidget (cache_spaces_label, 0, 5);

  stats_layout->addWidget (walking_info_label, 1, 4);
  stats_layout->addWidget (walking_label, 1, 5);

  stats_layout->addWidget (daemon_info_label, 2, 4);
  stats_layout->addWidget (daemon_label, 2, 5);


  stats_layout->addWidget (overlap_max_info_label, 0, 7);
  stats_layout->addWidget (overlap_max_label, 0, 8);

  stats_layout->addWidget (churn_info_label, 1, 7);
  stats_layout->addWidget (churn_label, 1, 8);

  QFrame *stat_frameA = new QFrame (stats_box);
  stat_frameA->setFrameStyle ( QFrame::VLine | QFrame::Sunken);
  stats_layout->addWidget (stat_frameA, 0, 3, 3, 1);

  QFrame *stat_frameB = new QFrame (stats_box);
  stat_frameB->setFrameStyle ( QFrame::VLine | QFrame::Sunken);
  stats_layout->addWidget (stat_frameB, 0, 6, 3, 1);

  stats_layout->setColumnStretch (9, 2);

  stats_box->setLayout (stats_layout);

  //////////////////////////////////////////////////////////
  layout->addWidget (estimate_box, 0, 0);
  layout->addWidget (button_box, 0, 1, 3, 1);
  layout->addWidget (stats_box, 1, 0);

  layout->setRowStretch (0, 10);
  layout->setRowStretch (1, 1);
  layout->setRowStretch (2, 1);

  // MAEMO
  layout->setColumnStretch (0, 10);
  layout->setColumnStretch (1, 1);



  setLayout (layout);


}

void Binder::handle_clicked_about() {
  qDebug () << "handle_clicked_about";

  // TODO get rid of done button
  // TODO problem in title

  //QString version = "About "+mole;
  QString version = "About Organic Indoor Positioning";

  QString about = 
    "<h2>Organic Indoor Positioning 0.2</h2>"
       "<p>Copyright &copy; 2010 Nokia Inc."
       "<p>Organic Indoor Positioning is a joint development from Massachusetts Institute of Technology and Nokia Research."
	"<p>By using this software, you accept its <a href=\"http://oil.nokiaresearch.com/privacy.html\">Privacy Policy</a> and <a href=\"http://oil.nokiaresearch.com/terms.html\">Terms of Service</a>.";

  // TODO check links
  //about.setOpenExternalLinks(true);

  QMessageBox::about 
       (this, version, about);

}

void Binder::handle_clicked_feedback() {
  qDebug () << "handle_clicked_feedback";
  bool ok;

  // TODO turn into multiline box

  QString text = QInputDialog::getText 
    (this, tr("Send Feedback to Developers"),
     NULL, QLineEdit::Normal, NULL, &ok);
  if (ok && !text.isEmpty()) {
    qDebug () << "feedback " << text;

    // TODO truncate - make sure text does not exceed some max length

    QNetworkRequest request;
    QString url_str = setting_server_url_value;
    QUrl url(url_str.append("/feedback"));
    request.setUrl(url);
    set_network_request_headers (request);
    feedback_reply = network_access_manager->post (request, text.toUtf8());
    connect (feedback_reply, SIGNAL(finished()), 
	     SLOT (handle_send_feedback_finished()));

  }
}

void Binder::handle_send_feedback_finished () {
  feedback_reply->deleteLater();
  if (feedback_reply->error() != QNetworkReply::NoError) {
    qWarning() << "sending feedback failed "
	       << feedback_reply->errorString()
      	       << " url " << feedback_reply->url();
  } else {
    qDebug() << "sending feedback succeeded";
  }

}


void Binder::handle_clicked_help() {
  qDebug () << "handle_clicked_help";

  QString help = 
    tr("<p>Help grow Organic Indoor Positioning by <b>binding</b> your current location.  Each bind links the name you pick with an RF signature.  When other Organic Indoor Positioning users are in the same place, they will see the same name.");

  QMessageBox::information
       (this, tr ("Why Bind?"), help);

}

void Binder::handle_clicked_settings() {
  qDebug () << "handle_clicked_settings";
  settings_dialog->show();
}

void Binder::set_places_enabled (bool isEnabled) {
  country_edit->setEnabled(isEnabled);
  region_edit->setEnabled(isEnabled);
  city_edit->setEnabled(isEnabled);
  area_edit->setEnabled(isEnabled);
  space_name_edit->setEnabled(isEnabled);
  tags_edit->setEnabled(isEnabled);
}

void Binder::set_submit_state (SubmitState _submit_state) {

  qDebug () << "set_submit_state"
	    << " current " << submit_state
	    << " new " << _submit_state;
  submit_state = _submit_state;

  switch (submit_state) {
  case DISPLAY:
    submit_button->setText (display_str);
    set_places_enabled (false);
    refresh_last_estimate ();
    bind_timer->stop ();
    break;
  case CORRECT:
    submit_button->setText (correct_str);
    set_places_enabled (true);
    reset_bind_timer ();
    break;
  }
}

void Binder::handle_place_changed () {
  qDebug () << "handle_place_changed ";

  reset_bind_timer ();

}

void Binder::handle_online_change (bool _online) {
  qDebug () << "handle_online_change " << _online;

  if (online != _online) {
    online = _online;
    if (online) {
      net_label->setText ("OK");
      net_label->setLevel (1);
    } else {
      net_label->setText ("Not OK");
      net_label->setLevel (-1);
    }


  }

}

void Binder::handle_daemon_change (bool daemon_online) {
  qDebug () << "handle_daemon_change " << daemon_online;

  if (daemon_online) {
    daemon_label->setText ("OK");
    daemon_label->setLevel (1);
  } else {
    daemon_label->setText ("Not OK");
    daemon_label->setLevel (-1);
  }
}



void Binder::request_location_estimate() {
  qDebug () << "Binder: request_location_estimate";

  request_location_estimate_counter++;
  if (request_location_estimate_counter > 5) {
    handle_daemon_change (false);
  }

  QDBusMessage msg = QDBusMessage::createSignal("/", "com.nokia.moled", "GetLocationEstimate");
  //msg << "foo";
  QDBusConnection::sessionBus().send(msg);
}

void Binder::handle_clicked_submit() {

  qDebug () << "handle_clicked_submit";

  switch (submit_state) {
  case DISPLAY:
    set_submit_state (CORRECT);

    break;
  case CORRECT:

    if (country_edit->text().isEmpty() ||
	region_edit->text().isEmpty() ||
	city_edit->text().isEmpty() ||
	area_edit->text().isEmpty() ||
	space_name_edit->text().isEmpty()) {

      QString empty_fields = 
	tr("Please fill in all fields when adding or repairing a location estimate (Tags are optional).");

      QMessageBox::information
	(this, tr ("Missing Fields"), empty_fields);


    } else {

      // confirm bind 

      QMessageBox bind_box;
      bind_box.setText ("Are you here?");

      QString est_str = area_edit->text() + " Rm: " + space_name_edit->text()
	+ "\n" + city_edit->text() + ", " + area_edit->text() + ", "
	+ country_edit->text() + "\n" + "Tags: " + tags_edit->text();
      bind_box.setInformativeText (est_str);
      bind_box.setStandardButtons (QMessageBox::Yes | QMessageBox::No);

      bind_box.setDefaultButton (QMessageBox::No);
      int ret = bind_box.exec ();
      switch (ret) {
      case (QMessageBox::Yes):
	send_bind_msg ();
	handle_bind_timer ();
	break;
      case (QMessageBox::No):
	reset_bind_timer ();
	break;
      }
    }
    break;
  }
}


void Binder::handle_bind_timer() {
  qDebug () << "handle_bind_timer";
  set_submit_state (DISPLAY);
}

void Binder::reset_bind_timer() {
  qDebug () << "reset_bind_timer";
  bind_timer->start (20000);
}


void Binder::send_bind_msg () {
  QDBusMessage msg = QDBusMessage::createSignal("/", "com.nokia.moled", "Bind");

  msg 
    << country_edit->text()
    << region_edit->text()
    << city_edit->text()
    << area_edit->text()
    << space_name_edit->text()
    << tags_edit->text();
  QDBusConnection::sessionBus().send(msg);

}

void Binder::handle_location_estimate(QString fq_name, bool _online) {
  qDebug () << "handle_location_estimate " << fq_name
	    << "online " << online;

  request_location_estimate_counter = 0;
  handle_daemon_change (true);

  if (submit_state == DISPLAY) {


    QStringList est_parts = fq_name.split ("/");

    qDebug () << "size " << est_parts.size();
    qDebug () << "part " << est_parts.at(0);

    if (est_parts.size() == 5) {
      country_estimate = est_parts.at(0);
      region_estimate = est_parts.at(1);
      city_estimate = est_parts.at(2);
      area_estimate = est_parts.at(3);
      space_name_estimate = est_parts.at(4);

      // TODO set tags

      refresh_last_estimate ();
    }



  } else {
    qDebug () << "not displaying new estimate because not in display mode";
  }

  request_location_timer->stop();
  handle_online_change (_online);

}

void Binder::refresh_last_estimate () {
    country_edit->setText (country_estimate);
    region_edit->setText (region_estimate);
    city_edit->setText (city_estimate);
    area_edit->setText (area_estimate);
    space_name_edit->setText (space_name_estimate);
    tags_edit->setText (tags_estimate);
}

void Binder::handle_speed_estimate(int moving_count) {
  qDebug () << "statistics got speed estimate" << moving_count;

  if (moving_count > 0) {
    set_walking_label (true);
    walking_timer->start (5000);
  } else {
    set_walking_label (false);
  }

}

void Binder::set_walking_label (bool is_walking) {
  if (is_walking) {
    walking_label->setText ("Yes");
    walking_label->setStyleSheet ("QLabel { color: green }");
  } else {
    walking_label->setText ("No");
    walking_label->setStyleSheet ("QLabel { color: white }");
  }
}

void Binder::handle_walking_timer () {
  walking_timer->stop ();
  set_walking_label (false);
}

void Binder::handle_location_stats
(QString /*fq_name*/, QDateTime /*start_time*/,
 int scan_queue_size,int macs_seen_size,int total_area_count,int /*total_space_count*/,int /*potential_area_count*/,int potential_space_count,int /*movement_detected_count*/,double scan_rate_ms,double emit_new_location_sec,double /*network_latency*/,double network_success_rate,double overlap_max,double overlap_diff) {

  qDebug () << "statistics";

    //qDebug () << "statistics got location stats queue=" << scan_queue_size
    //<< " active_macs="<< active_macs;
  // estimate_label->setText (fq_name);

  scan_count_label->setText (QString::number(scan_queue_size) 
			     + "/" + QString::number(macs_seen_size));
  cache_spaces_label->setText (QString::number(total_area_count) 
			     + "/" + QString::number(potential_space_count));
  scan_rate_label->setText (QString::number((int)(round(scan_rate_ms/1000)))+"s");

  overlap_max_label->setText(QString::number(overlap_max,'f',4));

  qDebug () << "emit_new_location_sec " << emit_new_location_sec;

  churn_label->setText (QString::number((int)(round(emit_new_location_sec)))+"s");

  qDebug () << "overlap " << overlap_max << " diff " << overlap_diff;
  qDebug () << "scan rate " << scan_rate_ms;

  qDebug () << "network_success_rate " << network_success_rate;

}
