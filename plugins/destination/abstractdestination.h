/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2000-2018 Klaas Freitag <freitag@suse.de>		*
 *                          Jonathan Marten <jjm@keelhaul.me.uk>	*
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

#ifndef ABSTRACTDESTINATION_H
#define ABSTRACTDESTINATION_H

#include <qobject.h>
#include <qtextformat.h>
#include <qprocess.h>

#include "scanimage.h"
#include "abstractplugin.h"


class ScanGallery;

#ifndef PLUGIN_EXPORT
#define PLUGIN_EXPORT
#endif


class PLUGIN_EXPORT AbstractDestination : public AbstractPlugin
{
    Q_OBJECT

public:
    virtual ~AbstractDestination();

    virtual bool scanStarting(ScanImage::ImageType type) = 0;
    virtual void imageScanned(ScanImage::Ptr img) = 0;

    virtual QString scanDestinationString()		{ return (QString()); }

    void setScanGallery(ScanGallery *gallery)		{ mGallery = gallery; }

protected:
    explicit AbstractDestination(QObject *pnt, const char *name);

    ScanGallery *gallery() const			{ return (mGallery); }

private:
    ScanGallery *mGallery;
};

#endif							// ABSTRACTDESTINATION_H