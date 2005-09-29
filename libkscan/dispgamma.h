/* This file is part of the KDE Project
   Copyright (C) 2000 Klaas Freitag <freitag@suse.de>  

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef DISPGAMMA_H
#define DISPGAMMA_H

#include <qwidget.h>
#include <qsizepolicy.h>
#include <qsize.h>
#include <qmemarray.h>

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

    void setValueRef( QMemArray<SANE_Word> *newVals )
    {
        vals = newVals;
    }
protected:
    void paintEvent (QPaintEvent *ev );
    void resizeEvent( QResizeEvent* );

private:

    QMemArray<SANE_Word> *vals;
    int margin;

   class DispGammaPrivate;
   DispGammaPrivate *d;
};

#endif
