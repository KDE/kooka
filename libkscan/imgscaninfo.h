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

#ifndef IMGSCANINFO_H
#define IMGSCANINFO_H

#include "libkscanexport.h"

#include <qstring.h>
#include <qimage.h>





/////////// TODO: have an enum here for the image type
// (avoid having to have 2 parameters format/grey)
// getFormat => return that
// ImageSaver type = that plus extras



class KSCAN_EXPORT ImgScanInfo
{
public:
    ImgScanInfo();

    int getXResolution() const				{ return (m_xRes); }
    int getYResolution() const				{ return (m_yRes); }
    QString getMode() const				{ return (m_mode); }
    QString getScannerName() const			{ return (m_scanner); }
    QImage::Format getFormat() const			{ return (m_format); }
    bool getIsGrey() const				{ return (m_isgrey); }

    void setXResolution(int xres)			{ m_xRes = xres; }
    void setYResolution(int yres)			{ m_yRes = yres; }
    void setMode(const QString &mode)			{ m_mode = mode; }
    void setScannerName(const QString &scanner)		{ m_scanner = scanner; }
    void setFormat(QImage::Format fmt, bool grey)	{ m_format = fmt; m_isgrey = grey; }

private:
    int m_xRes;
    int m_yRes;
    QString m_mode;
    QString m_scanner;
    QImage::Format m_format;
    bool m_isgrey;
};

#endif							// IMGSCANINFO_H
