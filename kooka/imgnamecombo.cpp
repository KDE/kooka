/***************************************************************************
                          imgnamecombo.cpp - combobox for image names 
                             -------------------                                         
    begin                : Tue Nov 13 2001
    copyright            : (C) 2001 by Klaas Freitag                         
    email                : freitag@suse.de
 ***************************************************************************/


/***************************************************************************
 *                                                                         *
 *  This file may be distributed and/or modified under the terms of the    *
 *  GNU General Public License version 2 as published by the Free Software *
 *  Foundation and appearing in the file COPYING included in the           *
 *  packaging of this file.                                                *
 *
 *  As a special exception, permission is given to link this program       *
 *  with any version of the KADMOS ocr/icr engine of reRecognition GmbH,   *
 *  Kreuzlingen and distribute the resulting executable without            *
 *  including the source code for KADMOS in the source distribution.       *
 *
 *  As a special exception, permission is given to link this program       *
 *  with any edition of Qt, and distribute the resulting executable,       *
 *  without including the source code for Qt in the source distribution.   *
 *                                                                         *
 ***************************************************************************/

#include <qlayout.h>
#include <qlabel.h>
#include <qlistview.h>

#include <kcombobox.h>

#include <kdebug.h>
#include <klocale.h>
#include <kfiletreebranch.h>

#include "imgnamecombo.h"
#include "img_saver.h"

ImageNameCombo::ImageNameCombo( QWidget *parent )
   : KComboBox( parent )
{
   setInsertionPolicy( QComboBox::AtTop );
}

ImageNameCombo::~ImageNameCombo()
{
   
}

void ImageNameCombo::slotPathRemove( KFileTreeBranch *branch, const QString& relPath )
{
   QString path = branch->name() + QString::fromLatin1(" - ") + relPath;

   kdDebug(28000) << "ImageNameCombo: Removing " << path << endl;
   QString select = currentText();
   
   if( items.contains( path ))
   {
      kdDebug(28000) << "ImageNameCombo: Item exists-> deleting" << endl;
      items.remove( path );
   }

   /* */
   rewriteList( branch, select );
}

void ImageNameCombo::rewriteList( KFileTreeBranch *branch, const QString& selText )
{
   clear();
   for ( QStringList::Iterator it = items.begin(); it != items.end(); ++it )
   {
      insertItem( branch->pixmap(), *it );
   }

   int index = items.findIndex( selText );
   setCurrentItem( index );
}

void ImageNameCombo::slotGalleryPathChanged( KFileTreeBranch* branch, const QString& relativPath )
{
   QString newPath;

   newPath = branch->name() + QString::fromLatin1(" - ") + relativPath;

   kdDebug( 28000) << "Inserting " << newPath << " to combobox" << endl;

   setCurrentItem( newPath, true /* insert if missing */ );
}

/* The End */
#include "imgnamecombo.moc"
