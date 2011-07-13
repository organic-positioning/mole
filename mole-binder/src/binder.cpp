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

#include "places.h"
#include "settings.h"

// TODO show coverage of local area in color
// TODO have tags come from server

Binder::Binder(QWidget *parent)
  : QWidget(parent)
  , m_online(true)
  , m_requestLocationEstimateCounter(0)
  , m_bindTimer(new QTimer())
  , m_requestLocationTimer(new QTimer())
  , m_walkingTimer(new QTimer())
  , m_feedbackReply(0)
{

  buildUI();

  connect(m_bindTimer, SIGNAL(timeout()), SLOT(onBindTimeout()));

  setSubmitState(DISPLAY);

  connect(m_requestLocationTimer, SIGNAL(timeout()), SLOT(requestLocationEstimate()));
  m_requestLocationTimer->start(2000);

  connect(m_walkingTimer, SIGNAL(timeout()), SLOT(onWalkingTimeout()));

  QDBusConnection::sessionBus().connect
    (QString(), QString(), "com.nokia.moled", "LocationStats", this,
     SLOT(handleLocationStats(QString,QDateTime,int,int,int,int,int,
                              int,int,double,double,double,double,double,double)));

  QDBusConnection::sessionBus().connect
    (QString(), QString(), "com.nokia.moled", "LocationEstimate", this,
     SLOT(handleLocationEstimate(QString, bool)));

  QDBusConnection::sessionBus().connect
    (QString(), QString(), "com.nokia.moled", "MotionEstimate", this,
     SLOT(onSpeedStatusChanged(int)));
}

void Binder::handleQuit()
{
  qDebug() << "Binder: aboutToQuit";
  delete m_bindTimer;
  delete m_requestLocationTimer;
  delete m_walkingTimer;
}

Binder::~Binder()
{
  qDebug() << "Binder: destructor";
}

void Binder::buildUI()
{
  // LINUX
  resize(QSize(UI_WIDTH, UI_HEIGHT).expandedTo(minimumSizeHint()));

  QGridLayout *layout = new QGridLayout();

  QGroupBox *buttonBox = new QGroupBox("", this);
  QGridLayout *buttonLayout = new QGridLayout();

  // about
  QToolButton *aboutButton = new QToolButton(buttonBox);
  aboutButton->setIcon(QIcon(MOLE_ICON_PATH + "about.png"));
  aboutButton->setToolTip(tr("About"));
  aboutButton->setIconSize(QSize(ICON_SIZE_REG, ICON_SIZE_REG));
  connect(aboutButton, SIGNAL(clicked()), SLOT(onAboutClicked()));

  // feedback
  QToolButton *feedbackButton = new QToolButton(buttonBox);
  feedbackButton->setIcon(QIcon(MOLE_ICON_PATH + "email.png"));
  feedbackButton->setToolTip(tr("Send Feedback"));
  feedbackButton->setIconSize(QSize(ICON_SIZE_REG, ICON_SIZE_REG));
  connect(feedbackButton, SIGNAL(clicked()), SLOT(onFeedbackClicked()));

  // help
  QToolButton *helpButton = new QToolButton(buttonBox);
  helpButton->setIcon(QIcon(MOLE_ICON_PATH + "help.png"));
  helpButton->setToolTip(tr("Help"));
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
  spaceNameEdit = new PlaceEdit(areaEdit, 4, tr("Room"), estimateBox);

  tagsEdit = new QLineEdit(estimateBox);
  MultiCompleter *tagsCompleter = new MultiCompleter(this);
  tagsCompleter->setSeparator(QLatin1String(","));
  tagsCompleter->setCaseSensitivity(Qt::CaseInsensitive);

  countryEdit->setToolTip(tr("Country"));
  regionEdit->setToolTip(tr("Region"));
  cityEdit->setToolTip(tr("City"));
  areaEdit->setToolTip(tr("Area"));
  spaceNameEdit->setToolTip(tr("Space"));
  tagsEdit->setToolTip(tr("Tags"));

  qDebug() << "rootDir " << rootDir;
  QString tagsFile = rootDir.absolutePath().append("/map/tags.txt");
  QString urlStr = mapServerURL;
  QUrl tagsUrl(urlStr.append("/map/tags.txt"));
  UpdatingFileModel *tagModel = new UpdatingFileModel(tagsFile, tagsUrl, tagsCompleter);

  tagsCompleter->setModel(tagModel);
  tagsCompleter->setCaseSensitivity(Qt::CaseInsensitive);
  tagsEdit->setCompleter(tagsCompleter);

  tagsEdit->setMaxLength(80);
  tagsEdit->setPlaceholderText(tr("Tags..."));

  connect(countryEdit, SIGNAL(textChanged(QString)),
          SLOT(onPlaceChanged()));
  connect(regionEdit, SIGNAL(textChanged(QString)),
          SLOT(onPlaceChanged()));
  connect(cityEdit, SIGNAL(textChanged(QString)),
          SLOT(onPlaceChanged()));
  connect(areaEdit, SIGNAL(textChanged(QString)),
          SLOT(onPlaceChanged()));
  connect(spaceNameEdit, SIGNAL(textChanged(QString)),
          SLOT(onPlaceChanged()));
  connect(tagsEdit, SIGNAL(textChanged(QString)),
          SLOT(onPlaceChanged()));

  // TODO focus

  connect(countryEdit, SIGNAL(cursorPositionChanged(int,int)),
          SLOT(onPlaceChanged()));
  connect(regionEdit, SIGNAL(cursorPositionChanged(int,int)),
          SLOT(onPlaceChanged()));
  connect(cityEdit, SIGNAL(cursorPositionChanged(int,int)),
          SLOT(onPlaceChanged()));
  connect(areaEdit, SIGNAL(cursorPositionChanged(int,int)),
          SLOT(onPlaceChanged()));
  connect(spaceNameEdit, SIGNAL(cursorPositionChanged(int,int)),
          SLOT(onPlaceChanged()));
  connect(tagsEdit, SIGNAL(cursorPositionChanged(int,int)),
          SLOT(onPlaceChanged()));
  connect(countryEdit->model(), SIGNAL(network_change(bool)),
          SLOT(onNetworkStatusChanged(bool)));

  submitButton = new QPushButton(tr("Incorrect Estimate?"), estimateBox);
  submitButton->setToolTip(tr("Correct the current position estimate if"
                               "it is not showing where you are."));
  connect(submitButton, SIGNAL(clicked()), SLOT(onSubmitClicked()));

  QGridLayout *estimateLayout = new QGridLayout();

  QLabel *slashA = new QLabel("/", estimateBox);
  QLabel *slashB = new QLabel("/", estimateBox);
  QLabel *slashC = new QLabel("/", estimateBox);

  estimateLayout->addWidget(areaEdit, 1, 1);
  estimateLayout->addWidget(slashA, 1, 2);
  estimateLayout->addWidget(spaceNameEdit, 1, 3, 1, 3);

  estimateLayout->addWidget(cityEdit, 2, 1);
  estimateLayout->addWidget(slashB, 2, 2);
  estimateLayout->addWidget(regionEdit, 2, 3);
  estimateLayout->addWidget(slashC, 2, 4);
  estimateLayout->addWidget(countryEdit, 2, 5);

  estimateLayout->addWidget(tagsEdit, 3, 1);

  estimateLayout->addWidget(submitButton, 3, 3, 1, 3);

  estimateLayout->setColumnStretch(1,16);
  estimateLayout->setColumnStretch(2,0);
  estimateLayout->setColumnStretch(3,12);
  estimateLayout->setColumnStretch(4,0);
  estimateLayout->setColumnStretch(5,6);

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
  daemonLabel = new ColoredLabel(tr("OK"), statsBox);
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
}

void Binder::onAboutClicked()
{
  qDebug() << "handle clicked about";

  // TODO get rid of done button
  // TODO problem in title

  QString version = tr("About Organic Indoor Positioning");

  QString about =
    tr("<h2>Organic Indoor Positioning 0.4</h2>"
       "<p>Copyright &copy; 2011 Nokia Inc."
       "<p>Organic Indoor Positioning is a joint development from Massachusetts Institute of Technology and Nokia Research. "
       "By using this software, you accept its <a href=\"http://mole.research.nokia.com/policy/privacy.html\">Privacy Policy </a>"
       "and <a href=\"http://mole.research.nokia.com/policy/terms.html\">Terms of Service</a>.");

  // TODO check links

  QMessageBox::about(this, version, about);
}

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
    set_network_request_headers(request);
    m_feedbackReply = networkAccessManager->post(request, text.toUtf8());
    connect(m_feedbackReply, SIGNAL(finished()), SLOT(onSendFeedbackFinished()));
  }
}

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
  qDebug() << "handle clicked help";

  QString help =
    tr("<p>Help grow Organic Indoor Positioning by <b>binding</b> your current location. "
       "Each bind links the name you pick with an RF signature.  When other Organic Indoor "
       "Positioning users are in the same place, they will see the same name.");

  QMessageBox::information(this, tr("Why Bind?"), help);
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
  spaceNameEdit->setEnabled(isEnabled);
  tagsEdit->setEnabled(isEnabled);
}

void Binder::setSubmitState(SubmitState _submit_state)
{
  qDebug() << "set submit state"
           << " current " << m_submitState
           << " new " << _submit_state;
  m_submitState = _submit_state;

  switch (m_submitState) {
  case DISPLAY:
    submitButton->setText(tr("Incorrect Estimate?"));
    setPlacesEnabled(false);
    refreshLastEstimate();
    m_bindTimer->stop();
    break;
  case CORRECT:
    submitButton->setText(tr("Submit Correction"));
    setPlacesEnabled(true);
    resetBindTimer();
    break;
  }
}

void Binder::onPlaceChanged()
{
  qDebug() << "handle place changed ";

  resetBindTimer();
}

void Binder::onNetworkStatusChanged(bool _online)
{
  qDebug() << "network status changed " << _online;

  if (m_online != _online) {
    m_online = _online;
    if (m_online) {
      netLabel->setText(tr("OK"));
      netLabel->setLevel(1);
    } else {
      netLabel->setText(tr("Not OK"));
      netLabel->setLevel(-1);
    }
  }
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
}

void Binder::requestLocationEstimate()
{
  qDebug() << "Binder: request location estimate";

  ++m_requestLocationEstimateCounter;

  if (m_requestLocationEstimateCounter > 5)
    setDaemonLabel(false);

  QDBusMessage msg = QDBusMessage::createSignal("/", "com.nokia.moled", "GetLocationEstimate");
  QDBusConnection::sessionBus().send(msg);
}

void Binder::onSubmitClicked()
{
  qDebug() << "handle clicked submit";

  switch (m_submitState) {
  case DISPLAY:
    setSubmitState(CORRECT);
    break;
  case CORRECT:
    if (countryEdit->text().isEmpty() || regionEdit->text().isEmpty() ||
        cityEdit->text().isEmpty() || areaEdit->text().isEmpty() ||
        spaceNameEdit->text().isEmpty()) {

      QString empty_fields =
          tr("Please fill in all fields when adding or repairing a location estimate (Tags are optional).");

      QMessageBox::information(this, tr("Missing Fields"), empty_fields);
    } else {
      // confirm bind
      QMessageBox bindBox;
      bindBox.setText(tr("Are you here?"));

      QString estStr = areaEdit->text() + " Rm: " + spaceNameEdit->text()
                        + "\n" + cityEdit->text() + ", " + areaEdit->text() + ", "
                        + countryEdit->text() + "\n" + "Tags: " + tagsEdit->text();
      bindBox.setInformativeText(estStr);
      bindBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

      bindBox.setDefaultButton(QMessageBox::No);
      int ret = bindBox.exec();
      switch (ret) {
      case QMessageBox::Yes:
        sendBindMsg();
        onBindTimeout();
        break;
      case QMessageBox::No:
        resetBindTimer();
        break;
      }
    }
    break;
  }
}

void Binder::onBindTimeout()
{
  qDebug() << "handle bind timeout";
  setSubmitState(DISPLAY);
}

void Binder::resetBindTimer()
{
  qDebug() << "reset bind timer";
  m_bindTimer->start(20000);
}

void Binder::sendBindMsg()
{
  QDBusMessage msg = QDBusMessage::createSignal("/", "com.nokia.moled", "Bind");

  msg
    << countryEdit->text()
    << regionEdit->text()
    << cityEdit->text()
    << areaEdit->text()
    << spaceNameEdit->text()
    << tagsEdit->text();

  QDBusConnection::sessionBus().send(msg);
}

void Binder::handleLocationEstimate(QString fqName, bool online)
{
  qDebug() << "handle location estimate " << fqName
            << "online " << m_online;

  m_requestLocationEstimateCounter = 0;
  setDaemonLabel(true);

  if (m_submitState == DISPLAY) {
    QStringList estParts = fqName.split("/");

    qDebug() << "size " << estParts.size();
    qDebug() << "part " << estParts.at(0);

    if (estParts.size() == 5) {
      countryEstimate = estParts.at(0);
      regionEstimate = estParts.at(1);
      cityEstimate = estParts.at(2);
      areaEstimate = estParts.at(3);
      spaceNameEstimate = estParts.at(4);

      // TODO set tags
      refreshLastEstimate();
    }
  } else {
    qDebug() << "not displaying new estimate because not in display mode";
  }

  m_requestLocationTimer->stop();
  onNetworkStatusChanged(online);
}

void Binder::refreshLastEstimate()
{
  countryEdit->setText(countryEstimate);
  regionEdit->setText(regionEstimate);
  cityEdit->setText(cityEstimate);
  areaEdit->setText(areaEstimate);
  spaceNameEdit->setText(spaceNameEstimate);
  tagsEdit->setText(tagsEstimate);
}

void Binder::onSpeedStatusChanged(int motion)
{
  qDebug() << "statistics got speed estimate" << motion;

  if (motion == MOVING) {
    setWalkingLabel(true);
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
}

void Binder::onWalkingTimeout()
{
  qDebug() << "handle walking timeout";
  m_walkingTimer->stop();
  setWalkingLabel(false);
}

void Binder::handleLocationStats
(QString /*fqName*/, QDateTime /*startTime*/,
 int scanQueueSize,int macsSeenSize,int totalAreaCount,int /*totalSpaceCount*/,int /*potentialAreaCount*/,int potentialSpaceCount,int /*movementDetectedCount*/,double scanRateTime,double emitNewLocationSec,double /*networkLatency*/,double networkSuccessRate,double overlapMax,double overlapDiff)
{
  qDebug() << "statistics";

  scanCountLabel->setText(QString::number(scanQueueSize) +
                            "/" + QString::number(macsSeenSize));
  cacheSpacesLabel->setText(QString::number(totalAreaCount) +
                             "/" + QString::number(potentialSpaceCount));
  scanRateLabel->setText(QString::number((int)(round(scanRateTime/1000)))+"s");

  overlapMaxLabel->setText(QString::number(overlapMax,'f', 4));

  qDebug() << "emit new location sec " << emitNewLocationSec;

  churnLabel->setText(QString::number((int)(round(emitNewLocationSec)))+"s");

  qDebug() << "overlap " << overlapMax << " diff " << overlapDiff;
  qDebug() << "scan rate " << scanRateTime;
  qDebug() << "network_success_rate " << networkSuccessRate;
}
