/***************************************************** -*- mode:c++; -*- ***
                     kocrgocr.h - ocr dialog for GOCR
                             -------------------
    begin                : Sun Jun 11 2000
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


#ifndef OCRGOCRDIALOG_H
#define OCRGOCRDIALOG_H

#include <kdialog.h>

#include "libkscan/kscancontrols.h"

#include "ocrbasedialog.h"


/**
  *@author Klaas Freitag
  */

class QWidget;


class OcrGocrDialog : public OcrBaseDialog
{
    Q_OBJECT

public:
    OcrGocrDialog(QWidget *parent);
    ~OcrGocrDialog();

    OcrEngine::EngineError setupGui();

    QString getOCRCmd() const	{ return (m_ocrCmd); }
    int getGraylevel() const	{ return (sliderGrayLevel->value()); }
    int getDustsize() const	{ return (sliderDustSize->value()); }
    int getSpaceWidth() const	{ return (sliderSpace->value()); }

    QString ocrEngineLogo() const;
    QString ocrEngineName() const;
    QString ocrEngineDesc() const;

    void introduceImage(const KookaImage *img);

protected:
    void enableFields(bool enable);

protected slots:
    void slotWriteConfig();

private:
    QString version();

private:
    KScanSlider *sliderGrayLevel;
    KScanSlider *sliderDustSize;
    KScanSlider *sliderSpace;

    QWidget *m_setupWidget;
    QString m_ocrCmd;
    bool m_isBW;
};


#endif							// OCRGOCRDIALOG_H
