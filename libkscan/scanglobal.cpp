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

extern "C" {
#include <sane/sane.h>
}


/* Configuration-file definitions */
#define SCANNER_DB_FILE		"scannerrc"


static ScanGlobal *sInstance = NULL;


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


bool ScanGlobal::init()
{
    if (mSaneInitDone) return (true);			// already done, no more to do
    if (mSaneInitError) return (false);			// error happened, no point

    kDebug() << "calling sane_init()";
    // TODO: authorisation callback
    SANE_Status status = sane_init(NULL, NULL);
    if (status!=SANE_STATUS_GOOD)
    {
        mSaneInitError = true;
        kDebug() << "sane_init() failed, status" << status;
    }
    else
    {
        mSaneInitDone = true;
    }

    return (mSaneInitDone);
}


bool ScanGlobal::available() const
{
    return (mSaneInitDone && !mSaneInitError);
}


KConfigGroup ScanGlobal::configGroup(const QString &groupName)
{
    if (mScanConfig==NULL) mScanConfig = new KConfig(SCANNER_DB_FILE, KConfig::SimpleConfig);
    return (mScanConfig->group(!groupName.isEmpty() ? groupName : GROUP_STARTUP));
}
