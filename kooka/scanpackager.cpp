/***************************************************************************
                          scanpackager.cpp  -  description                              
                             -------------------                                         
    begin                : Fri Dec 17 1999                                           
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

#include "scanpackager.h"
#include "packageritem.h"
#include "entrydialog.h"
#include "resource.h"

#include <qapplication.h>
#include <qdir.h>
#include <qfile.h>
#include <qpopupmenu.h>
#include <qdict.h>
#include <qpixmap.h>
#include <qmessagebox.h>
#include <qfiledialog.h>

#include <kfiledialog.h>
#include <kurl.h>
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
	addColumn( "Scan-Pakets" );
	addColumn( "size" );   setColumnAlignment( 1, AlignRight );
	addColumn( "Colors" );	setColumnAlignment( 2, AlignRight );
	
	QDir home = QDir::home();
	save_root = home.absPath() + "/.kscan/";
	
	setRootIsDecorated( true );
	root = new PackagerItem( this, true );
	root->setFilename( save_root );
	root->createFolder();
	
	root->setText( 0, "Image collection" );
	
	connect( this, SIGNAL( selectionChanged( QListViewItem*)),
					   SLOT( slSelectionChanged(QListViewItem*)));

	connect( this, SIGNAL( rightButtonPressed( QListViewItem *, const QPoint &, int )),
	               SLOT( slShowContextMenue(QListViewItem *, const QPoint &, int )));
	
	img_counter = 1;
	if( readDir( root, save_root ) == 0 )
	{
		PackagerItem *incoming = new PackagerItem( root, true );
		incoming->setText( 0, "incoming" );
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
		debug( "selectionChanged: Is a father" );
	 	/* no action, because its a dir */
	}
	else
	{
		debug( "selectionChanged: Is loaded image !" );
		QApplication::setOverrideCursor(waitCursor);
		emit( showImage( pi->getImage() ));
		QApplication::restoreOverrideCursor();
  			
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
      debug( "ERROR: Saving failed: %s", (const char*) img_saver.errorString( is_stat ));
      /* And now ?? */
   }
	
   /* Add the new image to the list of new images */
 	
   setSelected( curr, true );
   PackagerItem *item = new PackagerItem( curr, false );
   item->setFilename( d.absPath() + "/" + img_saver.lastFilename());
   QString s;

   /* Count amount of children of the father */
   int childcount = curr->childCount();
   curr->setText( 1, s.sprintf( "%d images", childcount ));
   item->setText( 0, s.sprintf( "image %d", childcount ));
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
void ScanPackager::slShowContextMenue(QListViewItem *lvi, const QPoint &p, int col )
{
 	debug( "Showing Context Menue" );
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
            popup->insertItem( *icons["mini-ray"], I18N("unload image"), ID_POP_UNLOAD_IMG );
            popup->insertItem( *icons["mini-floppy"], I18N("export image"), ID_POP_SAVE_IMG );
            popup->setItemEnabled ( ID_POP_CREATE_NEW, ! curr->isDir() );
            popup->insertSeparator();
            popup->insertItem( *icons["mini-trash"], I18N("delete image"), ID_POP_DELETE );
            popup->insertSeparator();
  	}
  	
        popup->insertItem( *icons["mini-folder_new"], "create new folder", ID_POP_CREATE_NEW );
        popup->setItemEnabled ( ID_POP_CREATE_NEW, curr->isDir() );

  	connect( popup, SIGNAL( activated(int)), this, SLOT(slHandlePopup(int)));
 	popup->move( p );
  	popup->show();

}

/* ----------------------------------------------------------------------- */
void ScanPackager::slHandlePopup( int menu_id )
{
 	debug( "Popup to handle ID: %d", menu_id );
 	
 	PackagerItem *curr = (PackagerItem*) currentItem();
 	if( ! curr ) return;
 	PackagerItem *newitem = (PackagerItem*) curr->itemAbove();
 	bool setnew = false;
 	
 	switch( menu_id )
 	{
 		case ID_POP_RESCAN:
 			debug( "Not yet implemented !" );
 		break;
 		case ID_POP_DELETE:
 			if( curr && curr != root ) {
 				deleteItem( curr, true );
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
      debug( "Not yet implemented!" );
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
   debug( I18N("Canceled by user"));
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
      debug( "ERROR in Exporting an image: <%s>", (const char*) msg );
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
	debug( "Deleting: %s", (const char*) curr->getFilename());
	if( curr->isDir() )
	{
     	/* Remove all files and the dir */
     	PackagerItem  *child = (PackagerItem*) curr->firstChild();
     	PackagerItem *new_child;

     	QFileInfo d( curr->getFilename());
     	
     	if( ask )
     	{
     		QString s;
     		s = "Do you really want to delete the folder " + d.baseName();
     		s += "\nand all the images inside ?";
     	   int result = QMessageBox::warning(this, "Delete collection item", (const char*) s,
	   			   								 QMessageBox::Ok, QMessageBox::Cancel);
			debug( "This is result: %d", result );	   			   								
			if( result == 2 )	   			   								
      		return( false );
     		
      }
     	     	
     	ok = true;
     	while( child && ok )
     	{
     		debug( "deleting %s", (const char*) child->getFilename());
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
     		s = "Do you really want to delete this image ?";
     		s += "\nIt cant be restored !";
     	   int result = QMessageBox::warning(this, "Delete image", (const char*) s,
	   			   								 QMessageBox::Ok, QMessageBox::Cancel);
			// debug( "This is result: %d", result );	   			   								
			if( result == 2 )	   			   								
      		return( false );
     		
      }

 		/* delete a normal file */
 		emit( deleteImage( curr->getImage() ));
 		ok = curr->deleteFile();
 		delete curr;	 	
 	}
 	
 	if( ! ok )
 		debug( "Deleting Item failed !" );
 	
 	return( ok );
}

/* ----------------------------------------------------------------------- */
void ScanPackager::createFolder( void )
{
 	EntryDialog d( this, "New Folder ", "Please enter a new folder name.\n"
 								"This name will be displayed in the List" );
	if( d.exec() == QDialog::Accepted )
	{
	  	QString folder = d.getText();
	  	if( folder.length() > 0 )
	  	{
	  		PackagerItem *curr = (PackagerItem*)currentItem();
	  		if( curr )
	  		{
	  	 		PackagerItem *new_item = new PackagerItem( curr, true );
	  	 		new_item->setFilename( curr->getFilename() + "/" + folder );
	  	 		
	  	 		new_item->createFolder();
	  		}
	  	}
	}
}

/* ----------------------------------------------------------------------- */
int ScanPackager::readDir( QListViewItem *parent, const char *dir_to_read )
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
		s.sprintf("%.2f Mb", double( si/1024/1024 ) );
	    }
	    else if( si > 1024 )
	    {
		s.sprintf("%.0f kb", double(si /1024) );		 	
		// s.setNum( double(si/1024), 'f', 1 );
		// s += " kb";
	    }
	    else
	    {
		s.sprintf("%d byte", si );		 	
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
	
	s.sprintf( "image %d", img_counter++ );
	return( s );     	
}

/* ----------------------------------------------------------------------- */
ScanPackager::~ScanPackager(){
}
