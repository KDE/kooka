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

#include <klocalizedstring.h>
#include <kaboutdata.h>

#include "kscandevice.h"
#include "scansettings.h"

#include "libkookascan_logging.h"

extern "C" {
#include <sane/sane.h>
#ifdef HAVE_TIFF
#include <tiffvers.h>
#endif
}


static ScanGlobal *sInstance = nullptr;
static KScanDevice *sScanDevice = nullptr;

ScanGlobal *ScanGlobal::self()
{
    if (sInstance == nullptr) {
        sInstance = new ScanGlobal();
    }
    return (sInstance);
}

ScanGlobal::ScanGlobal()
{
    qCDebug(LIBKOOKASCAN_LOG);

    mSaneInitDone = false;
    mSaneInitError = false;
}

ScanGlobal::~ScanGlobal()
{
    if (mSaneInitDone) {
        qCDebug(LIBKOOKASCAN_LOG) << "calling sane_exit()";
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
    qCDebug(LIBKOOKASCAN_LOG) << "for resource" << resource;

    if (sScanDevice == nullptr) {          // no device set
        qCWarning(LIBKOOKASCAN_LOG) << "cannot authenticate, no device";
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

    qCDebug(LIBKOOKASCAN_LOG) << "calling sane_init()";
    SANE_Int vers;
    SANE_Status status = sane_init(&vers, &authCallback);
    if (status != SANE_STATUS_GOOD) {
        mSaneInitError = true;
        qCWarning(LIBKOOKASCAN_LOG) << "sane_init() failed, status" << status;
    } else {
        qCDebug(LIBKOOKASCAN_LOG) << "sane_init() done, version" << qPrintable(QString("%1").arg(vers, 6, 16, QLatin1Char('0')));

        // Add the SANE component information to the application data,
        // now that its version is available.
        KAboutData about = KAboutData::applicationData();
        about.addComponent(i18n("SANE - Scanner Access Now Easy"),		// name
                           i18n("Scanner access and driver library"),		// description
                           QString("%1.%2.%3").arg(SANE_VERSION_MAJOR(vers))	// version
                                              .arg(SANE_VERSION_MINOR(vers))
                                              .arg(SANE_VERSION_BUILD(vers)),
                           "http://sane-project.org",				// webAddress
                           KAboutLicense::GPL);					// licenseKey
#ifdef HAVE_TIFF
        about.addComponent(i18n("LibTIFF"),
                           i18n("TIFF image format library"),
#ifdef TIFFLIB_MAJOR_VERSION				// added in libtiff 4.5.0, maybe not in BSD
                           QString("%1.%2.%3").arg(TIFFLIB_MAJOR_VERSION)
                                              .arg(TIFFLIB_MINOR_VERSION)
                                              .arg(TIFFLIB_MICRO_VERSION),
#else
                           QString::number(TIFFLIB_VERSION),
#endif
                           "https://libtiff.gitlab.io/libtiff/",
                           // Licence text is at https://libtiff.gitlab.io/libtiff/project/license.html
                           KAboutLicense::Custom);
#endif
#ifdef HAVE_LIBPAPER
        about.addComponent(i18n("libpaper"),
                           i18n("Paper size configuration library"),
                           // The library version does not seem to be available.
                           "",
                           "https://github.com/rrthomas/libpaper",
                           // Some supporting executables are under GPLv2 or GPLv3,
                           // but Kooka does not use or link to those.
                           KAboutLicense::LGPL_V2_1);
#endif
        KAboutData::setApplicationData(about);

        mSaneInitDone = true;
    }

    return (mSaneInitDone);
}

bool ScanGlobal::available() const
{
    return (mSaneInitDone && !mSaneInitError);
}
