/***************************************************************************
                          imgnamecombo.cpp - combobox for image names 
                             -------------------                                         
    begin                : Tue Nov 13 2001
    copyright            : (C) 2001 by Klaas Freitag                         
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

#include <qlayout.h>
#include <qlabel.h>
#include <qlistview.h>

#include <packageritem.h>

#include <kcombobox.h>

#include <kdebug.h>
#include <klocale.h>
#include <kfiletreebranch.h>

#include "imgnamecombo.h"
#include "img_saver.h"

ImageNameCombo::ImageNameCombo( QWidget *parent )
   : KComboBox( parent )
{

}

ImageNameCombo::~ImageNameCombo()
{
   
}


void ImageNameCombo::slotGalleryPathChanged( KFileTreeBranch* branch, const QString& relativPath )
{
   QString newPath;

   newPath = branch->name() + QString::fromLatin1(" - ") + relativPath;

   kdDebug( 28000) << "Inserting " << newPath << " to combobox" << endl;
   
   if( ! items.contains( newPath ) )
   {
      // insert sorted
      items.append( newPath );
      items.sort();

      clear();
      for ( QStringList::Iterator it = items.begin(); it != items.end(); ++it )
      {
	 insertItem( branch->pixmap(), *it );
      }
   }
   int index = items.findIndex( newPath );
   kdDebug(28000) << "Having this index " << index << endl;
   
   setCurrentItem( index );
       
}

/* The End */
#include "imgnamecombo.moc"
