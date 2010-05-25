/* This file is part of the KDE Project				-*- mode:c++ -*-

   Copyright (C) 2000 Klaas Freitag <freitag@suse.de>
   Copyright (C) 2010 Jonathan Marten <jjm@keelhaul.me.uk>  

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   As a special exception, permission is given to link this program
   with any version of the KADMOS ocr/icr engine of reRecognition GmbH,
   Kreuzlingen and distribute the resulting executable without
   including the source code for KADMOS in the source distribution.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.

*/


#ifndef KOOKAPREF_H
#define KOOKAPREF_H

#include <kpagedialog.h>


#define GROUP_GALLERY		"Gallery"
#define GALLERY_ALLOW_RENAME	"AllowRename"
#define GALLERY_LAYOUT		"Layout"

#define GALLERY_LOCATION	"Location"
#define GALLERY_DEFAULT_LOC	"KookaGallery"

// Note that this is not the same GROUP_STARTUP which is used in
// libkscan/scanglobal!  Settings here are used by Kooka only.
#define GROUP_STARTUP		"Startup"
#define STARTUP_READ_IMAGE      "ReadImageOnStart"

#define CFG_GROUP_OCR_DIA       "ocrDialog"
#define CFG_OCRAD_BINARY        "ocradBinary"
#define CFG_GOCR_BINARY         "gocrBinary"


class QVBoxLayout;

class KookaPrefsPage;


class KookaPref : public KPageDialog
{
    Q_OBJECT

public:
    KookaPref(QWidget *parent = NULL);

    int createPage(KookaPrefsPage *page, const QString &name,
                   const QString &header, const char *icon);

    static QString tryFindGocr();
    static QString tryFindOcrad();

    void showPageIndex(int page);
    int currentPageIndex();

    /**
     * Static function that returns the image gallery base dir.
     */
    // TODO: this should return a KUrl
    static QString galleryRoot();

protected slots:
    void slotSaveSettings();
    void slotSetDefaults();

signals:
    void dataSaved();

private:
    QVector<KPageWidgetItem *> mPages;

    static QString findGalleryRoot();

    static QString sGalleryRoot;
};

#endif							// KOOKAPREF_H
