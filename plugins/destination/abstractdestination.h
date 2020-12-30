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
class ScanParamsPage;

#ifndef PLUGIN_EXPORT
#define PLUGIN_EXPORT
#endif


class PLUGIN_EXPORT AbstractDestination : public AbstractPlugin
{
    Q_OBJECT

public:
    /**
     * Destructor.
     **/
    virtual ~AbstractDestination();

    /**
     * Indicates that a scan is starting.  The plugin should prepare
     * to receive the image.
     *
     * @param type The type of the image that is about to be scanned.
     * @return @c true if the scan is to go ahead, or @c false if it
     * is to be cancelled.
     **/
    virtual bool scanStarting(ScanImage::ImageType type) = 0;

    /**
     * Indicates that an image has been scanned.  The plugin should
     * save or send the image as required.
     *
     * @param img The image that has been scanned,
     **/
    virtual void imageScanned(ScanImage::Ptr img) = 0;

    /**
     * Get a description string for the scan destination.
     *
     * @return A descriptive filename or title which will be shown
     * along with the scan progress indicator.  If the description
     * is an empty string then nothing will be shown.  The base class
     * implementation returns an empty string.
     *
     * @note @c scanStarting() may not necessarily have been called
     * before this function.
     **/
    virtual QString scanDestinationString()			{ return (QString()); }

    /**
     * Create GUI widgets as required for the selected destination
     * plugin.  This includes loading the saved settings if the
     * plugin has any.
     *
     * @param page The parent frame.
     *
     * @note The current row of the @p page is the first reserved
     * parameter row.  The base class implementation adds nothing.
     **/
    virtual void createGUI(ScanParamsPage *page)		{ }

    /**
     * Sets the scan gallery, in the event that the plugin needs
     * to locate it.
     *
     * @param gallery The scan gallery.
     * @see @c gallery()
     **/
    void setScanGallery(ScanGallery *gallery)			{ mGallery = gallery; }

    /**
     * Save the plugin settings, either to the application settings or
     * a private configuration.  The base class implementation does nothing.
     **/
    virtual void saveSettings() const				{ }

protected:
    /**
     * Constructor.
     *
     * @param pnt The parent object.
     * @param name The Qt object name.
     **/
    explicit AbstractDestination(QObject *pnt, const char *name);

    /**
     * Get the scan gallery, if needed by the plugin.
     *
     * @return the scan gallery, or @c nullptr if none has been set.
     * @see @c setScanGallery()
     **/
    ScanGallery *gallery() const				{ return (mGallery); }

private:
    ScanGallery *mGallery;
};

#endif							// ABSTRACTDESTINATION_H
