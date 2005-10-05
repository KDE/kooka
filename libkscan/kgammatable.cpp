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

#include <math.h>

#include <kdebug.h>
#include "kgammatable.h"

KGammaTable::KGammaTable( int gamma, int brightness, int contrast  ) 
    : QObject()
{
   g = gamma < 1 ? 1 : gamma;
   b = brightness;
   c = contrast;
   gt.resize(256);
   calcTable();
}

const KGammaTable& KGammaTable::operator=(const KGammaTable& gt)
{
   if( this != &gt )
   {
      g = gt.g;
      b = gt.b;
      c = gt.c;

      calcTable();
   }

   return( *this );
}

inline SANE_Word adjust( SANE_Word x, int gl,int bl,int cl)
{
   //printf("x : %d, x/256 : %lg, g : %d, 100/g : %lg",
   //       x,(double)x/256.0,g,100.0/(double)g);
   x = ( SANE_Int )(256*pow((double)x/256.0,100.0/(double)gl));
   x = ((65536/(128-cl)-256)*(x-128)>>8)+128+bl;
   if(x<0) x = 0; else if(x>255) x = 255;
   //printf(" -> %d\n",x);
   return x;
}

void KGammaTable::calcTable( )
{
   int br = (b<<8)/(128-c);
   int gr = g;
   int cr = c;

   if( gr == 0 )
   {
      kdDebug(29000) << "Cant calc table -> would raise div. by zero !" << endl;
      return;
   }
   
   for( SANE_Word i = 0; i<256; i++)
      gt[i] = adjust(i, gr, br, cr );

   dirty = false;
}

void KGammaTable::setAll( int gamma, int brightness, int contrast )
{
    g = gamma < 1 ? 1 : gamma;
    b = brightness;
    c = contrast;

    dirty = true;
}


SANE_Word* KGammaTable::getTable()
{
    if( dirty ) calcTable();
    return( gt.data());
}
#include "kgammatable.moc"
