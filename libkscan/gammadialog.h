/***************************************************************************
                          gammadialog.h  -  description
                             -------------------
    begin                : Sat Aug 12 2000
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


#ifndef GAMMADIALOG_H
#define GAMMADIALOG_H

#include <qwidget.h>
#include <qlayout.h>

#include <kgammatable.h>
#include <kdialogbase.h>



#include "dispgamma.h"


/**
  *@author Klaas Freitag
  */

class KScanSlider;
class KGammaTable;

class GammaDialog : public KDialogBase
{
   Q_OBJECT
   Q_PROPERTY( KGammaTable *gt READ getGt WRITE setGt )
      
public:
   GammaDialog ( QWidget *parent );
   ~GammaDialog( );

   KGammaTable *getGt( ) const { return gt; }
   void         setGt( KGammaTable& ngt);

public slots:
   virtual void slotApply();

signals:
   void gammaToApply( GammaDialog& );
   
private:
   KGammaTable *gt;
   DispGamma   *gtDisp;

   QHBoxLayout *lhMiddle;
   QVBoxLayout *lvSliders;

   KScanSlider *wGamma;
   KScanSlider *wBright;
   KScanSlider *wContrast;

   class GammaDialogPrivate;
   GammaDialogPrivate *d;
};

#endif
