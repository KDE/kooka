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

#include <kdialogbase.h>

#include "ksaneocr.h"

#define STARTUP_READ_IMAGE "ReadImageOnStart"
#define CFG_GROUP_OCR_DIA  "ocrDialog"
#define CFG_OCRAD_BINARY   "ocradBinary"
#define CFG_GOCR_BINARY    "gocrBinary"

class KIntNumInput;
class KColorButton;
class ImageSelectLine;
class KURLRequester;
class QCheckBox;
class KComboBox;
class KActiveLabel;


class KookaPref : public KDialogBase
{
    Q_OBJECT

public:
    KookaPref();

    static QString tryFindGocr();
    static QString tryFindOcrad();

protected slots:
    void slotOk( void );
    void slotApply( void );
    void slotDefault( void );
    void slotEngineSelected(int i);

signals:
    void dataSaved();

private:
    void setupStartupPage();
    void setupSaveFormatPage();
    void setupThumbnailPage();
    void setupOCRPage();

    bool checkOCRBin(const QString &cmd,const QString &bin,bool showMsg);
    static QString tryFindBinary(const QString &bin,const QString &configKey);

    KConfig   *konf;
    KSaneOcr::OcrEngine originalEngine;
    KSaneOcr::OcrEngine selectedEngine;

    // Startup
    QCheckBox *cbNetQuery;
    QCheckBox *cbShowScannerSelection;
    QCheckBox *cbReadStartupImage;

    // Image Saving
    QCheckBox *cbSkipFormatAsk;
    QCheckBox *cbFilenameAsk;

    // Thumbnail View
    KIntNumInput *m_thumbWidth;
    KIntNumInput *m_thumbHeight;
    KIntNumInput *m_frameWidth;
    ImageSelectLine *m_tileSelector;
    KColorButton *m_colButt1;
    KColorButton *m_colButt2;

    // OCR
    KURLRequester *binaryReq;
    KComboBox *engineCB;
    KActiveLabel *ocrDesc;
};

#endif
