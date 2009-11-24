/* This file is part of the KDE Project			-*- mode:c++; -*-
   Copyright (C) 1999 Klaas Freitag <freitag@suse.de>
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

#ifndef SCANGLOBAL_H
#define SCANGLOBAL_H

#include "libkscanexport.h"

#include <qstring.h>


/* Configuration-file definitions */
#define DEFAULT_OPTIONSET	"saveSet"

// Note that this is not the same GROUP_STARTUP which is used in kookapref!
// That uses Kooka's config file 'kookarc', libkscan uses the global scanner
// config file 'scannerrc'.
#define GROUP_STARTUP		"Startup"
#define STARTUP_SCANDEV		"ScanDevice"
#define STARTUP_SKIP_ASK	"SkipStartupAsk"
#define STARTUP_ONLY_LOCAL	"QueryLocalOnly"

#define USERDEV_GROUP		"User Specified Scanners"
#define USERDEV_DEVS		"Devices"
#define USERDEV_DESC		"Description"
#define USERDEV_TYPE		"Type"


class KConfig;
class KConfigGroup;


class KSCAN_EXPORT ScanGlobal
{

public:
    /**
     * Create (if necessary) and return the single instance.
     */
    static ScanGlobal *self();

    /**
     * Calls sane_init() to initialise the SANE library, the first time this
     * function is called.  Subsequent calls are ignored.  Sets up to call
     * sane_exit() when the application exits.
     * @return true if SANE intialisation was OK
     */
    bool init();

    /**
     * Checks whether sane_init() succeeded, returns @c true if that is so.
     * If not, @c false is returned and no other scanning functions are available.
     */
    bool available() const;

    /**
     * Get a config group in the global scanner configuration file (named by
     * SCANNER_DB_FILE).
     */
    KConfigGroup configGroup(const QString &groupName = QString::null);

private:
    explicit ScanGlobal();
    ~ScanGlobal();

    bool mSaneInitDone;
    bool mSaneInitError;

    KConfig *mScanConfig;

    class ScanGlobalPrivate;
    ScanGlobalPrivate *d;
};


#endif							// SCANGLOBAL_H
