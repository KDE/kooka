/***************************************************************************
                          dispgamma.h  -  description                              
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


#ifndef DISPGAMMA_H
#define DISPGAMMA_H

#include <qwidget.h>
#include <qsizepolicy.h>
#include <qsize.h>
#include <qarray.h>

extern "C"{
#include <sane/sane.h>
#include <sane/saneopts.h>
}
/**
  *@author Klaas Freitag
  */

class DispGamma : public QWidget  {
    Q_OBJECT
public: 
    DispGamma( QWidget *parent );
    ~DispGamma();

    QSize sizeHint( void );
    QSizePolicy sizePolicy( void );

    void setValueRef( QArray<SANE_Word> *newVals )
    {
        vals = newVals;
    }
protected:
    void paintEvent (QPaintEvent *ev );
    void resizeEvent( QResizeEvent* );

private:

    QArray<SANE_Word> *vals;
    int margin;

   class DispGammaPrivate;
   DispGammaPrivate *d;
};

#endif
