/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2009-2016 Jonathan Marten <jjm@keelhaul.me.uk>	*
 *									*
 *  Kooka is free software; you can redistribute it and/or modify it	*
 *  under the terms of the GNU Library General Public License as	*
 *  published by the Free Software Foundation and appearing in the	*
 *  file COPYING included in the packaging of this file;  either	*
 *  version 2 of the License, or (at your option) any later version.	*
 *									*
 *  As a special exception, permission is given to link this program	*
 *  with any version of the KADMOS OCR/ICR engine (a product of		*
 *  reRecognition GmbH, Kreuzlingen), and distribute the resulting	*
 *  executable without including the source code for KADMOS in the	*
 *  source distribution.						*
 *									*
 *  This program is distributed in the hope that it will be useful,	*
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of	*
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*
 *  GNU General Public License for more details.			*
 *									*
 *  You should have received a copy of the GNU General Public		*
 *  License along with this program;  see the file COPYING.  If		*
 *  not, see <http://www.gnu.org/licenses/>.				*
 *									*
 ************************************************************************/

#include "scanglobal.h"

#include <qdebug.h>

#include <klocalizedstring.h>
#include <kglobal.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kstandarddirs.h>

#include "kscandevice.h"
#include "scansettings.h"

extern "C" {
#include <sane/sane.h>
}


static ScanGlobal *sInstance = NULL;
static KScanDevice *sScanDevice = NULL;

ScanGlobal *ScanGlobal::self()
{
    if (sInstance == NULL) {
        sInstance = new ScanGlobal();
    }
    return (sInstance);
}

ScanGlobal::ScanGlobal()
{
    //qDebug();

    mSaneInitDone = false;
    mSaneInitError = false;
}

ScanGlobal::~ScanGlobal()
{
    if (mSaneInitDone) {
        //qDebug() << "calling sane_exit()";
        sane_exit();
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
    //qDebug() << "for resource" << resource;

    if (sScanDevice == NULL) {          // no device set
        //qDebug() << "cannot authenticate, no device";
        return;
    }

    QByteArray user;
    QByteArray pass;
    if (sScanDevice->authenticate(&user, &pass)) {
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
    if (mSaneInitDone) {
        return (true);    // already done, no more to do
    }
    if (mSaneInitError) {
        return (false);    // error happened, no point
    }

    //qDebug() << "calling sane_init()";
    SANE_Status status = sane_init(NULL, &authCallback);
    if (status != SANE_STATUS_GOOD) {
        mSaneInitError = true;
        //qDebug() << "sane_init() failed, status" << status;
    } else {
        mSaneInitDone = true;
    }

    return (mSaneInitDone);
}

bool ScanGlobal::available() const
{
    return (mSaneInitDone && !mSaneInitError);
}
