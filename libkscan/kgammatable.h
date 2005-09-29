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

#include <qmemarray.h>
#include <qobject.h>

extern "C" {
#include <sane/sane.h>
}

class KGammaTable: public QObject
{
   Q_OBJECT

   Q_PROPERTY( int g READ getGamma WRITE setGamma )
   Q_PROPERTY( int c READ getContrast WRITE setContrast )
   Q_PROPERTY( int b READ getBrightness WRITE setBrightness )
      
public:
   KGammaTable ( int gamma = 100, int brightness = 0,
		 int contrast = 0 );
   void setAll ( int gamma, int brightness, int contrast );
   QMemArray<SANE_Word> *getArrayPtr( void ) { return &gt; }

   int  getGamma( ) const      { return g; }
   int  getBrightness( ) const { return b; }
   int  getContrast( ) const   { return c; }

   const KGammaTable& operator=(const KGammaTable&);

public slots:
   void       setContrast   ( int con )   { c = con; dirty = true; emit( tableChanged() );}
   void       setBrightness ( int bri )   { b = bri; dirty = true; emit( tableChanged() );}
   void       setGamma      ( int gam )   { g = gam; dirty = true; emit( tableChanged() );}

   int        tableSize()      { return gt.size(); }
   SANE_Word  *getTable();

signals:
   void tableChanged(void);

private:
   void       calcTable( );
   int        g, b, c;
   bool       dirty;
   QMemArray<SANE_Word> gt;

   class KGammaTablePrivate;
   KGammaTablePrivate *d;
};

#endif // KGAMMATABLE_H
