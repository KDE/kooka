/***************************************************************************
                          kookaimage.h  - Kooka's Image 
                             -------------------                                         
    begin                : Thu Nov 20 2001
    copyright            : (C) 1999 by Klaas Freitag                         
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


#ifndef KOOKAIMAGE_H
#define KOOKAIMAGE_H

#include <qimage.h>

/**
  *@author Klaas Freitag
  */


class KookaImage: public QImage
{
public:
   KookaImage( );
   KookaImage( const QImage& img );
   ~KookaImage();

};

#endif
