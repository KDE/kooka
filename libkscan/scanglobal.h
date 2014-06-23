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


class KConfigGroup;

class KScanDevice;


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
     *
     * @return @c true if SANE intialisation succeeded
     */
    bool init();

    /**
     * Checks whether SANE initialisation succeeded.
     *
     * @return @c true if @c init() has been called and it did, @c false if it did not. 
     * @see init
     */
    bool available() const;

    /**
     * Set the scanner device in use.  If authentication is required, its
     * @c authenticate() function will be called which is expected to supply
     * or prompt for a username/password.
     */
    void setScanDevice(KScanDevice *device);

private:
    ScanGlobal();
    ~ScanGlobal();

    bool mSaneInitDone;
    bool mSaneInitError;
};


#endif							// SCANGLOBAL_H
