/***************************************************************************
                          kocrstartdia.h  -  description                              
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


#ifndef KOCRSTARTDIA_H
#define KOCRSTARTDIA_H

#include <kdialogbase.h>
#include <qimage.h>
#include <qstring.h>

#include <kscanslider.h>
#include <kanimwidget.h>
/**
  *@author Klaas Freitag
  */




class KOCRStartDialog: public KDialogBase
{
   Q_OBJECT
public: 
   KOCRStartDialog( QWidget *);
   ~KOCRStartDialog();

   QCString getOCRCmd( void ) const
      { return( (entryOcrBinary->text()).latin1());}

   int getGraylevel( void ) const
      { return( sliderGrayLevel->value());}
   int getDustsize( void ) const
      { return( sliderDustSize->value());}
   int getSpaceWidth( void ) const
      { return( sliderSpace->value());}

public slots:
   void stopAnimation( void ){ ani->stop(); }
   /* displaying a popup if not ok */
   void checkOCRBinary( const QCString& );
   void checkOCRBinaryShort( const QCString& );
protected:

private slots:
   void disableFields( void );
   void writeConfig( void );
   void checkOCRBinIntern( const QCString&, bool );
   
private:
   QCString tryFindGocr( void ) const;
   
   KScanSlider *sliderGrayLevel;
   KScanSlider *sliderDustSize;
   KScanSlider *sliderSpace;
   KScanEntry  *entryOcrBinary;
   KAnimWidget *ani;
};

#endif
