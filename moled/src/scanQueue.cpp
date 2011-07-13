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

#include "scanQueue.h"

// Implements a circular queue of scans
// with a fixed number of readings per scan.

// The various scanners (maemo, network manager, symbian)
// add their raw readings here (via addReading)
// and then tell it when they have completed each scan
// (via scanCompleted)

const QRegExp LocallyAdministeredMAC ("^.[2367abef]:");
const QRegExp MacRegExp ("^[0-9a-f][0-9a-f]:[0-9a-f][0-9a-f]:[0-9a-f][0-9a-f]:[0-9a-f][0-9a-f]:[0-9a-f][0-9a-f]:[0-9a-f][0-9a-f]$");

ScanQueue::ScanQueue(QObject *parent, Localizer *_localizer,
		     int _maxActiveQueueLength) :
  QObject(parent), maxActiveQueueLength (_maxActiveQueueLength), localizer (_localizer) {

  qDebug () << "ScanQueue maxActiveQueueLength" << maxActiveQueueLength;

  movementDetected = false;
  activeScanCount = 0;
  currentScan = 0;
  currentReading = 0;
  responseRateTotal = 0;
  seenMacsSize = 0;

  // init scan queue
  clear (-1);

}

bool ScanQueue::addReading (QString mac, QString ssid, 
			    qint16 frequency, qint8 strength) {

  //qDebug () << "sQ addReading " << mac << ssid << "rssi="<<strength << "currentScan="<<currentScan;

  // virtual AP detection elimination

  /*
  LEAVING OFF

  simplest thing to do is to set to zero last digit
  and only accept the first one that we've seen
  and include the zeroed value in the sig

  have two piles:
  nonVirtualAPs, virtualAPs
  if in either pile, return answer
  take last digit and make it 0

  keep using original mac (without 0)  
  */
  mac = mac.toLower();
  // TODO convert any - to :

  if (seenMacs.contains (mac)) {
    qDebug () << "skipping duplicate mac" << mac;
  } else if (mac.contains (LocallyAdministeredMAC)) {
    qDebug () << "dropping locally administered MAC" << mac;
  } else if (!mac.contains (MacRegExp)) {
    qDebug () << "skipping non MAC" << mac;
  } else {

    if (currentReading < MAX_SCANQUEUE_READINGS) {

      //const QRegExp testMac ("00:24:36:a4:c3");
      //if (mac.contains (testMac)) {
      //mac = "test1";
      //qWarning ("swapping in test1 mac");
      //}
      //qDebug () << "sQ addReading scan" << currentScan << "reading" << currentReading;
      // stash this reading in the current scan
      // but do not apply it to the fingerprint yet.

      APDesc* ap = getAP (mac, ssid, frequency);

      //ap->incrementUse();
      //qDebug () << "sQ ap use count " << *ap << " incr";                      
      //ap->addSignalStrength (strength);
      //responseRateTotal++;
      //dirtyAPs.insert (ap);
      scans[currentScan].readings[currentReading].set (ap, strength);

      currentReading++;

      // if we have just started a new scan, set the time
      if (seenMacs.size() == 0) {
	//qDebug () << "setting time";
	scans[currentScan].timestamp = QDateTime::currentDateTime();
      }

      seenMacs.insert (mac);

      return true;
    } else {
      qWarning ("too many readings in this scan");
    }
  }
  return false;
}

bool ScanQueue::scanCompleted () {

  qDebug () << "sQ scanCompleted" << currentScan;
  //qDebug () << "sQ scanCompleted" << *this;

  currentReading = 0;

  if (seenMacs.size() < 0) {
    qDebug () << "scanQueue: found no readings";
    return false;
  }

  int previousSeenMacsSize = seenMacsSize;
  seenMacsSize = seenMacs.size();

  seenMacs.clear();

  qDebug () << "scanSize previous" << previousSeenMacsSize << "current" << seenMacsSize;


  // Check for duplicate scans.
  // Duplicate readings appear to "appear" in the same order
  // so we do not need to use a hash
  // (we can just check the same index)

  if (previousSeenMacsSize == seenMacsSize) {
    int previousScan = currentScan - 1;
    if (previousScan < 0) { 
      previousScan = MAX_SCANQUEUE_SCANS -1; 
    }
    if (scans[previousScan].state == ACTIVE) {
      bool duplicateScan = true;
      for (int i = 0; duplicateScan && i < MAX_SCANQUEUE_READINGS; i++) {
	APDesc* apC = scans[currentScan].readings[i].ap;
	APDesc* apP = scans[previousScan].readings[i].ap;
	if (apC != NULL && apP != NULL) {
	  if (apC->mac != apP->mac ||
	      scans[currentScan].readings[i].strength !=
	      scans[previousScan].readings[i].strength) {
	    duplicateScan = false;
	  }
	  /*
	  qDebug () << i << "c" << apC->mac
		    << scans[currentScan].readings[i].strength
		    << "p" << apP->mac
		    << scans[previousScan].readings[i].strength
		    << "ap" << (apC->mac == apP->mac)
		    << "rssi" 
		    << (scans[currentScan].readings[i].strength == 
			scans[previousScan].readings[i].strength);
	  */
	}
      }
      if (duplicateScan) {
	qDebug () << "rejecting duplicate scan";
	return false;
      }

    }
  }


  // apply the current readings to the fingerprint

  for (int i = 0; i < MAX_SCANQUEUE_READINGS; i++) {
    APDesc* ap = scans[currentScan].readings[i].ap;
    if (ap != NULL) {
      ap->incrementUse();
      ap->addSignalStrength (scans[currentScan].readings[i].strength);
      responseRateTotal++;
      dirtyAPs.insert (ap);
    }
  }


  if (movementDetected) {
    truncate ();
    movementDetected = false;
  }

  // TODO check for extra macs vs prior scans;

  // Use 'state' to prevent partial scans from being bound
  scans[currentScan].state = ACTIVE;
  activeScanCount++;


  // Here, we change any necessary values in the user's signature
  // We keep a set of dirty (changed) APs
  // and walk through them after we have added or substracted
  // any raw RSSI histogram readings.

  // Drop the oldest recent scan from the user's signature
  // if we are only keeping a limited number of active scans
  // and not using the motion detector
  if (maxActiveQueueLength > 0) {
    qDebug () << "starting expired";
    int expiringScan = currentScan - maxActiveQueueLength;
    if (expiringScan < 0) {
      expiringScan = MAX_SCANQUEUE_SCANS + expiringScan;
    }
    if (scans[expiringScan].state == ACTIVE) {
      for (int i = 0; i < MAX_SCANQUEUE_READINGS; i++) {
	APDesc* ap = scans[expiringScan].readings[i].ap;
	if (ap != NULL) {
	  ap->removeSignalStrength (scans[expiringScan].readings[i].strength);
	  responseRateTotal--;
	  if (ap->isEmpty()) {
	    localizer->fp()->remove 
	      (getAPString (ap->mac, ap->ssid, ap->frequency));
	  } else {
	    dirtyAPs.insert(ap);
	  }
	}
      }
      scans[expiringScan].state = INACTIVE;
      activeScanCount--;
    }
    qDebug () << "finished expired";
  }



  // Finished processing this scan.
  // Get ready for the next set of readings.

  currentScan++;
  if (currentScan >= MAX_SCANQUEUE_SCANS) {
    currentScan = 0;
  }

  // clean out any unused APs
  for (int i = 0; i < MAX_SCANQUEUE_READINGS; i++) {
    APDesc* ap = scans[currentScan].readings[i].ap;
    if (ap != NULL) {
      ap->decrementUse();
      if (ap->getUseCount() <= 0) {

	// note that the ap might not be in the sig if it was
	// expired by maxActiveQueueLength
	QString apString = getAPString (ap->mac, ap->ssid, ap->frequency);
	if (localizer->fp()->contains(apString)) {
	  localizer->fp()->remove (apString);
	}
	if (dirtyAPs.contains (ap)) {
	  dirtyAPs.remove (ap);
	}
	delete ap;
      } else if (scans[currentScan].state == ACTIVE) {
	ap->removeSignalStrength (scans[currentScan].readings[i].strength);
	responseRateTotal--;
	dirtyAPs.insert(ap);
      }
      scans[currentScan].readings[i].ap = NULL;
    }
  }
  if (scans[currentScan].state == ACTIVE) {
    activeScanCount--;
  }
  scans[currentScan].state = INCOMPLETE;

  // reset the normalized histogram for all changed histograms
  QSetIterator <APDesc*> dirtyIt (dirtyAPs);
  while (dirtyIt.hasNext()) {
    APDesc* ap = dirtyIt.next();
    ap->normalizeHistogram ();
  }
  dirtyAPs.clear ();

  // rebalance the weights for all APs
  QMapIterator <QString,APDesc*> apIt (*(localizer->fp()));
  while (apIt.hasNext()) {
    apIt.next();
    APDesc* apDesc = apIt.value();
    apDesc->setWeight (responseRateTotal);
  }

  // debugging activeScanCount
  int activeScanCountTest = 0;
  for (int i = 0; i < MAX_SCANQUEUE_SCANS; i++) {
    if (scans[i].state == ACTIVE) {
      activeScanCountTest++;
    }
  }
  Q_ASSERT (activeScanCount == activeScanCountTest);

  Q_ASSERT (responseRateTotal > 0);
  Q_ASSERT (responseRateTotal <= MAX_SCANQUEUE_READINGS*MAX_SCANQUEUE_SCANS);

  // We do not need to notify the binder at all.
  // We do tell the localizer however.
  localizer->localize (activeScanCount);

  return true;

}

void ScanQueue::serialize (QDateTime oldestValidScan,
			   QVariantList &scanList) {
  // walk from the oldest scan to the newest
  // skipping over any scans that are too old

  int scanIndex = currentScan;
  int scanCount = 0;
  int readingCount = 0;

  for (int i = 0; i < MAX_SCANQUEUE_SCANS; i++) {

    scanIndex++;
    if (scanIndex >= MAX_SCANQUEUE_SCANS) {
      scanIndex = 0;
    }

    if ((scans[scanIndex].state == ACTIVE ||
	 scans[scanIndex].state == INACTIVE) &&
	scans[scanIndex].timestamp > oldestValidScan) {
      QVariantList readingsList;
      for (int j = 0; j < MAX_SCANQUEUE_READINGS; j++) {
	APDesc* ap = scans[scanIndex].readings[j].ap;
	if (ap == NULL) {
	  // skip over ap that has since been deleted
	} else {
	  QVariantMap readingMap;
	  readingMap.insert ("bssid", ap->mac);
	  readingMap.insert ("ssid", ap->ssid);
	  readingMap.insert ("frequency", ap->frequency);
	  readingMap.insert ("level", scans[scanIndex].readings[j].strength);
	  readingsList << readingMap;
	  readingCount++;
	}
      }

      if (readingsList.size() > 0) {
	QVariantMap scanMap;
	scanMap.insert ("stamp", scans[scanIndex].timestamp.toTime_t());
	scanMap.insert ("readings", readingsList);
	scanList << scanMap;
	scanCount++;
      }
    }
  }

  qDebug () << "scanQueue serialize scanCount " << scanCount << "readingCount" << readingCount;

}

APDesc* ScanQueue::getAP (QString mac, QString ssid, 
			  qint16 frequency) {
  QString apString = getAPString (mac, ssid, frequency);
  if (localizer->fp()->contains (apString)) {
    return localizer->fp()->value(apString);
  }
  APDesc* ap = new APDesc (mac, ssid, frequency);
  localizer->fp()->insert(apString, ap);
  return ap;
}

QString ScanQueue::getAPString (QString mac, QString /*ssid*/, 
				qint16 /*frequency*/) {
  QString res = mac;
  // Can't add this other stuff as we directly use
  // this in in the call to getAreas (by localizer)
  //res.append ('/');
  //res.append (ssid);
  //res.append ('/');
  //res.append (QString::number(frequency));
  return res;
}

void ScanQueue::handleMotionEstimate(int motion) {
  qDebug () << "localizer got speed estimate" << motion;

  if (motion == MOVING) {
    movementDetected = true;
    localizer->movement_detected ();
  }

}

// scale back everything except for the current scan
void ScanQueue::truncate() {

  qDebug () << "sQ truncate start " << currentScan;

  // Create a new fingerprint object
  QMap<QString,APDesc*> *newFP = new QMap<QString,APDesc*> ();

  responseRateTotal = 0;
  activeScanCount = 0;
  dirtyAPs.clear ();

  // Copy just the current scan into the new fingerprint
  // (requires the outgoing fingerprint for the ap metadata)
  // Change the scan's pointers to the new fingerprint at the same time.
  for (int i = 0; i < MAX_SCANQUEUE_READINGS; i++) {
    //qDebug () << "sQ trunc A " << i;
    APDesc* oldAP = scans[currentScan].readings[i].ap;
    if (oldAP != NULL) {
      QString apString = getAPString (oldAP->mac, oldAP->ssid, oldAP->frequency);
      APDesc *newAP = new APDesc (oldAP->mac, oldAP->ssid, oldAP->frequency);
      newAP->incrementUse ();
      newAP->addSignalStrength (scans[currentScan].readings[i].strength);
      responseRateTotal++;
      //qDebug () << "sQ trunc B " << i << *newAP;
      dirtyAPs.insert (newAP);
      newFP->insert (apString, newAP);
      scans[currentScan].readings[i].ap = newAP;
    }

  }

  // Mark all of the other scans as inactive
  clear (currentScan);

  // Delete the outgoing fingerprint and
  // swap in the newly created fingerprint
  localizer->replaceFingerprint (newFP);

  //qDebug () << "sQ truncate end " << currentScan;

}

void ScanQueue::clear (int ignoreScan) {
  for (int i = 0; i < MAX_SCANQUEUE_SCANS; i++) {
    if (i != ignoreScan) {
      scans[i].state = INCOMPLETE;
      for (int j = 0; j < MAX_SCANQUEUE_READINGS; j++) {
	scans[i].readings[j].ap = NULL;
      }
    }
  }
}


/*

  void Localizer::emit_new_local_signature () {

  QDBusMessage msg = QDBusMessage::createSignal("/", "com.nokia.moled", "SignatureChanged");

  // serialize the current signature (OK, not very flexible, I admit)

  QStringList sigs;

  sigs << QString::number (active_macs->size());

  QMapIterator <QString,MacDesc*> i (*active_macs);
  while (i.hasNext()) {
  i.next();
  sigs << i.key();
  sigs << QString::number (i.value()->mean);
  sigs << QString::number (i.value()->stddev);
  sigs << QString::number (i.value()->weight);
  }

  msg << sigs;

  stats->add_ap_per_sig_count (active_macs->size());

  if (verbose) {
  qDebug () << "current sig " << sigs;
  }

  QDBusConnection::sessionBus().send(msg);

  }

  QString Localizer::handle_signature_request() {
  return "sig request";
  }

*/


QDebug operator<<(QDebug dbg, const Reading &reading) {
  dbg.nospace() << "[";
  if (reading.ap != NULL) {
    dbg.nospace() << *(reading.ap);
    dbg.nospace() << ",rssi="<< reading.strength;
  }
  dbg.nospace() << "]";
  return dbg.space();
}

QDebug operator<<(QDebug dbg, const Scan &scan) {
  dbg.nospace() << "[";
  switch (scan.state) {
  case INCOMPLETE:
    dbg.nospace() << "INCOMPLETE";
    break;
  case ACTIVE:
    dbg.nospace() << "ACTIVE";
    break;
  case INACTIVE:
    dbg.nospace() << "INACTIVE";
  }

  for (int i = 0; i < MAX_SCANQUEUE_READINGS; i++) {
    dbg.nospace() << scan.readings[i];
  }
  dbg.nospace() << scan.timestamp;
  dbg.nospace() << "]";
  return dbg.space();
}       

QDebug operator<<(QDebug dbg, const ScanQueue &s) {
  dbg.nospace() << "[";
  for (int i = 0; i < MAX_SCANQUEUE_SCANS; i++) {
    dbg.nospace() << s.scans[i];
    if (i == s.currentScan) {
      dbg.nospace() << " *";
    }
    dbg.nospace() << "\n";
  }
  dbg.nospace() << ", rr="<<s.responseRateTotal;
  dbg.nospace() << "]";
  return dbg.space();
}       
