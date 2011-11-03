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

#include "binder.h"

#include "places.h"
#include "settings.h"

// TODO show coverage of local area in color
// TODO have tags come from server

Binder::Binder(QWidget *parent)
  : QWidget(parent)
  , m_daemonOnline(false)
  , m_currentProximityUpdate(QByteArray())
  , m_proximityDialog(NULL)
  , m_rankedSpacesDialog(NULL)
  , m_feedbackReply(0)
{

  buildUI();

  connect(&m_bindTimer, SIGNAL(timeout()), SLOT(onBindTimeout()));

  setSubmitState(SCANNING);

  connect(&m_requestLocationTimer, SIGNAL(timeout()), SLOT(requestLocationEstimate()));
  m_requestLocationTimer.start(2000);

  connect(&m_walkingTimer, SIGNAL(timeout()), SLOT(onWalkingTimeout()));

  connect(&m_daemonHeartbeatTimer, SIGNAL(timeout()), SLOT(onDaemonTimerTimeout()));
  m_daemonHeartbeatTimer.start(15000);

  QDBusConnection::systemBus().connect
    (QString(), QString(), "com.nokia.moled", "LocationStats", this,
     SLOT(handleLocationStats(QString,QDateTime,int,int,int,int,int,
                              int,int,int,double,double,double,double,
			      double,QVariantMap)));

  QDBusConnection::systemBus().connect
    (QString(), QString(), "com.nokia.moled", "LocationEstimate", this,
     SLOT(handleLocationEstimate(QString)));

  QDBusConnection::systemBus().connect
    (QString(), QString(), "com.nokia.moled", "MotionEstimate", this,
     SLOT(onSpeedStatusChanged(int)));

  QDBusConnection::systemBus().connect
    (QString(), QString(), "com.nokia.moled", "ProximityUpdate", this,
     SLOT(handleProximityUpdate(QByteArray)));

}


void Binder::handleQuit()
{
  qDebug() << "Binder: aboutToQuit";
}


Binder::~Binder()
{
  qDebug() << "Binder: destructor";
}


void Binder::buildUI()
{

#ifdef Q_WS_MAEMO_5
  // not sure if these is needed
  resize(QSize(UI_WIDTH, UI_HEIGHT).expandedTo(minimumSizeHint()));
#endif

  QGridLayout *layout = new QGridLayout();

  QGroupBox *buttonBox = new QGroupBox("", this);
  QGridLayout *buttonLayout = new QGridLayout();

  // about
  QToolButton *aboutButton = new QToolButton(buttonBox);
  qDebug () << "icon " << MOLE_ICON_PATH << "about.png";
  aboutButton->setIcon(QIcon(MOLE_ICON_PATH + "about.png"));
  aboutButton->setToolTip(tr("About"));
  aboutButton->setIconSize(QSize(ICON_SIZE_REG, ICON_SIZE_REG));
  connect(aboutButton, SIGNAL(clicked()), SLOT(onAboutClicked()));

  // feedback
  QToolButton *feedbackButton = new QToolButton(buttonBox);
  feedbackButton->setIcon(QIcon(MOLE_ICON_PATH + "email.png"));
  feedbackButton->setToolTip(tr("Show nearby users"));
  feedbackButton->setIconSize(QSize(ICON_SIZE_REG, ICON_SIZE_REG));
  connect(feedbackButton, SIGNAL(clicked()), SLOT(onFeedbackClicked()));

  // help
  QToolButton *helpButton = new QToolButton(buttonBox);
  helpButton->setIcon(QIcon(MOLE_ICON_PATH + "help.png"));
  helpButton->setToolTip(tr("Show nearby spaces"));
  helpButton->setIconSize(QSize(ICON_SIZE_REG, ICON_SIZE_REG));
  connect(helpButton, SIGNAL(clicked()), SLOT(onHelpClicked()));

  // settings
  QToolButton *settingsButton = new QToolButton(buttonBox);
  settingsButton->setIcon(QIcon(MOLE_ICON_PATH + "settings.png"));
  settingsButton->setToolTip(tr("Settings"));
  settingsButton->setIconSize(QSize(ICON_SIZE_REG, ICON_SIZE_REG));
  connect(settingsButton, SIGNAL(clicked()), SLOT(onSettingsClicked()));

  //////////////////////////////////////////////////////////

  buttonLayout->setRowStretch(5, 2);
  buttonLayout->addWidget(aboutButton, 1, 0);
  buttonLayout->addWidget(settingsButton, 2, 0);
  buttonLayout->addWidget(helpButton, 3, 0);
  buttonLayout->addWidget(feedbackButton, 4, 0);
  buttonBox->setLayout(buttonLayout);

  QGroupBox *estimateBox = new QGroupBox(tr("Estimate"), this);
  countryEdit = new PlaceEdit(NULL, 0, tr("Country"), estimateBox);
  countryEdit->setMaxLength(3);
  countryEdit->setInputMask(">AAA");

  regionEdit = new PlaceEdit(countryEdit, 1, tr("Region/State"), estimateBox);
  cityEdit = new PlaceEdit(regionEdit, 2, tr("City"), estimateBox);
  areaEdit = new PlaceEdit(cityEdit, 3, tr("Area/Blg"), estimateBox);
  floorEdit = new PlaceEdit(areaEdit, 4, tr("Floor"), estimateBox);
  // constrain these to be numbers
  floorEdit->setMaxLength(3);
  floorEdit->setInputMask("###");
  spaceNameEdit = new PlaceEdit(floorEdit, 5, tr("Room"), estimateBox);

  /*
  tagsEdit = new QLineEdit(estimateBox);
  MultiCompleter *tagsCompleter = new MultiCompleter(this);
  tagsCompleter->setSeparator(QLatin1String(","));
  tagsCompleter->setCaseSensitivity(Qt::CaseInsensitive);
  */

  countryEdit->setToolTip(tr("Country"));
  regionEdit->setToolTip(tr("Region"));
  cityEdit->setToolTip(tr("City"));
  areaEdit->setToolTip(tr("Area"));
  floorEdit->setToolTip(tr("Floor"));
  spaceNameEdit->setToolTip(tr("Space"));
  //tagsEdit->setToolTip(tr("Tags"));

  qDebug() << "rootDir " << rootDir;
  /*
  QString tagsFile = rootDir.absolutePath().append("/map/tags.txt");
  QString urlStr = mapServerURL;
  QUrl tagsUrl(urlStr.append("/map/tags.txt"));
  UpdatingFileModel *tagModel = new UpdatingFileModel(tagsFile, tagsUrl, tagsCompleter);
  

  tagsCompleter->setModel(tagModel);
  tagsCompleter->setCaseSensitivity(Qt::CaseInsensitive);
  tagsEdit->setCompleter(tagsCompleter);

  tagsEdit->setMaxLength(80);
  tagsEdit->setPlaceholderText(tr("Tags..."));
  */

  connect(countryEdit, SIGNAL(textChanged(QString)),
          SLOT(onPlaceChanged()));
  connect(regionEdit, SIGNAL(textChanged(QString)),
          SLOT(onPlaceChanged()));
  connect(cityEdit, SIGNAL(textChanged(QString)),
          SLOT(onPlaceChanged()));
  connect(areaEdit, SIGNAL(textChanged(QString)),
          SLOT(onPlaceChanged()));
  connect(floorEdit, SIGNAL(textChanged(QString)),
          SLOT(onPlaceChanged()));
  connect(spaceNameEdit, SIGNAL(textChanged(QString)),
          SLOT(onPlaceChanged()));
  //connect(tagsEdit, SIGNAL(textChanged(QString)),
  //SLOT(onPlaceChanged()));

  // TODO focus

  connect(countryEdit, SIGNAL(cursorPositionChanged(int,int)),
          SLOT(onPlaceChanged()));
  connect(regionEdit, SIGNAL(cursorPositionChanged(int,int)),
          SLOT(onPlaceChanged()));
  connect(cityEdit, SIGNAL(cursorPositionChanged(int,int)),
          SLOT(onPlaceChanged()));
  connect(areaEdit, SIGNAL(cursorPositionChanged(int,int)),
          SLOT(onPlaceChanged()));
  connect(floorEdit, SIGNAL(cursorPositionChanged(int,int)),
          SLOT(onPlaceChanged()));
  connect(spaceNameEdit, SIGNAL(cursorPositionChanged(int,int)),
          SLOT(onPlaceChanged()));
  //connect(tagsEdit, SIGNAL(cursorPositionChanged(int,int)),
  //SLOT(onPlaceChanged()));

  connect (networkConfigurationManager, SIGNAL(onlineStateChanged(bool)),
	   this, SLOT(onlineStateChanged(bool)));

  submitButton = new QPushButton(tr("Edit"), estimateBox);
  submitButton->setToolTip(tr("Correct the current position estimate if"
                               "it is not showing where you are."));
  connect(submitButton, SIGNAL(clicked()), SLOT(onSubmitClicked()));

  confirmButton = new QPushButton(tr("Confirm"), estimateBox);
  confirmButton->setToolTip(tr("Confirm that the estimate being shown is where you really are."));
  connect(confirmButton, SIGNAL(clicked()), SLOT(onConfirmClicked()));

  QGridLayout *estimateLayout = new QGridLayout();

  QLabel *slashA = new QLabel("/", estimateBox);
  QLabel *slashB = new QLabel("/", estimateBox);
  QLabel *slashC = new QLabel("/", estimateBox);
  QLabel *slashD = new QLabel("/", estimateBox);

  estimateLayout->addWidget(areaEdit, 1, 1);
  estimateLayout->addWidget(slashA, 1, 2);
  estimateLayout->addWidget(floorEdit, 1, 3);
  estimateLayout->addWidget(slashB, 1, 4);
  estimateLayout->addWidget(spaceNameEdit, 1, 5, 1, 3);

  estimateLayout->addWidget(cityEdit, 2, 1);
  estimateLayout->addWidget(slashC, 2, 2);
  estimateLayout->addWidget(regionEdit, 2, 3, 1, 3);
  estimateLayout->addWidget(slashD, 2, 6);
  estimateLayout->addWidget(countryEdit, 2, 7);

  //estimateLayout->addWidget(tagsEdit, 3, 1);
  estimateLayout->addWidget(confirmButton, 3, 1, 1, 2);
  estimateLayout->addWidget(submitButton, 3, 3, 1, 5);

  estimateLayout->setColumnStretch(1,16);
  estimateLayout->setColumnStretch(2,0);
  estimateLayout->setColumnStretch(3,6);
  estimateLayout->setColumnStretch(4,0);
  estimateLayout->setColumnStretch(5,6);
  estimateLayout->setColumnStretch(6,0);
  estimateLayout->setColumnStretch(7,6);

  estimateBox->setLayout(estimateLayout);

  //////////////////////////////////////////////////////////
  QGroupBox *statsBox = new QGroupBox(tr("Statistics"), this);
  QGridLayout *statsLayout = new QGridLayout();
  // grey  235 233 216  EB E9 D8
  // blue  000 051 204  00 33 CC
  // green 068 165 028  44 A5 1C
  // border: 0px; padding 0px;

  // network connectivity
  QLabel *netInfoLabel = new QLabel(tr("Net:"), statsBox);
  netLabel = new ColoredLabel(tr("OK"), statsBox);

  QString netInfoTooltip(tr("Network Connectivity Status.\nCan we talk to the network?"));
  netInfoLabel->setToolTip(netInfoTooltip);
  netLabel->setToolTip(netInfoTooltip);

  // daemon connectivity
  QLabel *daemonInfoLabel = new QLabel(tr("Daemon:"), statsBox);
  daemonLabel = new ColoredLabel(tr("??"), statsBox);
  QString daemonInfoTooltip(tr("Daemon Status.  Is the positioning daemon started?"));
  daemonInfoLabel->setToolTip(daemonInfoTooltip);
  daemonLabel->setToolTip(daemonInfoTooltip);

  // scans used
  QLabel *scanCountInfoLabel = new QLabel(tr("Scans/APs:"), statsBox);
  scanCountLabel = new QLabel("0/0   ", statsBox);
  QString scanCountTooltip(tr("Number of scans and access points the daemon is using to compute your position."));
  scanCountInfoLabel->setToolTip(scanCountTooltip);
  scanCountLabel->setToolTip(scanCountTooltip);

  // scans rate
  QLabel *scanRateInfoLabel = new QLabel(tr("Scan Rate:"), statsBox);
  scanRateLabel = new QLabel("   0s", statsBox);
  QString scanRateTooltip(tr("How often is the daemon using a new scan?"));
  scanRateInfoLabel->setToolTip(scanRateTooltip);
  scanRateLabel->setToolTip(scanRateTooltip);

  // walking?
  QLabel *walkingInfoLabel = new QLabel(tr("Moving?"), statsBox);
  walkingLabel = new QLabel(tr("No"), statsBox);
  setWalkingLabel(false);
  QString walkingTooltip(tr("Does the daemon think you are moving?"));
  walkingInfoLabel->setToolTip(walkingTooltip);
  walkingLabel->setToolTip(walkingTooltip);

  QLabel *cacheSpacesInfoLabel = new QLabel(tr("Bld/Room:"), statsBox);
  cacheSpacesLabel = new QLabel("0/0   ");
  QString cacheSpacesTooltip(tr("Number of buildings locally cached /\nNumber of resulting candidate rooms"));
  cacheSpacesInfoLabel->setToolTip(cacheSpacesTooltip);
  cacheSpacesLabel->setToolTip(cacheSpacesTooltip);

  // overlap algorithm
  QLabel *overlapMaxInfoLabel = new QLabel(tr("Overlap Max:"), statsBox);
  overlapMaxLabel = new QLabel("0.0  ", statsBox);
  QString overlapMaxTooltip(tr("Internal score of top estimated space (1=max)"));
  overlapMaxInfoLabel->setToolTip(overlapMaxTooltip);
  overlapMaxLabel->setToolTip(overlapMaxTooltip);

  QLabel *churnInfoLabel = new QLabel(tr("Churn:"), statsBox);
  churnLabel = new QLabel("0s  ", statsBox);
  QString churnTooltip(tr("Rate at which the room estimate is changing"));
  churnInfoLabel->setToolTip(churnTooltip);
  churnLabel->setToolTip(churnTooltip);

  statsLayout->addWidget(netInfoLabel, 0, 1);
  statsLayout->addWidget(netLabel, 0, 2);

  statsLayout->addWidget(scanCountInfoLabel, 1, 1);
  statsLayout->addWidget(scanCountLabel, 1, 2);

  statsLayout->addWidget(scanRateInfoLabel, 2, 1);
  statsLayout->addWidget(scanRateLabel, 2, 2);

  statsLayout->addWidget(cacheSpacesInfoLabel, 0, 4);
  statsLayout->addWidget(cacheSpacesLabel, 0, 5);

  statsLayout->addWidget(walkingInfoLabel, 1, 4);
  statsLayout->addWidget(walkingLabel, 1, 5);

  statsLayout->addWidget(daemonInfoLabel, 2, 4);
  statsLayout->addWidget(daemonLabel, 2, 5);


  statsLayout->addWidget(overlapMaxInfoLabel, 0, 7);
  statsLayout->addWidget(overlapMaxLabel, 0, 8);

  statsLayout->addWidget(churnInfoLabel, 1, 7);
  statsLayout->addWidget(churnLabel, 1, 8);

  QFrame *statFrameA = new QFrame(statsBox);
  statFrameA->setFrameStyle(QFrame::VLine | QFrame::Sunken);
  statsLayout->addWidget(statFrameA, 0, 3, 3, 1);

  QFrame *statFrameB = new QFrame(statsBox);
  statFrameB->setFrameStyle(QFrame::VLine | QFrame::Sunken);
  statsLayout->addWidget(statFrameB, 0, 6, 3, 1);

  statsLayout->setColumnStretch(9, 2);

  statsBox->setLayout(statsLayout);

  //////////////////////////////////////////////////////////
  layout->addWidget(estimateBox, 0, 0);
  layout->addWidget(buttonBox, 0, 1, 3, 1);
  layout->addWidget(statsBox, 1, 0);

  layout->setRowStretch(0, 10);
  layout->setRowStretch(1, 1);
  layout->setRowStretch(2, 1);

  // MAEMO
  layout->setColumnStretch(0, 10);
  layout->setColumnStretch(1, 1);

  setLayout(layout);

  /*
  rankMsgBox = new QMessageBox(this);
  rankMsgBox->setVisible(false);
  rankMsgBox->setWindowTitle (tr("Nearby Spaces"));
  rankMsgBoxButton = new QPushButton(tr("Done"), rankMsgBox);
  rankMsgBox->addButton (rankMsgBoxButton, QMessageBox::AcceptRole);
  //connect(rankMsgBoxButton, SIGNAL(clicked()), SLOT(onRankMsgBoxButtonClicked()));
  */
  //binderUi.show();
}


void Binder::onAboutClicked()
{
  qDebug() << "handle clicked about";

  // TODO get rid of done button
  // TODO problem in title

  QString version = tr("About Organic Indoor Positioning");

  QString about = "<h2>Organic Indoor Positioning ";
  about.append (MOLE_VERSION);
  about.append ("</h2>");
  about.append ("<p>Copyright &copy; 2011 Nokia Inc.");
  about.append ("<p>Organic Indoor Positioning is a joint development from Massachusetts Institute of Technology and Nokia Research.");
  about.append ("<p>Help grow Organic Indoor Positioning by <b>binding</b> your current location. "
       "Each bind links the name you pick with a WiFi signature.  When other Organic Indoor "
       "Positioning users are in the same place, they will see the same name.");


  //"By using this software, you accept its <a href=\"http://mole.research.nokia.com/policy/privacy.html\">Privacy Policy </a>"
  //"and <a href=\"http://mole.research.nokia.com/policy/terms.html\">Terms of Service</a>.");

  QMessageBox::about(this, version, about);
}

void Binder::onFeedbackClicked()
{
  qDebug () << "onFeedbackClicked";
  /*
  qDebug() << "handle clicked feedback";
  int ret = QMessageBox::information
    (this, tr("Top Nearby Spaces"),
     "A list of spaces..<BR>More spaces<BR>More");
  qDebug() << "onFeedbackClicked ret" << ret;
  */
  //rankMsgBox->setVisible(true);

  QString title = "Proximity - Who is Nearby?";
  if (!getProximityActive()) {
    title.append (" (inactive)");
  }
  QStringList headerNames;
  headerNames << "Display Name";
  headerNames << "Proximity";
  qDebug () << "creating UpdatingDisplayDialog";

  m_proximityDialog = new UpdatingDisplayDialog(this, title, headerNames);
  qDebug () << "created UpdatingDisplayDialog";

  m_proximityDialog->handleUpdate(m_currentProximityUpdate);
  m_proximityDialog->show();

  connect(m_proximityDialog, SIGNAL(rejected()), this, SLOT(onProximityClosed()));

}

void Binder::onProximityClosed()
{
  qDebug() << "handle closed proximity";
  m_proximityDialog->disconnect();
  delete m_proximityDialog;
  m_proximityDialog = NULL;
}

void Binder::handleProximityUpdate(QByteArray rawJson) {
  m_currentProximityUpdate = rawJson;
  if (m_proximityDialog != NULL) {
    m_proximityDialog->handleUpdate(rawJson);
  }
}


/*
void Binder::onRankMsgBoxButtonClicked()
{
  rankMsgBox->setVisible(false);
}
*/

/*
  // please keep this
void Binder::onFeedbackClicked()
{
  qDebug() << "handle clicked feedback";
  bool ok;

  // TODO turn into multiline box

  QString text = QInputDialog::getText
    (this, tr("Send Feedback to Developers"),
     NULL, QLineEdit::Normal, NULL, &ok);

  if (ok && !text.isEmpty()) {
    qDebug() << "feedback " << text;

    // TODO truncate - make sure text does not exceed some max length
    QNetworkRequest request;
    QString urlStr = mapServerURL;
    QUrl url(urlStr.append("/feedback"));
    request.setUrl(url);
    setNetworkRequestHeaders(request);
    m_feedbackReply = networkAccessManager->post(request, text.toUtf8());
    connect(m_feedbackReply, SIGNAL(finished()), SLOT(onSendFeedbackFinished()));
  }
}
*/

void Binder::onSendFeedbackFinished()
{
  m_feedbackReply->deleteLater();
  if (m_feedbackReply->error() != QNetworkReply::NoError) {
    qWarning() << "sending feedback failed "
               << m_feedbackReply->errorString()
               << " url " << m_feedbackReply->url();
  } else {
    qDebug() << "sending feedback succeeded";
  }
}

void Binder::onHelpClicked()
{
  qDebug() << "onHelpClicked";

  QStringList headerNames;
  headerNames << "Place";
  headerNames << "Similarity";
  m_rankedSpacesDialog = new UpdatingDisplayDialog(this, QString("Nearby Spaces"), headerNames);
  m_rankedSpacesDialog->handleUpdate(m_currentProximityUpdate);
  m_rankedSpacesDialog->show();
  connect(m_rankedSpacesDialog, SIGNAL(rejected()), this, SLOT(onRankedSpacesClosed()));

}

void Binder::onRankedSpacesClosed()
{
  qDebug() << "handle closed proximity";
  m_rankedSpacesDialog->disconnect();
  delete m_rankedSpacesDialog;
  m_rankedSpacesDialog = NULL;
}


void Binder::onSettingsClicked()
{
  qDebug() << "handle clicked settings";
  m_settingsDialog = new Settings(this);
  m_settingsDialog->show();

  connect(m_settingsDialog, SIGNAL(rejected()), this, SLOT(onSettingsClosed()));
}

void Binder::onSettingsClosed()
{
  qDebug() << "handle closed settings";
  m_settingsDialog->disconnect();
  delete m_settingsDialog;
}

void Binder::setPlacesEnabled(bool isEnabled)
{
  countryEdit->setEnabled(isEnabled);
  regionEdit->setEnabled(isEnabled);
  cityEdit->setEnabled(isEnabled);
  areaEdit->setEnabled(isEnabled);
  floorEdit->setEnabled(isEnabled);
  spaceNameEdit->setEnabled(isEnabled);
  //tagsEdit->setEnabled(isEnabled);
}

void Binder::setSubmitState(SubmitState _submit_state, int scanCount)
{
  qDebug() << "set submit state"
           << "current" << m_submitState
           << "new" << _submit_state
	   << "scanCount" << scanCount;
  m_submitState = _submit_state;
  QString scanStr = tr("Scanning");

  switch (m_submitState) {
  case SCANNING:
    for (int i = 0; i < scanCount; i++) {
      scanStr.append (".");
    }
    submitButton->setText(scanStr);
    setPlacesEnabled(false);
    refreshLastEstimate();
    submitButton->setEnabled(false);
    confirmButton->setEnabled(false);
    break;
  case DISPLAY:
    submitButton->setText(tr("Edit"));
    setPlacesEnabled(false);
    refreshLastEstimate();
    m_bindTimer.stop();
    submitButton->setEnabled(true);
    confirmButton->setEnabled(true);
    break;
  case EDITING:
    submitButton->setText(tr("Submit Edit"));
    setPlacesEnabled(true);
    resetBindTimer();
    submitButton->setEnabled(true);
    confirmButton->setEnabled(false);
    break;
  }
  qDebug() << "set submit state finished";
}

void Binder::onPlaceChanged()
{
  qDebug() << "handle place changed ";

  resetBindTimer();
}

void Binder::onlineStateChanged(bool _online)
{
  qDebug() << "Binder network status changed " << _online;

  //if (m_networkOnline != _online) {
  //m_networkOnline = _online;
    if (_online) {
      netLabel->setText(tr("OK"));
      netLabel->setLevel(1);
    } else {
      netLabel->setText(tr("Not OK"));
      netLabel->setLevel(-1);
    }
    //}
    //binderUi.setNetworkStatus(_online);
}

void Binder::setDaemonLabel(bool online)
{
  qDebug() << "set daemon label " << online;

  if (online) {
    daemonLabel->setText(tr("OK"));
    daemonLabel->setLevel(1);
  } else {
    daemonLabel->setText(tr("Not OK"));
    daemonLabel->setLevel(-1);
  }
  //binderUi.setNetworkStatus(online);
}

void Binder::onDaemonTimerTimeout () {
  qDebug () << "onDaemonTimerTimeout";
  if (!m_daemonOnline) {
    setSubmitState (SCANNING);
  }
  setDaemonLabel (m_daemonOnline);
  m_daemonOnline = false;
}

void Binder::receivedDaemonMsg () {
  qDebug () << "receivedDaemonMsg";
  if (!m_daemonOnline) {
    m_daemonOnline = true;
    setDaemonLabel (m_daemonOnline);
  }
}

void Binder::requestLocationEstimate()
{
  qDebug() << "Binder: request location estimate";

  QDBusMessage msg = QDBusMessage::createSignal("/", "com.nokia.moled", "GetLocationEstimate");
  QDBusConnection::systemBus().send(msg);
}

void Binder::onConfirmClicked()
{
  qDebug() << "handle confirm submit";
  //confirmBind(BindSource::VALIDATE);
  confirmBind(VALIDATE);
}

void Binder::onSubmitClicked()
{
  qDebug() << "handle clicked submit";

  switch (m_submitState) {
  case SCANNING:
    qFatal ("submit button should not be enabled when SCANNING");
    break;
  case DISPLAY:
    setSubmitState(EDITING);
    break;
  case EDITING:
    if (countryEdit->text().isEmpty() || regionEdit->text().isEmpty() ||
        cityEdit->text().isEmpty() || areaEdit->text().isEmpty() ||
	floorEdit->text().isEmpty() ||
        spaceNameEdit->text().isEmpty()) {

      QString empty_fields =
          tr("Please fill in all fields when adding or repairing a location estimate.");

      QMessageBox::information(this, tr("Missing Fields"), empty_fields);
    } else {

      //confirmBind (BindSource::FIX);
      confirmBind (FIX);

    }
    break;
  }
}

void Binder::confirmBind(BindSource source) {
      // confirm bind
      QMessageBox bindBox;
      bindBox.setText(tr("Are you here?"));

      QString estStr = areaEdit->text() + " Rm: " + spaceNameEdit->text()
	+ " Floor: "+ floorEdit->text()
	+ "\n" + cityEdit->text() + ", " + areaEdit->text() + ", "
	+ countryEdit->text() + "\n";
      //+ "Tags: " + tagsEdit->text();

      bindBox.setInformativeText(estStr);
      bindBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

      bindBox.setDefaultButton(QMessageBox::No);
      int ret = bindBox.exec();
      switch (ret) {
      case QMessageBox::Yes:
        sendBindMsg(source);
        onBindTimeout();
        break;
      case QMessageBox::No:
        resetBindTimer();
        break;
      }

}

void Binder::onBindTimeout()
{
  qDebug() << "onBindTimeout";
  if (m_submitState == EDITING) {
    setSubmitState(DISPLAY);
  }
}


void Binder::resetBindTimer()
{
  qDebug() << "reset bind timer";
  m_bindTimer.start(20000);
}


void Binder::sendBindMsg(BindSource source)
{
  QDBusMessage msg = QDBusMessage::createSignal("/", "com.nokia.moled", "Bind");

  QString tagsIgnored = "";
  switch (source) {
  case FIX:
    msg << "fix";
    break;
  case ADD:
    msg << "add";
    break;
  case VALIDATE:
    msg << "validate";
    break;
  case REMOVE:
    msg << "remove";
    break;
  }

    msg 
    << countryEdit->text()
    << regionEdit->text()
    << cityEdit->text()
    << areaEdit->text()
    << floorEdit->text().toInt()
    << spaceNameEdit->text()
    << tagsIgnored;
    //    << tagsEdit->text();

  QDBusConnection::systemBus().send(msg);
  qDebug () << "sendBindMsg sent";
}

void Binder::handleLocationEstimate(QString fqName)
{
  qDebug() << "handle location estimate " << fqName;

  receivedDaemonMsg ();

  if (m_submitState == DISPLAY || m_submitState == SCANNING) {
    QStringList estParts = fqName.split("/");

    qDebug() << "size " << estParts.size();
    qDebug() << "part " << estParts.at(0);

    if (estParts.size() == 6) {
      countryEstimate = estParts.at(0);
      regionEstimate = estParts.at(1);
      cityEstimate = estParts.at(2);
      areaEstimate = estParts.at(3);
      floorEstimate = estParts.at(4);
      spaceNameEstimate = estParts.at(5);
    } else if (fqName == "??") {
      countryEstimate = "??";
      regionEstimate = "??";
      cityEstimate = "??";
      areaEstimate = "??";
      floorEstimate = "0";
      spaceNameEstimate = "??";
    }
    // TODO set tags
    refreshLastEstimate(); 
    //binderUi.updateLocation(countryEstimate, regionEstimate, cityEstimate,
    //areaEstimate, floorEstimate, spaceNameEstimate);


  } else {
    qDebug() << "not displaying new estimate because not in display mode";
  }

  m_requestLocationTimer.stop();
}


void Binder::refreshLastEstimate()
{
  countryEdit->setText(countryEstimate);
  regionEdit->setText(regionEstimate);
  cityEdit->setText(cityEstimate);
  areaEdit->setText(areaEstimate);
  floorEdit->setText(floorEstimate);
  spaceNameEdit->setText(spaceNameEstimate);
  //tagsEdit->setText(tagsEstimate);
}


void Binder::onSpeedStatusChanged(int motion)
{
  qDebug() << "statistics got speed estimate" << motion;
  receivedDaemonMsg ();

  if (motion == MOVING) {
    setWalkingLabel(true);
    spaceNameEstimate = "??";
    refreshLastEstimate();
    setSubmitState(SCANNING);
  } else {
    setWalkingLabel(false);
  }
}


void Binder::setWalkingLabel(bool isWalking)
{
  if (isWalking) {
    walkingLabel->setText(tr("Yes"));
    walkingLabel->setStyleSheet("QLabel { color: green }");
  } else {
    walkingLabel->setText(tr("No"));
    walkingLabel->setStyleSheet("QLabel { color: white }");
  }
  //binderUi.setMotionStats(isWalking);
}


void Binder::onWalkingTimeout()
{
  qDebug() << "handle walking timeout";
  m_walkingTimer.stop();
  setWalkingLabel(false);
}


void Binder::handleLocationStats
(QString /*fqName*/, QDateTime /*startTime*/,
 int scanQueueSize,int macsSeenSize,int totalAreaCount,int /*totalSpaceCount*/,int /*potentialAreaCount*/,int potentialSpaceCount,int /*movementDetectedCount*/,int scanRateTime,double emitNewLocationSec,double /*networkLatency*/,double networkSuccessRate,double overlapMax,double overlapDiff,QVariantMap rankEntries)
{
  qDebug() << "statistics";
  receivedDaemonMsg ();

  scanCountLabel->setText(QString::number(scanQueueSize) +
                            "/" + QString::number(macsSeenSize));
  cacheSpacesLabel->setText(QString::number(totalAreaCount) +
                             "/" + QString::number(potentialSpaceCount));
  scanRateLabel->setText(QString::number(scanRateTime)+"s");

  overlapMaxLabel->setText(QString::number(overlapMax,'f', 4));

  qDebug() << "emit new location sec " << emitNewLocationSec;

  churnLabel->setText(QString::number((int)(round(emitNewLocationSec)))+"s");

  qDebug() << "overlap " << overlapMax << " diff " << overlapDiff;
  qDebug() << "scan rate " << scanRateTime;
  qDebug() << "network_success_rate " << networkSuccessRate;

  if (m_rankedSpacesDialog != NULL) {
    m_rankedSpacesDialog->handleUpdate(rankEntries);
  }

  if (scanQueueSize >= MIN_SCANS_TO_BIND) {
    if (m_submitState == SCANNING) {
      setSubmitState (DISPLAY);
    }
  } else {
    qDebug () << "statsScanQueue" << scanQueueSize;
    setSubmitState (SCANNING, scanQueueSize);
  }

  //binderUi.setLocationStats(scanCountLabel->text(), cacheSpacesLabel->text(), scanRateLabel->text(), overlapMaxLabel->text(), churnLabel->text());
}
