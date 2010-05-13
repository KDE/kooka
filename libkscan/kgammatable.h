/* This file is part of the KDE Project
   Copyright (C) 2002 Klaas Freitag <freitag@suse.de>  

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

#ifndef KGAMMATABLE_H
#define KGAMMATABLE_H

#include "libkscanexport.h"

#include <qvector.h>
#include <qobject.h>

extern "C" {
#include <sane/sane.h>
}


class KSCAN_EXPORT KGammaTable : public QObject
{
   Q_OBJECT

   Q_PROPERTY( int g READ getGamma WRITE setGamma )
   Q_PROPERTY( int c READ getContrast WRITE setContrast )
   Q_PROPERTY( int b READ getBrightness WRITE setBrightness )
      
public:
   KGammaTable ( int gamma = 100, int brightness = 0,
		 int contrast = 0 );
   void setAll ( int gamma, int brightness, int contrast );
   QVector<SANE_Word> *getArrayPtr( void ) { return &gt; }

   int  getGamma( ) const      { return g; }
   int  getBrightness( ) const { return b; }
   int  getContrast( ) const   { return c; }

   const KGammaTable& operator=(const KGammaTable&);

public slots:
   void       setContrast   ( int con )   { c = con; dirty = true; emit( tableChanged() );}
   void       setBrightness ( int bri )   { b = bri; dirty = true; emit( tableChanged() );}
   void       setGamma      ( int gam )   { g = gam; dirty = true; emit( tableChanged() );}

   int        tableSize() const      { return gt.size(); }
   SANE_Word  *getTable();


// TODO: this signal is never used,
// so does this class even need to be a Q_OBJECT?
signals:
   void tableChanged(void);

private:
   void       calcTable( );
   int        g, b, c;
   bool       dirty;
   QVector<SANE_Word> gt;
};

#endif							// KGAMMATABLE_H
