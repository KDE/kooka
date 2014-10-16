/* This file is part of the KDE Project         -*- mode:c++; -*-
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

#include "kookascan_export.h"

#include <qstring.h>

// Configuration file definitions

#define DEFAULT_OPTIONSET   "saveSet"

#define STARTUP_SCANDEV     "ScanDevice"
#define STARTUP_SKIP_ASK    "SkipStartupAsk"
#define STARTUP_ONLY_LOCAL  "QueryLocalOnly"

class KConfig;
class KConfigGroup;

class KScanDevice;

class KOOKASCAN_EXPORT ScanGlobal
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
     *
     * @return true if SANE intialisation was OK
     */
    bool init();

    /**
     * Checks whether sane_init() succeeded, returns @c true if that is so.
     * If not, @c false is returned and no other scanning functions are available.
     */
    bool available() const;

    /**
     * Set the scanner device in use.  If authentication is required, its
     * @c authenticate() function will be called which is expected to supply
     * or prompt for a username/password.
     */
    void setScanDevice(KScanDevice *device);

    /**
     * Get a config group in the global scanner configuration file (named by
     * SCANNER_DB_FILE).  If the @c groupName parameter is null or not
     * specified, it defaults to the general group (GROUP_GENERAL).
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

#endif                          // SCANGLOBAL_H
