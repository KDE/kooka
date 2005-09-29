/* This file is part of the KDE Project
   Copyright (C) 2004 Klaas Freitag <freitag@suse.de>

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

#include "imgscaninfo.h"

#include <klocale.h>
#include <kdebug.h>


/* ############################################################################## */

ImgScanInfo::ImgScanInfo()
    :m_xRes(0),
     m_yRes(0),
     d(0)
{


}

void ImgScanInfo::setXResolution( int xres )
{
    m_xRes = xres;
}

int ImgScanInfo::getXResolution()
{
    return m_xRes;
}

void ImgScanInfo::setYResolution( int yres )
{
    m_yRes = yres;
}

int ImgScanInfo::getYResolution()
{
    return m_yRes;
}

void ImgScanInfo::setMode( const QString& smode )
{
    m_mode = smode;
}

QString ImgScanInfo::getMode()
{
    return m_mode;
}

void ImgScanInfo::setScannerName( const QString& name )
{
    m_scanner = name;
}

QString ImgScanInfo::getScannerName()
{
    return m_scanner;
}
