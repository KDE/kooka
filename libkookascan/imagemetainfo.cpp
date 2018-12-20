/* This file is part of the KDE Project
   Copyright (C) 2004 Klaas Freitag <freitag@suse.de>
   Copyright (C) 2011 Jonathan Marten <jjm@keelhaul.me.uk>

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

#include "imagemetainfo.h"

#include <qimage.h>

ImageMetaInfo::ImageMetaInfo()
    : m_xRes(-1), m_yRes(-1),
      m_type(ImageMetaInfo::Unknown)
{
}

ImageMetaInfo::ImageType ImageMetaInfo::findImageType(const QImage *image)
{
    if (image == nullptr || image->isNull()) {
        return (ImageMetaInfo::Unknown);
    }

    if (image->depth()==1 || image->colorCount()==2) {
        return (ImageMetaInfo::BlackWhite);
    } else {
        if (image->depth()>8) {
            return (ImageMetaInfo::HighColour);
        } else {
            if (image->allGray()) {
                return (ImageMetaInfo::Greyscale);
            } else {
                return (ImageMetaInfo::LowColour);
            }
        }
    }
}
