/***************************************************************************
                imgprintdialog.h  - Kooka's Image Printing
                             -------------------
    begin                : May 2003
    copyright            : (C) 1999 by Klaas Freitag
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

#ifndef __IMGPRINTDIALOG_H__
#define __IMGPRINTDIALOG_H__

#include <qmap.h>
#include <qcheckbox.h>
#include <kdeprint/kprintdialogpage.h>

#include "kookaimage.h"

#define OPT_SCALING  "kde-kooka-scaling"
#define OPT_SCAN_RES "kde-kooka-scanres"
#define OPT_SCREEN_RES "kde-kooka-screenres"
#define OPT_WIDTH    "kde-kooka-width"
#define OPT_HEIGHT   "kde-kooka-height"
#define OPT_PSGEN_DRAFT  "kde-kooka-psdraft"
#define OPT_RATIO    "kde-kooka-ratio"
#define OPT_FITPAGE  "kde-kooka-fitpage"
class QWidget;
class QString;
class QLabel;
class KIntNumInput;
class KookaImage;
class QVButtonGroup;
class QRadioButton;
class QCheckBox;

class ImgPrintDialog: public KPrintDialogPage
{
    Q_OBJECT
public:
    ImgPrintDialog( KookaImage *img, QWidget *parent=0L, const char* name=0L );

    void setOptions(const QMap<QString,QString>& opts);
    void getOptions(QMap<QString,QString>& opts, bool include_def = false);
    bool isValid(QString& msg);

    void setImage( KookaImage *img );

protected slots:
    void slScaleChanged( int id );
    void slCustomWidthChanged(int);
    void slCustomHeightChanged(int);

private:
    QButtonGroup  *m_scaleRadios;
    QRadioButton  *m_rbOrigSize;
    QRadioButton  *m_rbScale;
    QRadioButton  *m_rbScreen;
    QRadioButton  *m_rbFitPage;
    
    KIntNumInput *m_sizeW;
    KIntNumInput *m_sizeH;
    KIntNumInput *m_dpi;

    QCheckBox    *m_psDraft;
    QCheckBox    *m_ratio;

    KookaImage *m_image;
    QLabel     *m_screenRes;
    bool        m_ignoreSignal;
};

#endif
