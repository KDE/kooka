/***************************************************************************
                     kocrgocr.h - ocr dialog for GOCR
                             -------------------
    begin                : Sun Jun 11 2000
    copyright            : (C) 2000 by Klaas Freitag
    email                : freitag@suse.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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



class KGOCRDialog: public KOCRBase
{
    Q_OBJECT
public:
    KGOCRDialog( QWidget *);
    ~KGOCRDialog();

    QString getOCRCmd( void ) const
        { return m_ocrCmd;}

    int getGraylevel( void ) const
        { return( sliderGrayLevel->value());}
    int getDustsize( void ) const
        { return( sliderDustSize->value());}
    int getSpaceWidth( void ) const
        { return( sliderSpace->value());}

    void setupGui();

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
