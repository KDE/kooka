/***************************************************************************
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


#ifndef KOCRGOCR_H
#define KOCRGOCR_H

#include <kdialogbase.h>
#include <qimage.h>
#include <qstring.h>

#include <kscanslider.h>
#include <kanimwidget.h>

#include "kocrbase.h"
/**
  *@author Klaas Freitag
  */

class KSpellConfig;

class KGOCRDialog: public KOCRBase
{
    Q_OBJECT
public:
    KGOCRDialog( QWidget*, KSpellConfig* );
    ~KGOCRDialog();

    QString getOCRCmd( void ) const
        { return m_ocrCmd;}

    int getGraylevel( void ) const
        { return( sliderGrayLevel->value());}
    int getDustsize( void ) const
        { return( sliderDustSize->value());}
    int getSpaceWidth( void ) const
        { return( sliderSpace->value());}

    EngineError setupGui();

    QString ocrEngineName() const;
    QString ocrEngineDesc() const;
    QString ocrEngineLogo() const;

public slots:
    void enableFields(bool);
    void introduceImage( KookaImage* );

protected:
    void writeConfig();


private:


    KScanSlider *sliderGrayLevel;
    KScanSlider *sliderDustSize;
    KScanSlider *sliderSpace;

    QString      m_ocrCmd;
};

#endif
