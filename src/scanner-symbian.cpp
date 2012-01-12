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

#include "scanner_symbian.h"

#include <e32base.h>
#include <wlanmgmtclient.h>
#include <wlanscaninfo.h>

QList<APReading*> Scanner::availableAPs()
{
  QList<APReading*> wlans;
  QString apName;
  QString apMac;
  short int apLevel;

  TUint8 ieLen = 0;
  const TUint8* ieData = NULL;
  TBuf<1000> buf16;
  TWlanBssid bssid;
  TInt retVal;

  CWlanScanInfo* pScanInfo = CWlanScanInfo::NewL();
  CleanupStack::PushL(pScanInfo);
  CWlanMgmtClient* pMgmtClient = CWlanMgmtClient::NewL();
  CleanupStack::PushL(pMgmtClient);
  pMgmtClient->GetScanResults(*pScanInfo);

  for(pScanInfo->First(); !pScanInfo->IsDone(); pScanInfo->Next() ) {
    apLevel = pScanInfo->RXLevel();

    retVal = pScanInfo->InformationElement(0, ieLen, &ieData);
    if (retVal == KErrNone) {
      TPtrC8 ptr(ieData, ieLen);
      buf16.Copy(ptr);
      apName = QString::fromUtf16(buf16.Ptr(),buf16.Length());
    }

    pScanInfo->Bssid(bssid);
    buf16.SetLength(0);
    for(TInt i = 0; i < bssid.Length(); i++)
      buf16.AppendFormat(_L("%02x:"), bssid[i]);

    if (buf16.Length() > 0)
      buf16.Delete(buf16.Length()-1, 1);

    apMac = QString::fromUtf16(buf16.Ptr(),buf16.Length());
    APReading *wlan = new APReading(apName, apMac, apLevel);
    wlans << wlan;
  }

  CleanupStack::PopAndDestroy(pMgmtClient);
  CleanupStack::PopAndDestroy(pScanInfo);

  return wlans;
}

APReading::APReading(QString _name, QString _mac, short int _level)
  : name(_name)
  , mac(_mac)
  , level(_level)
{
}

APReading::~APReading()
{
}
