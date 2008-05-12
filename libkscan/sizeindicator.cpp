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

#include <kimageeffect.h>
#include <klocale.h>
#include <kdebug.h>

#include <kio/global.h>

#include <qpainter.h>
#include <qpalette.h>
#include <qimage.h>

#include "sizeindicator.h"
#include "sizeindicator.moc"


SizeIndicator::SizeIndicator( QWidget *parent, long  thres, long crit )
   :QLabel( parent )
{
   sizeInByte = -1;
   setFrameStyle( QFrame::Box | QFrame::Sunken );
   setMinimumWidth( fontMetrics().width( QString::fromLatin1("MMMM.MM MB") ));
   setCritical( crit );
   threshold = thres;

}


void SizeIndicator::setCritical( long crit )
{
   critical = crit;
   devider = 255.0 / double( critical );
}


void SizeIndicator::setThreshold( long thres )
{
   threshold = thres;
}


SizeIndicator::~SizeIndicator()
{
}


void SizeIndicator::setSizeInByte(long newSize)
{
    kdDebug(29000) << k_funcinfo << "New size " << newSize << " bytes" << endl;

    sizeInByte = newSize;

    if (newSize<0) setText("");
    else setText(KIO::convertSize(newSize));
}


void SizeIndicator::drawContents( QPainter *p )
{
   QSize s = size();

   QColor warnColor;

   if( sizeInByte >= threshold )
   {
      int c = int( double(sizeInByte) * devider );
      if( c > 255 ) c = 255;

      warnColor.setHsv( 0, c, c );

      p->drawImage( 0,0,
		    KImageEffect::unbalancedGradient( s, colorGroup().background(),
						      warnColor, KImageEffect::CrossDiagonalGradient, 200,200 ));
   }
   /* Displaying the text */
   QString t = text();
   p->drawText( 0, 0, s.width(), s.height(),
		AlignHCenter | AlignVCenter, t);

}
