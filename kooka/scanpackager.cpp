/***************************************************************************
                          scanpackager.cpp  -  description                              
                             -------------------                                         
    begin                : Fri Dec 17 1999                                           
    copyright            : (C) 1999 by Klaas Freitag                         
    email                : freitag@suse.de

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

#include "scanpackager.h"
#include "packageritem.h"
#include "miscwidgets.h"
#include "resource.h"

#include <qapplication.h>
#include <qdir.h>
#include <qfile.h>
#include <qpopupmenu.h>
#include <qdict.h>
#include <qpixmap.h>
#include <kmessagebox.h>
#include <qfiledialog.h>

#include <kfiledialog.h>
#include <kurl.h>
#include <kdebug.h>
#include <klocale.h>
#include <progressbase.h>
#include <kio/jobclasses.h>
#include <kio/file.h>
#include <kio/job.h>
#include <kio/jobclasses.h>

extern QDict<QPixmap> icons;
enum { ID_POP_RESCAN,
       ID_POP_DELETE,
       ID_POP_CREATE_NEW,
       ID_POP_UNLOAD_IMG,
       ID_POP_SAVE_IMG  };

/* ----------------------------------------------------------------------- */
/* Constructor Scan Packager */
ScanPackager::ScanPackager( QWidget *parent ) : KListView( parent )
{
	setFrameStyle( QFrame::Sunken | QFrame::Box);
	addColumn( i18n("Scan packages") );
	addColumn( i18n("Size") );   setColumnAlignment( 1, AlignRight );
	addColumn( i18n("Colors") );	setColumnAlignment( 2, AlignRight );
	
	QDir home = QDir::home();
	save_root = home.absPath() + "/.kscan/";
	
	setRootIsDecorated( true );
	root = new PackagerItem( this, true );
	root->setFilename( save_root );
	root->createFolder();
	
	root->setText( 0, i18n("Image collection") );
	
	connect( this, SIGNAL( selectionChanged( QListViewItem*)),
		 SLOT( slSelectionChanged(QListViewItem*)));

	connect( this, SIGNAL( rightButtonPressed( QListViewItem *, const QPoint &, int )),
		 SLOT( slShowContextMenue(QListViewItem *, const QPoint &, int )));
	
	img_counter = 1;
	if( readDir( root, save_root ) == 0 )
	{
		PackagerItem *incoming = new PackagerItem( root, true );
		incoming->setText( 0, i18n("incoming") );
	}
	setOpen( root, true );
	setSelected( root->firstChild(), true );
	
	/* Set the current export dir to home */
	curr_copy_dir = QDir::home();
}

/* ----------------------------------------------------------------------- */
/* This slot is called if the selection changes. */
void ScanPackager::slSelectionChanged( QListViewItem *newItem )
{
	if( ! newItem ) return;
	QString s;
	PackagerItem *pi = (PackagerItem*) newItem;
	
	if( pi->isDir())
	{
		kdDebug(28000) << "selectionChanged: Is a father" << endl;
	 	/* no action, because its a dir */
	}
	else
	{
		kdDebug(28000) << "selectionChanged: Is loaded image !" << endl;
		QApplication::setOverrideCursor(waitCursor);
		emit( showImage( pi->getImage() ));
		QApplication::restoreOverrideCursor();
  			
 	}
}



QString ScanPackager::getCurrImageFileName( bool withPath = true ) const
{
   QString result = "";
   
   PackagerItem *curr = (PackagerItem*) currentItem();
   if( ! curr )
   {
      curr = root;
   }

   if( curr )
   {
      result = curr->getFilename( withPath );
   }
   return( result );
}

/* ----------------------------------------------------------------------- */
/* Called if the image exists but was changed by image manipulation func   */
void ScanPackager::slotImageChanged( QImage *img )
{
   PackagerItem *curr = (PackagerItem*) currentItem();
   if( ! curr )
   {
      curr = root;
   }

   /* Do not save directories */
   if( curr->isDir() ) return;
   
   /* find the directory above */
   const QString filename = curr->getFilename( );

   ImgSaver saver( this );
   ImgSaveStat is_stat = ISS_OK;
   is_stat = saver.saveImage( img, filename );

   if( is_stat == ISS_ERR_FORMAT_NO_WRITE )
   {
      KMessageBox::error( this, i18n( "Cannot write this image format.\nImage will not be saved!"),
			    i18n("Save error") );
   }
   else if( is_stat == ISS_ERR_PERM )
   {
      KMessageBox::error( this, i18n( "Image file is write protected.\nImage will not be saved!"),
			    i18n("Save error") );

   }
   else if( is_stat != ISS_OK )
   {
      kdDebug(28000) << "Error while saving existing image !" << endl;
   }

   if( img && !img->isNull())
   {
      curr->setImage( img );
      emit( showImage( curr->getImage()));
   }
}


/* ----------------------------------------------------------------------- */
/* This slot takes a new scanned Picture and saves it.
 * It urgently needs to make a deep copy of the image !
 */
void ScanPackager::slAddImage( QImage *img )
{
   ImgSaveStat is_stat = ISS_OK;
   /* Save the image with the help of the ImgSaver */
   if( ! img ) return;
   /* currently selected item */
   PackagerItem *curr = (PackagerItem*) currentItem();
   if( ! curr )
   {
      curr = root;
   }
   /* find the directory above */
   while( ! curr->isDir())
   {
      curr = (PackagerItem*) curr->parent();
   }

   /* Path of curr sel item */
   QDir d (curr->getFilename());

   ImgSaver img_saver( this, d.absPath() );

   is_stat = img_saver.saveImage( img );

   if( is_stat != ISS_OK )
   {
      if( is_stat == ISS_SAVE_CANCELED )
      {
	 return;
      }
      kdDebug(28000) << "ERROR: Saving failed: " << img_saver.errorString( is_stat ) << endl;
      /* And now ?? */
   }

   /* Add the new image to the list of new images */

   setSelected( curr, true );
   PackagerItem *item = new PackagerItem( curr, false );
   item->setFilename( d.absPath() + "/" + img_saver.lastFilename());
   QString s;

   /* Count amount of children of the father */
   int childcount = curr->childCount();
   s = i18n("%1 images").arg(childcount);
   curr->setText( 1, s);
   s = i18n("image %1").arg(childcount);
   item->setText( 0, s);
   setOpen( curr, true );

   /* This call to set image allocs a new object qimage, which lives longer
    * than the one from the scanner, which will be destroyed after leaving
    * this callback.
    * All actions depending on the *pointer* to the qimage need to be done
    * after this call.
    */
   item->setImage( img );

   /* makes the new item the current, which shows it */
   setSelected( item, true );

}

/* ----------------------------------------------------------------------- */
/* selects and opens the file with the given name */
void ScanPackager::slSelectImage( const QString name )
{
   QListViewItem *found = spFindItem( NameSearch, name );

   if( found )
   {
      kdDebug(28000) << "slSelectImage: Found an item !" << endl;
      ensureItemVisible( found );
      setCurrentItem( found );
      slSelectionChanged( found );
   }
}


PackagerItem *ScanPackager::spFindItem( SearchType type, const QString name )
{
   PackagerItem *foundItem = 0L;
   bool searchError = false;

   QListViewItemIterator it( root );
   for ( ; it.current() && !foundItem && !searchError; ++it )
   {
      switch( type )
      {
	 case NameSearch:
	    if( ((PackagerItem*) it.current())->getFilename( true ) == name )
	    {
	       foundItem = (PackagerItem*)(it.current());
	    }

	    break;
	 default:
	    kdDebug(28000) << "Scanpackager: Wrong search type !" << endl;
	    searchError = true;
      }
   }
   return( foundItem );
}

/* ----------------------------------------------------------------------- */
void ScanPackager::slShowContextMenue(QListViewItem *lvi, const QPoint &p, int col )
{
   kdDebug(28000) << "Showing Context Menue" << endl;
   (void) col;

   PackagerItem *curr = 0;

   if( lvi )
   {
      curr = (PackagerItem*) lvi;
      if( curr->isDir() )
	 setSelected( curr, true );
   }

   QPopupMenu *popup = new QPopupMenu();
   popup->setCheckable( true );

   // debug ( "opening popup!" );
   if( curr )
   {
      popup->insertItem( *icons["mini-ray"], i18n("Unload image"), ID_POP_UNLOAD_IMG );
      popup->insertItem( *icons["mini-floppy"], i18n("Export image"), ID_POP_SAVE_IMG );
      popup->setItemEnabled ( ID_POP_CREATE_NEW, ! curr->isDir() );
      popup->insertSeparator();
      popup->insertItem( *icons["mini-trash"], i18n("Delete image"), ID_POP_DELETE );
      popup->insertSeparator();
   }

   popup->insertItem( *icons["mini-folder_new"], "Create new folder", ID_POP_CREATE_NEW );
   popup->setItemEnabled ( ID_POP_CREATE_NEW, curr->isDir() );

   connect( popup, SIGNAL( activated(int)), this, SLOT(slHandlePopup(int)));
   popup->move( p );
   popup->show();

}

/* ----------------------------------------------------------------------- */
void ScanPackager::slHandlePopup( int menu_id )
{
   kdDebug(28000) << "Popup to handle ID: " << menu_id << endl;

   PackagerItem *curr = (PackagerItem*) currentItem();
   if( ! curr ) return;
   PackagerItem *newitem = (PackagerItem*) curr->itemAbove();
   bool setnew = false;

   switch( menu_id )
   {
      case ID_POP_RESCAN:
	 kdDebug(28000) << "Not yet implemented !" << endl;
	 break;
      case ID_POP_DELETE:
	 if( curr && curr != root ) {
	    if( deleteItem( curr, true ))
	       setnew = true;
	 }
	 break;
      case ID_POP_CREATE_NEW:
	 createFolder();
	 break;
      case ID_POP_UNLOAD_IMG:
	 if( curr )
	    unloadItem( curr );
	 if( !curr->isLoaded() )
	    emit( showImage( 0 ));
	 setnew = true;
	 break;
      case ID_POP_SAVE_IMG:
	 if( curr ) exportFile( curr );

	 break;
      default:
	 break;
   }

   /* need to set a new item as actual item ? */
   if( setnew ) {
      setCurrentItem( newitem );
      setSelected( newitem, true );
      slSelectionChanged( newitem );
   }

}
/* ----------------------------------------------------------------------- */

void ScanPackager::exportFile( PackagerItem *curr )
{

   if( curr->isDir() )
   {
      kdDebug(28000) << "Not yet implemented!" << endl;
   }
   else
   {
      QString from_file = curr->getFilename();

      QString filter = "*." + curr->getImgFormat();
      QString initial = curr_copy_dir.absPath() + "/";
      initial += curr->getFilename(false);
      KURL fileName = KFileDialog::getSaveURL ( initial,
						filter,
						this, "COPY_SAVE_DIA" );

      if ( fileName.isValid() )                  // got a file name
      {
	 /* remember the filename for the next export */
	 curr_copy_dir = fileName.path( );
	 KURL kfrom( from_file );

	 if( kfrom == fileName ) return;
	 copyjob = KIO::copy( kfrom, fileName, false );

	 connect( copyjob, SIGNAL(result(KIO::Job*)), this, SLOT(slotCanceled( KIO::Job* )));

      }
   }
}

void ScanPackager::slotCanceled( KIO::Job* )
{
  kdDebug(28000) << i18n("Canceled by user") << endl;
}


void ScanPackager::slotResult( KIO::Job *job )
{
   if( ! job ) return;

   if ( job->error() )
   {
      job->showErrorDialog ( );
      // get formated error message
      QString msg = job->errorString();
      // int errid = job->error();
      kdDebug(28000) << "ERROR in Exporting an image: <" << msg << ">" << endl;
   }
   copyjob = 0L;
}





/* ----------------------------------------------------------------------- */
void ScanPackager::unloadItem( PackagerItem *curr )
{
   if( curr->isDir())
   {
      PackagerItem  *child = (PackagerItem*) curr->firstChild();
      while( child )
      {
	 unloadItem( child );
	 child = (PackagerItem*) child->nextSibling();
      }
   }
   else
   {
      emit( unloadImage( curr->getImage() ));
      curr->unload();
   }
}

/* ----------------------------------------------------------------------- */
bool ScanPackager::deleteItem( PackagerItem *curr, bool ask )
{
   bool ok = false;
   if( ! curr ) return( ok );
   kdDebug(28000) << "Deleting: " << curr->getFilename() << endl;
   if( curr->isDir() )
   {
      /* Remove all files and the dir */
      PackagerItem  *child = (PackagerItem*) curr->firstChild();
      PackagerItem *new_child;

      QFileInfo d( curr->getFilename());

      if( ask )
      {
	 QString s;
	 s = i18n("Do you really want to delete the folder %1\nand all the images inside ?").arg(d.baseName());
	 int result = KMessageBox::questionYesNo(this, s, i18n( "Delete collection item") );
	 if( result == KMessageBox::No )
	    return( false );

      }

      ok = true;
      while( child && ok )
      {
	 kdDebug(28000) << "deleting " << child->getFilename() << endl;
	 new_child = (PackagerItem*) child->nextSibling();
	 ok = deleteItem( child, false );

	 child = new_child;
      }

      if( ok )
	 ok = curr->deleteFolder();

      if( ok )
      {
	 if( curr->parent())
	    setSelected( curr->parent(), true);
	 delete curr;
	 curr = 0;
      }
   }
   else
   {
      if( ask )
      {
	 QString s;
	 s = i18n("Do you really want to delete this image ?\nIt can't be restored !" );
	 int result = KMessageBox::questionYesNo(this, s, i18n("Delete image") );

	 if( result == KMessageBox::No )
	    return( false );

      }

      /* delete a normal file */
      emit( deleteImage( curr->getImage() ));
      ok = curr->deleteFile();
      delete curr;
   }

   if( ! ok )
      kdDebug(28000) << "Deleting Item failed !" << endl;

   return( ok );
}

/* ----------------------------------------------------------------------- */
void ScanPackager::createFolder( void )
{
   EntryDialog d( this, i18n("New Folder"), i18n("<B>Please enter a name for the new folder:</B>"));
		  
   if( d.exec() == QDialog::Accepted )
   {
      QString folder = d.getText();
      if( folder.length() > 0 )
      {
	 PackagerItem *curr = (PackagerItem*)currentItem();
	 if( curr )
	 {
	    PackagerItem *new_item = new PackagerItem( curr, true );
	    CHECK_PTR( new_item );
	    new_item->setFilename( curr->getFilename() + "/" + folder );
	  	 		
	    new_item->createFolder();
	    setCurrentItem( new_item );
	    ensureItemVisible( new_item );
	 }
      }
   }
}

/* ----------------------------------------------------------------------- */
int ScanPackager::readDir( QListViewItem *parent, QString dir_to_read )
{
   if ( !dir_to_read ) return( 0 );
   QString s;	
   int amount_dirs = 0;
   QDir  dir( dir_to_read );

   dir.setFilter( QDir::Files | QDir::Dirs | QDir::NoSymLinks );

   const QFileInfoList *list = dir.entryInfoList();
   QFileInfoListIterator it( *list );      // create list iterator
   QFileInfo *fi;                          // pointer for traversing
	

   ++it;  ++it;  /* Skip the current and parent-dir */
   while ( (fi=it.current()) )
   {           // for each file...
      if( fi->isDir() )
      {
	 PackagerItem *new_folder = new PackagerItem( parent, true );
	 new_folder->setFilename( fi->absFilePath() );
	 /* recursive Call to readDir */
	 amount_dirs++;
	 amount_dirs += readDir( new_folder, fi->absFilePath() );
	 new_folder->decorateFile();
      }
      else
      {
	 /* its a file */
	 PackagerItem *item = new PackagerItem( parent, false );
	 item->setFilename( fi->absFilePath() );
		 	
	 uint si = fi->size();
	 s = "";

	 if( si > 1024*1024 )
	 {  /* Size is MegaByte */
            s = i18n("%1 MB").arg(KGlobal::locale()->formatNumber(si/1024/1024));
	 }
	 else if( si > 1024 )
	 {
            s = i18n("%1 kB").arg(KGlobal::locale()->formatNumber(si/1024, 0));
	 }
	 else
	 {
            s = i18n("%1 bytes").arg(si);
	 }
	 item->setText( 1, s );
      }

      ++it;                               // goto next list element
   }
   return( amount_dirs );
}


/* ----------------------------------------------------------------------- */
QString ScanPackager::getImgName( QString name_on_disk )
{
   QString s;
   (void) name_on_disk;

   s = i18n("image %1").arg(img_counter++);
   return( s );     	
}

/* ----------------------------------------------------------------------- */
ScanPackager::~ScanPackager(){
}
#include "scanpackager.moc"
