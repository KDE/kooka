/***************************************************************************
	     kscanoptset.cpp -  store a set of scan options
                             -------------------
    begin                : Wed Oct 13 2000
    copyright            : (C) 2000 by Klaas Freitag
    email                : Klaas.Freitag@SuSE.de

    $Id$
***************************************************************************/
 
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include <qstring.h>
#include <qlist.h>

#include "kscandevice.h"
#include "kscanoptset.h"

KScanOptSet::KScanOptSet( const QString setName, const KScanDevice *device ) :
  QObject()
{
  
  name = setName;

   if( device )
     {
       
     }	
}
