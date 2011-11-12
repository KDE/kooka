/* This file is part of the KDE Project
   Copyright (C) 2009 Jonathan Marten <jjm@keelhaul.me.uk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "scanglobal.h"

#include <kdebug.h>
#include <klocale.h>
#include <kglobal.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kstandarddirs.h>

#include "kscandevice.h"

extern "C" {
#include <sane/sane.h>
}


/* Configuration-file definitions */
#define SCANNER_DB_FILE		"scannerrc"
#define GROUP_GENERAL		"Startup"		// for historical reasons


static ScanGlobal *sInstance = NULL;
static KScanDevice *sScanDevice = NULL;


ScanGlobal *ScanGlobal::self()
{
    if (sInstance==NULL) sInstance = new ScanGlobal();
    return (sInstance);
}


ScanGlobal::ScanGlobal()
{
    kDebug();

    mSaneInitDone = false;
    mSaneInitError = false;

    mScanConfig = NULL;

    // Get SANE translations - bug 98150
    KGlobal::dirs()->addResourceDir("locale", "/usr/share/locale/");
    KGlobal::locale()->insertCatalog("sane-backends");
}


ScanGlobal::~ScanGlobal()
{
    if (mSaneInitDone)
    {
        kDebug() << "calling sane_exit()";
        sane_exit();
    }

    if (mScanConfig!=NULL)
    {
        mScanConfig->sync();
        delete mScanConfig;
    }
}


//  SANE authorisation callback for scanner access.
//
//  The scanner device in use (a KScanDevice) must have been set using
//  setScanDevice() before any SANE function that can request authentication
//  is called.  If no scanner device has been set, nothing is called and
//  the authentication will fail.
//
//  In theory, the 'resource' parameter here should be the scanner device
//  name for which authentication is required.  But sometimes it doesn't seem
//  to correspond and is in a rather cryptic format, e.g. trying to open the
//  device "net:localhost:hpaio:/net/Officejet_Pro_L7600?ip=***.***.***.***"
//  passes a 'resource' such as "net:localhost:hpaio$MD5$6f034bceede4608f95f8".
//  So the parameter is ignored and KScanDevice prompts using its own
//  internal scanner name.

extern "C" void authCallback(SANE_String_Const resource,
                             SANE_Char username[SANE_MAX_USERNAME_LEN],
                             SANE_Char password[SANE_MAX_PASSWORD_LEN])
{
    kDebug() << "for resource" << resource;

    if (sScanDevice==NULL)				// no device set
    {
        kDebug() << "cannot authenticate, no device";
        return;
    }

    QByteArray user;
    QByteArray pass;
    if (sScanDevice->authenticate(&user, &pass))
    {
        qstrncpy(username, user.constData(), SANE_MAX_USERNAME_LEN);
        qstrncpy(password, pass.constData(), SANE_MAX_PASSWORD_LEN);
    }
}


void ScanGlobal::setScanDevice(KScanDevice *device)
{
    sScanDevice = device;
}


bool ScanGlobal::init()
{
    if (mSaneInitDone) return (true);			// already done, no more to do
    if (mSaneInitError) return (false);			// error happened, no point

    kDebug() << "calling sane_init()";
    SANE_Status status = sane_init(NULL, &authCallback);
    if (status!=SANE_STATUS_GOOD)
    {
        mSaneInitError = true;
        kDebug() << "sane_init() failed, status" << status;
    }
    else mSaneInitDone = true;

    return (mSaneInitDone);
}


bool ScanGlobal::available() const
{
    return (mSaneInitDone && !mSaneInitError);
}


KConfigGroup ScanGlobal::configGroup(const QString &groupName)
{
    if (mScanConfig==NULL) mScanConfig = new KConfig(SCANNER_DB_FILE, KConfig::SimpleConfig);
    return (mScanConfig->group(!groupName.isEmpty() ? groupName : GROUP_GENERAL));
}
