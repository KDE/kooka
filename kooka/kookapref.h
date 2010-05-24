/***************************************************** -*- mode:c++; -*- ***
                          kookapref.h  - Preferences
                             -------------------
    begin                : Sun Jan 16 2000
    copyright            : (C) 2000 by Klaas Freitag
    email                : freitag@suse.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This file may be distributed and/or modified under the terms of the    *
 *  GNU General Public License version 2 as published by the Free Software *
 *  Foundation and appearing in the file COPYING included in the           *
 *  packaging of this file.                                                *
 *
 *  As a special exception, permission is given to link this program       *
 *  with any version of the KADMOS ocr/icr engine of reRecognition GmbH,   *
 *  Kreuzlingen and distribute the resulting executable without            *
 *  including the source code for KADMOS in the source distribution.       *
 *
 *  As a special exception, permission is given to link this program       *
 *  with any edition of Qt, and distribute the resulting executable,       *
 *  without including the source code for Qt in the source distribution.   *
 *                                                                         *
 ***************************************************************************/

#ifndef KOOKAPREF_H
#define KOOKAPREF_H

#include <kpagedialog.h>

#include "kookagallery.h"
#include "ocrengine.h"


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


class QCheckBox;
class QPushButton;
class QLabel;

class KIntNumInput;
class KColorButton;
class KUrlRequester;
class KComboBox;


class KookaPref : public KPageDialog
{
    Q_OBJECT

public:
    KookaPref(QWidget *parent = NULL);

    static QString tryFindGocr();
    static QString tryFindOcrad();

    bool galleryAllowRename() const;

    // TODO: reimplement for KDE4, used in Kooka::optionsPreferences()
    //void showPageIndex(int page);
    //int currentPageIndex(void);

    /**
     * Static function that returns the image gallery base dir.
     */
    // TODO: this should return a KUrl
    static QString galleryRoot();

protected slots:
    void slotSaveSettings();
    void slotSetDefaults();
    void slotEngineSelected(int i);
    void slotEnableWarnings();

signals:
    void dataSaved();

private slots:
    void slotCustomThumbBgndToggled(bool state);

private:
    void setupGeneralPage();
    void setupStartupPage();
    void setupSaveFormatPage();
    void setupThumbnailPage();
    void setupOCRPage();

    bool checkOCRBin(const QString &cmd,const QString &bin,bool showMsg);
    static QString findGalleryRoot();

    KSharedConfig *konf;
    OcrEngine::EngineType originalEngine;
    OcrEngine::EngineType selectedEngine;

    // General
    QCheckBox *cbAllowRename;
    QPushButton *pbEnableMsgs;
    KComboBox *layoutCB;

    // Startup
    QCheckBox *cbNetQuery;
    QCheckBox *cbShowScannerSelection;
    QCheckBox *cbReadStartupImage;

    // Image Saving
    QCheckBox *cbSkipFormatAsk;
    QCheckBox *cbFilenameAsk;

    // Thumbnail View
    KUrlRequester *m_tileSelector;
    KComboBox *m_thumbSizeCb;
    QCheckBox *cbCustomThumbBgnd;

    // OCR
    KUrlRequester *binaryReq;
    KComboBox *engineCB;
    QLabel *ocrDesc;

    static QString sGalleryRoot;
};

#endif							// KOOKAPREF_H
