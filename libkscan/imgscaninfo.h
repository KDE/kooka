/* This file is part of the KDE Project
   Copyright (C) 1999 Klaas Freitag <freitag@suse.de>

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

#ifndef __IMGSCANINFO_H__
#define __IMGSCANINFO_H__

#include <qstring.h>

/* ----------------------------------------------------------------------
 *
 */
class ImgScanInfo
{
public:
    ImgScanInfo();

    int getXResolution();
    int getYResolution();
    QString getMode();
    QString getScannerName();

    void setXResolution( int );
    void setYResolution( int );
    void setMode( const QString& );
    void setScannerName( const QString& );
private:
    int m_xRes;
    int m_yRes;
    QString m_mode;
    QString m_scanner;

   class ImgScanInfoPrivate;
   ImgScanInfoPrivate *d;
};

#endif
