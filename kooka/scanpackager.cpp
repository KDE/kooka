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
#include "miscwidgets.h"
#include "resource.h"
#include "img_saver.h"
#include "kookaimage.h"


#include <qapplication.h>
#include <qdir.h>
#include <qfile.h>
#include <qpopupmenu.h>
#include <qdict.h>
#include <qpixmap.h>
#include <kmessagebox.h>
#include <qfiledialog.h>
#include <qstringlist.h>
#include <qheader.h>

#include <kfiletreeview.h>
#include <kfiletreeviewitem.h>
#include <kfiletreebranch.h>

#include <kpopupmenu.h>
#include <kaction.h>
#include <kiconloader.h>
#include <kfiledialog.h>
#include <kurl.h>
#include <kdebug.h>
#include <klocale.h>
#include <kglobal.h>
#include <kio/progressbase.h>
#include <kio/netaccess.h>
#include <kio/jobclasses.h>
#include <kio/file.h>
#include <kio/job.h>

extern QDict<QPixmap> icons;

/* ----------------------------------------------------------------------- */
/* Constructor Scan Packager */
ScanPackager::ScanPackager( QWidget *parent ) : KFileTreeView( parent )
{
   // TODO: setItemsRenameable (true );
	addColumn( i18n("Image Name" ));
	setColumnAlignment( 0, AlignLeft );
	
	addColumn( i18n("Size") );
	setColumnAlignment( 1, AlignRight );
	setColumnAlignment( 2, AlignRight );

	addColumn( i18n("Format" )); setColumnAlignment( 3, AlignRight );

	/* Drag and Drop */
	setDragEnabled( true );
	setDropVisualizer(true);
	setAcceptDrops(true);
	connect( this, SIGNAL(dropped(QDropEvent*, QListViewItem*)),
		 this, SLOT( slDropped(QDropEvent*, QListViewItem*)));
	kdDebug(28000) << "connected Drop-Signal" << endl;
	setRenameable ( 0, true );
	setRenameable ( 1, false );
	setRenameable ( 2, false );
	setRenameable ( 3, false );

	setRootIsDecorated( false );
	
	
	connect( this, SIGNAL( selectionChanged( QListViewItem*)),
		 SLOT( slSelectionChanged(QListViewItem*)));

	connect( this, SIGNAL( rightButtonPressed( QListViewItem *, const QPoint &, int )),
		 SLOT( slShowContextMenue(QListViewItem *, const QPoint &, int )));
	
	connect( this, SIGNAL(itemRenamed (QListViewItem*, const QString &, int ) ), this,
		 SLOT(slFileRename( QListViewItem*, const QString&, int)));
	
	img_counter = 1;
	/* Set the current export dir to home */
	curr_copy_dir = QDir::home();

	/* Preload frequently used icons */
	KIconLoader *loader = KGlobal::iconLoader();
	m_floppyPixmap = loader->loadIcon( "3floppy_unmount", KIcon::Small );
	m_grayPixmap = loader->loadIcon( "palette_gray", KIcon::Small );
	m_bwPixmap = loader->loadIcon( "palette_lineart", KIcon::Small );
	m_colorPixmap = loader->loadIcon( "palette_color", KIcon::Small );

	   
	popup =0L;
	/* open the image galleries */
	openRoots();

	createMenus();
}

void ScanPackager::openRoots()
{
   /* First open the standard root */
   QString localRoot = ImgSaver::kookaImgRoot();
   
   KURL rootUrl(localRoot);

   /* standard root always exists, ImgRoot creates it */
   kdDebug(28000) << "Open standard root " << rootUrl.url();

   KFileTreeBranch *newbranch = addBranch( rootUrl, i18n("Kooka Gallery"), false /* do not showHidden */ );
   setDirOnlyMode( newbranch, false );
   newbranch->setShowExtensions( false );

   connect( newbranch, SIGNAL(newTreeViewItems( KFileTreeBranch*, const KFileTreeViewItemList& )),
	    this, SLOT( slotDecorate(KFileTreeBranch*, const KFileTreeViewItemList& )));

   connect( newbranch, SIGNAL(populateFinished( KFileTreeViewItem* )),
	    this, SLOT( slotSetChildCount( KFileTreeViewItem* )));
   
   populateBranch( newbranch );
   
   /* open more configurable image repositories TODO */

   /* select Incoming-Dir, TODO restore last selection ! */
   KFileTreeViewItem *startit = findItem( newbranch, i18n( "Incoming" ) );
   if( ! startit ) kdDebug(28000) << "No Start-Item found :(" << endl;
   setCurrentItem( startit );
}

void ScanPackager::createMenus()
{
   if( ! popup )
   {
      popup = new KPopupMenu();
      // popup->insertTitle( i18n( "Gallery" ));
      
      KAction *newAct = new KAction(i18n("&create directory"), "folder",0 ,
				    this, SLOT(slotCreateFolder()), this);
      newAct->plug( popup );	
      
      newAct = new KAction(i18n("&save Image"), "filesave",0 ,
			   this, SLOT(slotExportFile()), this);
      newAct->plug( popup );	

      newAct = new KAction(i18n("&delete Image"), "edittrash",0 ,
			   this, SLOT(slotDeleteItems()), this);
      newAct->plug( popup );	

      newAct = new KAction(i18n("&unload Image"), "unload", 0,
			   this, SLOT(slotUnloadItems()), this );
      newAct->plug( popup );
      popup->setCheckable( true );
   }
}


void ScanPackager::slotDecorate( KFileTreeViewItem* item )
{
   kdDebug(28000) << "decorating slot !" << endl;
   if( !item ) return;

   if( item->isDir())
   {
      QString cc = i18n( "%1 items").arg( item->childCount());
      item->setText( 1, cc );
   }
   else
   {
      KFileItem *kfi = item->fileItem();

      KookaImage *img = 0L;
      
      if( kfi )
      {
	 img = static_cast<KookaImage*>(kfi->extraData( this ));
      }
      kdDebug(28000) << "KookaImage: " << img << endl;

      if( img )
      {

	 /* The image appears to be loaded to memory. */
	 if( img->depth() == 1 )
	 {
	    /* a bw-image */
	    item->setPixmap( 0, m_bwPixmap );
	 }
	 else
	 {
	    if( img->isGrayscale() )
	    {
	       item->setPixmap( 0, m_grayPixmap );
	    }
	    else
	    {
	       item->setPixmap( 0, m_colorPixmap );
	    }
	 }
	 
	 /* set image size in pixels */
	 QString t = i18n( "%1 x %2" ).arg( img->width()).arg(img->height());
	 item->setText( 1, t );
	 kdDebug( 28000) << "Image laoded and decorated!" << endl;
      }
      else
      {
	 /* Item is not yet loaded. Display file information */
	 item->setPixmap( 0, m_floppyPixmap );
	 KIO::filesize_t s = kfi->size();
	 double dsize = s / 1024.0;  /* calc kilobytes */

	 QString sizeStr;
	 if( dsize > 999.99 )
	 {
	    dsize = dsize / 1024.0;
	    sizeStr = i18n( "%1 MB").arg( dsize, 0, 'f', 2 );
	 }
	 else
	 {
	    sizeStr = i18n( "%1 kB").arg( dsize, 0,'f', 1 );
	 }
	 item->setText(1, sizeStr );
      }

      /* Image format */
      QString format = getImgFormat( item );
      item->setText( 2, format );
   }
}




void ScanPackager::slotDecorate( KFileTreeBranch* branch, const KFileTreeViewItemList& list )
{
   (void) branch;
   kdDebug(28000) << "decorating slot for list !" << endl;

   KFileTreeViewItemListIterator it( list );

   bool end = false;
   for( ; !end && it.current(); ++it )
   {
      KFileTreeViewItem *kftvi = *it;
      slotDecorate( kftvi );
   }
}

void ScanPackager::slotSetChildCount( KFileTreeViewItem *parent )
{
   if( parent )
   {
      QString cc = i18n( "%1 items").arg( parent->childCount());
      parent->setText( 1, cc );
   }
}


void ScanPackager::slFileRename( QListViewItem* it, const QString& newStr, int col )
{
   (void) it;
   (void) newStr;
   (void) col;
}


/* ----------------------------------------------------------------------- */
/*
 * Method that checks if the new filename a user enters while renaming an image is valid.
 * It checks for a proper extension.
 */
QString ScanPackager::buildNewFilename( QString cmplFilename, QString currFormat ) const
{
   /* cmplFilename = new name the user wishes.
    * currFormat   = the current format of the image.
    * if the new filename has a valid extension, which is the same as the
    * format of the current, fine. A ''-String has to be returned.
    */
   QFileInfo fiNew( cmplFilename );
   QString base = fiNew.baseName();
   QString newExt = fiNew.extension( false ).lower();
   QString nowExt = currFormat.lower();
   QString ext = "";

   kdDebug(28000) << "Filename wanted: "<< cmplFilename << " <"<<newExt<<"> <" << nowExt<<">" <<endl;
   
   if( newExt == "" )
   {
      /* ok, fine -> return the currFormat-Extension */
      ext = base + "." + currFormat;
   }
   else if( newExt == nowExt )
   {
      /* also good, no reason to put another extension */
      ext = cmplFilename;
   }
   else
   {
      /* new Ext. differs from the current extension. Later. */
      KMessageBox::sorry( 0L, i18n( "You entered a file extension that differs from the existing one.\nThat is not yet possible. Converting 'on the fly' is planned for a future release.\n\n"
				      "Kooka corrects the extension."),
			  i18n("On the fly conversion"));
      ext = base + "." + currFormat;
   }
   return( ext );
}

/* ----------------------------------------------------------------------- */
/* This method returns the directory of an image or directory.
 */
QString ScanPackager::itemDirectory( const KFileTreeViewItem* item, bool relativ ) const
{
   QString relativUrl= (item->url()).prettyURL();
   
   if( ! item->isDir() )
   {
      // Cut off the filename in case it is not a dir
      relativUrl.truncate( relativUrl.findRev( '/' )+1);
   }
   else
   {
      /* add a "/" to the directory if not there */
      if( ! relativUrl.endsWith( "/" ) )
	 relativUrl.append( "/" );
   }
      
   if( relativ )
   {
      KFileTreeBranch *branch = item->branch();
      if( branch )
      {
	 kdDebug(28000) << "Relativ URL of the file " << relativUrl << endl;
	 QString rootUrl = (branch->rootUrl()).prettyURL();  // directory of branch root
      
	 if( relativUrl.startsWith( rootUrl ))
	 {
	    relativUrl.remove( 0, rootUrl.length() );

	    if( relativUrl == "" ) relativUrl = "/"; // The root
	 }
	 else
	 {
	    kdDebug(28000) << "ERR: Item-URL does not start with root url " << rootUrl << endl;
	 }
      }
   }
   return( relativUrl );
}
/* ----------------------------------------------------------------------- */
/* This slot receives a string from the gallery-path combobox shown under the
 * image gallery. The form of the string coming in here is <branch-name> - <
 * relativ directory under the branch. Now it is to assemble a complete path
 * from the data, find out which KFileTreeViewITem is associated with it and
 * call slSelectionChanged with it.
 */
 
void ScanPackager::slotSelectDirectory( const QString & dirString )
{
   kdDebug(28000) << "Trying to decode directory string " << dirString << endl;

   QString searchFor = QString::fromLatin1(" - ");
   int pos = dirString.find( searchFor );

   if( pos > -1 )
   {
      /* Splitting up the string coming in */
      QString branchName = dirString.left( pos );
      QString relPath( dirString );
      
      relPath = relPath.remove( 0, pos + searchFor.length());

      kdDebug(28000) << "Splitted up to branch <" << branchName << "> and <" << relPath << endl;
      
      KFileTreeViewItem *kfi = findItem( branchName, relPath );
      
      if( kfi )
      {
	 kdDebug(28000) << "got a new item to select !" << endl;
	 ensureItemVisible(kfi);
	 setCurrentItem(kfi);
      }
   }
}

/* ----------------------------------------------------------------------- */
/* This slot is called if the selection changes.                           */
void ScanPackager::slSelectionChanged( QListViewItem *newItem )
{
   KFileTreeViewItem *item = static_cast<KFileTreeViewItem*>(newItem);

   if( item )
   {
      kdDebug(28000) << "SelectionChanged - newItem !" << endl;
      /* Check if directory, hide image for now, later show a thumb view */
      if( item->isDir())
      {
	 kdDebug(28000) << "selectionChanged: Is a directory !" << endl;
	 emit( showImage( 0L ));
      }
      else
      {
	 /* if not a dir, load the image if neccessary. This is done by loadImageForItem,
	  * which is async( TODO ). The image finally arrives in slotImageArrived */
	 QApplication::setOverrideCursor(waitCursor);
	 loadImageForItem( item );
	 QApplication::restoreOverrideCursor();
      }

      /* emit a signal indicating the new directory if there is a new one */
      QString wholeDir = itemDirectory( item, false ); /* not relativ to root */
      
      if( currSelectedDir != wholeDir )
      {
	 currSelectedDir = wholeDir;
	 QString relativUrl = itemDirectory( item, true );
	 kdDebug(28000) << "Emitting " << relativUrl << " as new relative Url" << endl;
	 /* Emit the signal with branch and the relative path */
	 emit( galleryPathSelected( item->branch(), relativUrl ));
      }
      else
      {
	 // kdDebug(28000) << "directroy is not new: " << currSelectedDir << endl;
      }
   }
   else
   {
      kdDebug(28000) << "ERR: Failed to get a valid treeviewitem" << endl;
   }
}

void ScanPackager::loadImageForItem( KFileTreeViewItem *item )
{

   if( ! item ) return;
   
   KURL url = item->url();
   
   if( url.isLocalFile() )
   {
      KookaImage *img = new KookaImage( ); 
      if( img->load( localFilename(item) ) )
      {
	 slImageArrived( item, img );
      }
   }
   else
   {
      // TODO : non-local images.
      kdDebug(28000) << "Non-local images: Not yet implemented!" << endl;
   }
}

/* Hit this slot with a file for a kfiletreeviewitem. */
void ScanPackager::slImageArrived( KFileTreeViewItem *item, KookaImage* image )
{
   if( item && image )
   {
      /* Associate the image for the Scanpackager-Object. */
      KFileItem *kfi = item->fileItem();
      if( kfi )
      {
	 kfi->setExtraData( (void*) this, (void*) image );
      }
      slotDecorate( item );
      emit( showImage( image ));
   }
}



QString ScanPackager::getCurrImageFileName( bool withPath = true ) const
{
   QString result = "";
   
   KFileTreeViewItem *curr = currentKFileTreeViewItem();
   if( ! curr )
   {
      kdDebug( 28000) << "getCurrImageFileName: nothing selected !"<< endl;
   }
   else
   {
      if( withPath )
      {
	 result = localFilename(curr);
      }
      else
      {
	 KURL url( localFilename(curr));
	 url = curr->url();
	 result = url.filename();
      }
   }
   return( result );
}

/* ----------------------------------------------------------------------- */
QCString ScanPackager::getImgFormat( KFileTreeViewItem* item ) const 
{

   QCString cstr;
   
   if( !item ) return( cstr );
   
   KFileItem *kfi = item->fileItem();
   
   QString mime = kfi->mimetype();

   
   kdDebug(28000) << "Mimetype found: "<< mime << endl;

   // TODO find the real extension for use with the filename !
   // temporarely:
   QString f = localFilename( item );

   return( QImage::imageFormat( f ));
   
}

QString ScanPackager::localFilename( KFileTreeViewItem *it ) const
{
   if( ! it ) return( QString::null );

   KURL url = it->url();
   kdDebug(28000)<< "localFilename: URL is " << url.prettyURL() << endl;

   QString res;

   if( url.isLocalFile())
   {
      res = url.directory( false, true ) + url.fileName();
   }

   return( res );
    
}

/* Called if the image exists but was changed by image manipulation func   */
void ScanPackager::slotImageChanged( QImage *img )
{
   KFileTreeViewItem *curr = currentKFileTreeViewItem();
   if( ! curr )
   {
      kdDebug(28000) << "ImageChanged: nothing selected !" << endl;
      return;
   }

   /* Do not save directories */
   if( curr->isDir() ) return;
   
   /* unload image and free memory */
   slotUnloadItem( curr );
   
   const QString filename = localFilename( curr );
   const QCString format = getImgFormat( curr );
   ImgSaver saver( this );
   ImgSaveStat is_stat = ISS_OK;
   is_stat = saver.saveImage( img, filename, format );

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
   else if( is_stat == ISS_ERR_PROTOCOL )
   {
      KMessageBox::sorry( this, i18n( "Cannot save the image, because the file is local.\n"
				      "Kooka will support other protocols later."),
			    i18n("Save error") );

   }
   else if( is_stat != ISS_OK )
   {
      kdDebug(28000) << "Error while saving existing image !" << endl;
   }

   if( img && !img->isNull())
   {
      KookaImage *newImage = new KookaImage(*img);
      slImageArrived( curr, newImage );
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

   /* currently selected item is the directory or a file item */
   KFileTreeViewItem *curr = currentKFileTreeViewItem();

   /* Use root if nothing is selected */
   if( ! curr )
   {
      curr = root;
   }

   /* find the directory above the current one */
   KURL dir(itemDirectory( curr ));

   /* Path of curr sel item */
   ImgSaver img_saver( this, dir );

   is_stat = img_saver.saveImage( img );
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
      if( is_stat == ISS_SAVE_CANCELED )
      {
	 return;
      }
      kdDebug(28000) << "ERROR: Saving failed: " << img_saver.errorString( is_stat ) << endl;
      /* And now ?? */
   }

   /* Add the new image to the list of new images */
   slotSetNextUrlToSelect( KURL( img_saver.lastFilename()) );

   QString s;
   /* Count amount of children of the father */
   int childcount = curr->childCount();
   s = i18n("%1 images").arg(childcount);
   curr->setText( 1, s);
   setOpen( curr, true );

}

/* ----------------------------------------------------------------------- */
/* selects and opens the file with the given name. This is used to restore the
 * last displayed image by its name.
 */
void ScanPackager::slSelectImage( const KURL& name )
{

   KFileTreeViewItem *found = spFindItem( UrlSearch, name.url() );

   if( found )
   {
      kdDebug(28000) << "slSelectImage: Found an item !" << endl;
      ensureItemVisible( found );
      setCurrentItem( found );
      slSelectionChanged( found );
   }

}


KFileTreeViewItem *ScanPackager::spFindItem( SearchType type, const QString name, const KFileTreeBranch *branch )
{
   /* Prepare a list of branches to go through. If the parameter branch is set, search
    * only in the parameter branch. If it is zero, search all branches returned by
    * kfiletreeview.branches()
    */
   KFileTreeBranchList branchList;

   if( branch )
   {
      branchList.append( branch );
   }
   else
   {
      branchList = branches();
   }
 
   
   KFileTreeBranchIterator it( branchList );
   KFileItem *kfi = 0L;
   KFileTreeViewItem *foundItem = 0L;

   /* Leave the loop in case kfi is defined */
   KFileTreeBranch *branchloop = 0L;
   for( ; !kfi && it.current(); ++it )
   {
      branchloop = *it;
      KURL url(name);
      switch( type )
      {
	 case Dummy:
	    kdDebug(28000) << "Dummy search skipped !" << endl;
	    break;
	 case NameSearch:
	    kdDebug(28000) << "ScanPackager: searching for " << name << endl;
	    kfi = branchloop->findByName( name );
	    break;
	 case UrlSearch:
	    kdDebug(28000) << "ScanPackager: URL search for " << name << endl;
	    kfi = branchloop->find( url );
	    break;	
      default:
	 kdDebug(28000) << "Scanpackager: Wrong search type !" << endl;
	 break;
      }
	 
   }
   if( kfi )
   {
      foundItem = static_cast<KFileTreeViewItem*>(kfi->extraData(branchloop));
      kdDebug(28000) << "spFindItem: Success !" << foundItem << endl;
   }
   return( foundItem );
}

/* ----------------------------------------------------------------------- */
void ScanPackager::slShowContextMenue(QListViewItem *lvi, const QPoint &p, int col )
{
   kdDebug(28000) << "Showing Context Menue" << endl;
   (void) col;

   KFileTreeViewItem *curr = 0;

   if( lvi )
   {
      curr = currentKFileTreeViewItem();
      if( curr->isDir() )
	 setSelected( curr, true );
   }

   if( popup )
   {
      popup->move( p );
      popup->show();
   }

}

/* ----------------------------------------------------------------------- */

void ScanPackager::slotExportFile( )
{

}

void ScanPackager::slotExportFinished( KIO::Job *job )
{
   // nothing yet to do.
   kdDebug(28000) << "Export Finished" << endl;
   jobMap.remove(job);
}







void ScanPackager::slotCanceled( KIO::Job* )
{
  kdDebug(28000) << i18n("Canceled by user") << endl;
}






/* ----------------------------------------------------------------------- */
void ScanPackager::slotUnloadItems( )
{
   KFileTreeViewItem *curr = currentKFileTreeViewItem();
   emit( showImage( 0L ));
   slotUnloadItem( curr );
}

void ScanPackager::slotUnloadItem( KFileTreeViewItem *curr )
{
   if( ! curr ) return;
   
   if( curr->isDir())
   {
      KFileTreeViewItem *child = static_cast<KFileTreeViewItem*>(curr->firstChild());
      while( child )
      {
	 kdDebug(28000) << "Unloading item " << child << endl;
	 slotUnloadItem( child );
	 child = static_cast<KFileTreeViewItem*> (child->nextSibling());
      }
   }
   else
   {
      KFileItem *kfi = curr->fileItem();
      KookaImage *image = static_cast<KookaImage*>(kfi->extraData( this ));
      emit( unloadImage( image ));
      delete image;
      kfi->removeExtraData( this );
      slotDecorate( curr );
   }
}

/* ----------------------------------------------------------------------- */
void ScanPackager::slotDeleteItems( )
{
   bool ok = false;
   KFileTreeViewItem *curr = currentKFileTreeViewItem();
   if( ! curr ) return;
   kdDebug(28000) << "Deleting: " << localFilename(curr) << endl;
   if( curr->isDir() )
   {
#if 0
      /* Remove all files and the dir */
      KFileTreeViewItem  *child = (KFileTreeViewItem*) curr->firstChild();
      KFileTreeViewItem *new_child;

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
	 new_child = (KFileTreeViewItem*) child->nextSibling();
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
#endif
   }
   else
   {
#if 0 
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
      curr = 0;
#endif
   }

   if( ! ok )
      kdDebug(28000) << "Deleting Item failed !" << endl;

   return;
}

/* ----------------------------------------------------------------------- */
void ScanPackager::slotCreateFolder( )
{
   EntryDialog d( this, i18n("New Folder"), i18n("<B>Please enter a name for the new folder:</B>"));
		  
   if( d.exec() == QDialog::Accepted )
   {
      QString folder = d.getText();
      if( folder.length() > 0 )
      {
	 /* KIO create folder goes here */

	 KFileTreeViewItem *it = currentKFileTreeViewItem();
	 if( it )
	 {
	    KURL url = it->url();

	    /* If a directory is selected, the filename needs not to be deleted */
	    if( ! it->isDir())
	       url.setFileName( "" );
	    /* add the folder name from user input */
	    url.addPath( folder );
	    kdDebug(28000) << "Creating folder " << url.prettyURL() << endl;

	    /* Since the new directory arrives in the packager in the newItems-slot, we set a
	     * variable urlToSelectOnArrive here. The newItems-slot will honor it and select
	     * the treeviewitem with that url.
	     */  
	    slotSetNextUrlToSelect( url );
	       
	    if( ! KIO::NetAccess::mkdir( url ))
	    {
	       kdDebug(28000) << "ERR: creation of " << url.prettyURL() << " failed !" << endl;
	    }
	    else
	    {
	       /* created successfully */
	       /* open the branch if neccessary and select the new folder */
	       
	    }
	 }
      }
   }
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
      kdDebug(29000) << "Destructor of ScanPackager" << endl;

}
#include "scanpackager.moc"
