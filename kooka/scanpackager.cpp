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

#include "scanpackager.h"
#include "resource.h"
#include "img_saver.h"
#include "kookaimage.h"
#include "kookaimagemeta.h"
#include "previewer.h"
#include "devselector.h"

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

#include <kurldrag.h>
#include <kpopupmenu.h>
#include <kaction.h>
#include <klineeditdlg.h>
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

#define STARTUP_FIRST_START "firstStart"


/* ----------------------------------------------------------------------- */
/* Constructor Scan Packager */
ScanPackager::ScanPackager( QWidget *parent ) : KFileTreeView( parent )
{
   // TODO:
   setItemsRenameable (true );
   setDefaultRenameAction( QListView::Reject );
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

   connect( this, SIGNAL(dropped( QWidget*, QDropEvent*, KURL::List&, KURL& )),
	    this, SLOT( slotUrlsDropped( QWidget*, QDropEvent*, KURL::List&, KURL& )));

   kdDebug(28000) << "connected Drop-Signal" << endl;
   setRenameable ( 0, true );
   setRenameable ( 1, false );
   setRenameable ( 2, false );
   setRenameable ( 3, false );

   setRootIsDecorated( false );

   connect( this, SIGNAL( clicked( QListViewItem*)),
	    SLOT( slClicked(QListViewItem*)));

   connect( this, SIGNAL( rightButtonPressed( QListViewItem *, const QPoint &, int )),
	    SLOT( slShowContextMenue(QListViewItem *, const QPoint &, int )));

   connect( this, SIGNAL(itemRenamed (QListViewItem*, const QString &, int ) ), this,
	    SLOT(slFileRename( QListViewItem*, const QString&, int)));


   img_counter = 1;
   /* Set the current export dir to home */
   m_currCopyDir = QDir::home().absPath();

   /* Preload frequently used icons */
   KIconLoader *loader = KGlobal::iconLoader();
   m_floppyPixmap = loader->loadIcon( "3floppy_unmount", KIcon::Small );
   m_grayPixmap   = loader->loadIcon( "palette_gray", KIcon::Small );
   m_bwPixmap     = loader->loadIcon( "palette_lineart", KIcon::Small );
   m_colorPixmap  = loader->loadIcon( "palette_color", KIcon::Small );

   m_startup = true;

   /* create a context menu and set the title */
   m_contextMenu = new KPopupMenu();
   static_cast<KPopupMenu*>(m_contextMenu)->insertTitle( i18n( "Gallery" ));

}

void ScanPackager::openRoots()
{
   /* standard root always exists, ImgRoot creates it */
   KURL rootUrl(Previewer::galleryRoot());
   kdDebug(28000) << "Open standard root " << rootUrl.url() << endl;

   openRoot( rootUrl, true );
   m_defaultBranch->setOpen(true);

   /* open more configurable image repositories TODO */
}

KFileTreeBranch* ScanPackager::openRoot( const KURL& root, bool  )
{
   KIconLoader *loader = KGlobal::iconLoader();

   /* working on the global branch. FIXME */
   m_defaultBranch = addBranch( root, i18n("Kooka Gallery"),
				loader->loadIcon( "folder_image", KIcon::Small ),
				false /* do not showHidden */ );

   // Q_CHECK_PTR( m_defaultBranch );
   m_defaultBranch->setOpenPixmap( loader->loadIcon( "folder_blue_open", KIcon::Small ));

   setDirOnlyMode( m_defaultBranch, false );
   m_defaultBranch->setShowExtensions( true ); // false );

   connect( m_defaultBranch, SIGNAL( newTreeViewItems( KFileTreeBranch*, const KFileTreeViewItemList& )),
	    this, SLOT( slotDecorate(KFileTreeBranch*, const KFileTreeViewItemList& )));

   connect( m_defaultBranch, SIGNAL( directoryChildCount( KFileTreeViewItem* , int )),
	    this, SLOT( slotDirCount( KFileTreeViewItem *, int )));

   connect( m_defaultBranch, SIGNAL( deleteItem( KFileItem* )),
	    this, SLOT( slotDeleteFromBranch(KFileItem*)));

   connect( m_defaultBranch, SIGNAL( populateFinished( KFileTreeViewItem * )),
	    this, SLOT( slotStartupFinished( KFileTreeViewItem * )));


   return( m_defaultBranch );
}

void ScanPackager::slotStartupFinished( KFileTreeViewItem *it )
{
   if( m_startup && (it == m_defaultBranch->root()) )
   {
      kdDebug(28000) << "Slot population finished hit!" << endl;

      /* If nothing is selected, select the root. */
      if( ! currentKFileTreeViewItem() )
      {
	 (m_defaultBranch->root())->setSelected( true );
      }

      m_startup = false;
   }
}

void ScanPackager::slotDirCount( KFileTreeViewItem* item, int cnt )
{
   if( item && item->isDir() )
   {
      QString cc = i18n( "one item", "%n items", cnt);
      item->setText( 1, cc );
   }
   else
   {
      kdDebug(28000) << "Item is NOT directory - do not set child count!" << endl;
   }
}

void ScanPackager::slotDecorate( KFileTreeViewItem* item )
{
   if( !item ) return;
   if( item->isDir())
   {
      // done in extra slot.
      kdDebug(28000) << "Decorating directory!" << endl;
   }
   else

   {
      KFileItem *kfi = item->fileItem();

      KookaImage *img = 0L;

      if( kfi )
      {
	 img = static_cast<KookaImage*>(kfi->extraData( this ));
      }

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
	 kdDebug( 28000) << "Image loaded and decorated!" << endl;
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

   // This code is quite similar to m_nextUrlToSelect in KFileTreeView::slotNewTreeViewItems
   // When scanning a new image, we wait for the KDirLister to notice the new file,
   // and then we have the KFileTreeViewItem that we need to display the image.
   if ( ! m_nextUrlToShow.isEmpty() )
   {
       if( m_nextUrlToShow.cmp(item->url(), true ))
       {
           m_nextUrlToShow = KURL(); // do this first to prevent recursion
           slClicked( item );
       }
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
      emit fileChanged( kftvi->fileItem() );
   }
}



void ScanPackager::slFileRename( QListViewItem* it, const QString& newStr, int )
{

   bool success = true;
   if( !it ) return;

   if( newStr.isEmpty() )
      success = false;

   KFileTreeViewItem *item = static_cast<KFileTreeViewItem*>(it);

   /* Free memory and imform everybody who is interested. */
   KURL urlFrom = item->url();
   KURL urlTo( urlFrom );

   /* clean filename and apply new name */
   urlTo.setFileName("");
   urlTo.setFileName(newStr);

   if( success )
   {
      if( urlFrom == urlTo )
      {
	 kdDebug(28000) << "Renaming to same url does not make sense!" << endl;
	 success = false;
      }
      else
      {
	 /* clear selection, because the renamed image comes in through
	  * kdirlister again
	  */
	 slotUnloadItem( item );

	 kdDebug(28000) << "Renaming to " << urlTo.prettyURL() <<
	    " from " << urlFrom.prettyURL() << endl;

	 /* to urlTo the really used filename is written */
	 setSelected( item, false );

	 if( ImgSaver::renameImage( urlFrom, urlTo, false, this ) )
	 {
	    kdDebug(28000) << "renaming OK" << endl;
            emit fileRenamed( item->fileItem(), urlTo );
	    success=true;
	 }
	 else
	 {
	    success = false;
	 }
      }
   }

   if( !success )
   {
      kdDebug(28000) << "renaming failed" << endl;
      /* restore the name */
      item->setText(0, urlFrom.fileName() );
      setSelected( item, true );

   }

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

   if( newExt.isEmpty() )
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
      KMessageBox::sorry( 0L, i18n( "You entered a file extension that differs from the existing one. That is not yet possible. Converting 'on the fly' is planned for a future release.\n"
				      "Kooka corrects the extension."),
			  i18n("On the Fly Conversion"));
      ext = base + "." + currFormat;
   }
   return( ext );
}

/* ----------------------------------------------------------------------- */
/* This method returns the directory of an image or directory.
 */
QString ScanPackager::itemDirectory( const KFileTreeViewItem* item, bool relativ ) const
{
   if( ! item )
   {
      kdDebug(28000) << "ERR: itemDirectory without item" << endl;

   }

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

	    if( relativUrl.isEmpty() ) relativUrl = "/"; // The root
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
 * from the data, find out which KFileTreeViewItem is associated with it and
 * call slClicked with it.
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
         slClicked(kfi); // load thumbnails for this dir etc.
      }
   }
}

/* ----------------------------------------------------------------------- */
/* This slot is called when clicking on an item.                           */
void ScanPackager::slClicked( QListViewItem *newItem )
{
   KFileTreeViewItem *item = static_cast<KFileTreeViewItem*>(newItem);

   if( item ) // can be 0, when clicking where no item is present
   {
      kdDebug(28000) << "Clicked - newItem !" << endl;
      /* Check if directory, hide image for now, later show a thumb view */
      if( item->isDir())
      {
	 kdDebug(28000) << "clicked: Is a directory !" << endl;
	 emit( showImage( 0L ));
	 kdDebug(28000) << "emitting showThumbnails" << endl;
      }
      else
      {
	 /* if not a dir, load the image if neccessary. This is done by loadImageForItem,
	  * which is async( TODO ). The image finally arrives in slotImageArrived */
	 QApplication::setOverrideCursor(waitCursor);
	 emit( aboutToShowImage( item->url()));
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

	 if( item->isDir() )
	 {
	    emit( showThumbnails( item ));
	 }
	 else
	 {
	    emit( showThumbnails(  static_cast<KFileTreeViewItem*>(item->parent())));
	 }
      }
      else
      {
	 // kdDebug(28000) << "directory is not new: " << currSelectedDir << endl;
      }
   }
}

void ScanPackager::loadImageForItem( KFileTreeViewItem *item )
{

   if( ! item ) return;
   bool result = true;

   KFileItem *kfi = item->fileItem();
   if( ! kfi ) return;

   KookaImage *img = static_cast<KookaImage*>( kfi->extraData(this));

   if( img )
   {
      kdDebug(28000) << "Image already loaded." << endl;
      /* result is still true, image must be shown. */
   }
   else
   {
      /* The image needs to be loaded. Possibly it is a multi-page image.
       * If it is, the kookaImage has a subImageCount larger than one. We
       * create an subimage-item for every subimage, but do not yet load
       * them.
       */
      KURL url = item->url();

      img = new KookaImage( );
      if( !img || !img->loadFromUrl( url ) )
      {
	 kdDebug(28000) << "Loading KookaImage from File failed!" << endl;
	 result = false;
      }
      else
      {
          /* store the fileitem */
          img->setFileItem( kfi );

          /* care for subimages, create items for them */
          kdDebug(28000) << "subImage-count: " << img->subImagesCount() << endl;
          if( img->subImagesCount() > 1 )
          {
              KIconLoader *loader = KGlobal::iconLoader();
              kdDebug(28000) << "SubImages existing!" << endl;

              /* Start at the image with index 1, that makes  one less than are actually in the
               * image. But image 0 was already created above. */
              KFileTreeViewItem *prevItem=0;
              for( int i = 1; i < img->subImagesCount(); i++ )
              {
                  kdDebug(28000) << "Creating subimage no " << i << endl;
                  KFileItem *newKfi = new KFileItem( *kfi );
                  KFileTreeViewItem *subImgItem = new KFileTreeViewItem( item, newKfi, item->branch());

                  if( prevItem )
                  {
                      subImgItem->moveItem( prevItem );
                  }
                  prevItem = subImgItem;

                  subImgItem->setPixmap( 0, loader->loadIcon( "editcopy", KIcon::Small ));
                  subImgItem->setText( 0, i18n("Sub-image %1").arg( i ) );
                  KookaImage  *subImgImg = new KookaImage( i, img );
                  subImgImg->setFileItem( newKfi );
                  newKfi->setExtraData( (void*) this, (void*) subImgImg );
              }
          }
      }
   }


   if( result && img )
   {
      if( img->isSubImage() )
      {
	 kdDebug(28000) << "it _is_ a subimage" << endl;
	 /* load if not loaded */
	 if( img->isNull())
	 {
	    kdDebug(28000) << "extracting subimage" << endl;
	    img->extractNow();
	 }
	 else
	 {
	    kdDebug(28000) << "Is not a null image" << endl;
	 }
      }
      slImageArrived( item, img );
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

KookaImage* ScanPackager::getCurrImage() const
{
    KFileTreeViewItem *curr = currentKFileTreeViewItem();
    KookaImage *img = 0L;

    if( curr )
    {
        KFileItem *kfi = curr->fileItem();
        if( kfi )
        {
          img = static_cast<KookaImage*>(kfi->extraData( this ));
        }
    }
    return(img);
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
	 result = localFileName(curr);
      }
      else
      {
	 KURL url( localFileName(curr));
	 url = curr->url();
	 result = url.fileName();
      }
   }
   return( result );
}

/* ----------------------------------------------------------------------- */
QCString ScanPackager::getImgFormat( KFileTreeViewItem* item ) const
{

   QCString cstr;

   if( !item ) return( cstr );
#if 0
   KFileItem *kfi = item->fileItem();

   QString mime = kfi->mimetype();
#endif

   // TODO find the real extension for use with the filename !
   // temporarely:
   QString f = localFileName( item );

   return( QImage::imageFormat( f ));

}

QString ScanPackager::localFileName( KFileTreeViewItem *it ) const
{
   if( ! it ) return( QString::null );

   KURL url = it->url();

   QString res;

   if( url.isLocalFile())
   {
      res = url.directory( false, true ) + url.fileName();
   }

   return( res );
}

/* Called if the image exists but was changed by image manipulation func   */
void ScanPackager::slotCurrentImageChanged( QImage *img )
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

   const QString filename = localFileName( curr );
   const QCString format = getImgFormat( curr );
   ImgSaver saver( this );
   ImgSaveStat is_stat = ISS_OK;
   is_stat = saver.saveImage( img, filename, format );

   if( is_stat == ISS_ERR_FORMAT_NO_WRITE )
   {
      KMessageBox::error( this, i18n( "Cannot write this image format.\nImage will not be saved!"),
			    i18n("Save Error") );
   }
   else if( is_stat == ISS_ERR_PERM )
   {
      KMessageBox::error( this, i18n( "Image file is write protected.\nImage will not be saved!"),
			    i18n("Save Error") );

   }
   else if( is_stat == ISS_ERR_PROTOCOL )
   {
      KMessageBox::sorry( this, i18n( "Cannot save the image, because the file is local.\n"
				      "Kooka will support other protocols later."),
			    i18n("Save Error") );

   }
   else if( is_stat != ISS_OK )
   {
      kdDebug(28000) << "Error while saving existing image !" << endl;
   }

   if( img && !img->isNull())
   {
      emit( imageChanged( curr->fileItem()));
      KookaImage *newImage = new KookaImage(*img);
      slImageArrived( curr, newImage );
   }
}


/* ----------------------------------------------------------------------- */
/* This slot takes a new scanned Picture and saves it.
 * It urgently needs to make a deep copy of the image !
 */
void ScanPackager::slAddImage( QImage *img, KookaImageMeta* )
{
   ImgSaveStat is_stat = ISS_OK;
   /* Save the image with the help of the ImgSaver */
   if( ! img ) return;

   /* currently selected item is the directory or a file item */
   KFileTreeViewItem *curr = currentKFileTreeViewItem();

   /* Use root if nothing is selected */
   if( ! curr )
   {
      KFileTreeBranch *b = branches().at(0); /* There should be at least one */

      if( b )
      {
	 curr = findItem( b, i18n( "Incoming/" ) );
	 if( ! curr ) curr = b->root();
      }

      /* If curr is still undefined, something very tough has happend. Go away here */
      if( !curr ) return;

      setSelected( curr, true );
   }

   /* find the directory above the current one */

   KURL dir(itemDirectory( curr ));

   /* Path of curr sel item */
   ImgSaver img_saver( this, dir );

   is_stat = img_saver.saveImage( img );
   if( is_stat == ISS_ERR_FORMAT_NO_WRITE )
   {
      KMessageBox::error( this, i18n( "Cannot write this image format.\nImage will not be saved!"),
			    i18n("Save Error") );
   }
   else if( is_stat == ISS_ERR_PERM )
   {
      KMessageBox::error( this, i18n( "Image file is write protected.\nImage will not be saved!"),
			    i18n("Save Error") );

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
   KURL lurl = img_saver.lastFileUrl();

   KFileTreeBranchList branchlist = branches();
   KFileTreeBranch *kookaBranch = branchlist.at(0);

   QString strdir = itemDirectory(curr);
   if(strdir.endsWith(QString("/"))) strdir.truncate( strdir.length() - 1 );
   kdDebug(28000) << "Updating directory with " << strdir << endl;

   if( kookaBranch ) kookaBranch->updateDirectory( KURL(strdir) );
   slotSetNextUrlToSelect( lurl );
   m_nextUrlToShow = lurl;

   QString s;
   /* Count amount of children of the father */
   QListViewItem *paps = curr->parent();
   if( curr->isDir() ) /* take only father if the is no directory */
      paps = curr;

   if( paps )
   {
      int childcount = paps->childCount();
      s = i18n("%1 images").arg(childcount);
      paps->setText( 1, s);
      setOpen( paps, true );
   }

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
      slClicked( found );
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

   if( m_contextMenu )
   {
      m_contextMenu->exec( p );
   }

}

/* ----------------------------------------------------------------------- */

void ScanPackager::slotExportFile( )
{
   KFileTreeViewItem *curr = currentKFileTreeViewItem();
   if( ! curr ) return;

   if( curr->isDir() )
   {
      kdDebug(28000) << "Not yet implemented!" << endl;
   }
   else
   {
      KURL fromUrl( curr->url());
      QString filter = "*." + getImgFormat(curr).lower();
      filter += "\n*|" + i18n( "All Files" );

      // initial += fromUrl.filename(false);
      QString initial = m_currCopyDir + "/";
      initial += fromUrl.filename(false);
      KURL fileName = KFileDialog::getSaveURL ( initial,
						filter, this );

      if ( fileName.isValid() )                  // got a file name
      {
         if( fromUrl == fileName ) return;

	 /* Since it is asynchron, we will never get if it succeeded. */
	 ImgSaver::exportImage( fromUrl, fileName );

         /* remember the filename for the next export */
	 fileName.setFileName( QString());
	 m_currCopyDir = fileName.url( );
      }
   }
}

void ScanPackager::storeJob( KIO::Job *job, KFileTreeViewItem *item, JobDescription::JobType jType )
{
   JobDescription newJob ( job, item, jType );
   jobMap.insert( job, newJob );
}

void ScanPackager::slotExportFinished( KIO::Job *job )
{
   // nothing yet to do.
   kdDebug(28000) << "Export Finished" << endl;
   jobMap.remove(job);
}


void ScanPackager::slotUrlsDropped( QWidget*, QDropEvent* ev, KURL::List& urls, KURL& copyTo )
{
   if( !urls.isEmpty() )
   {
       kdDebug(28000) << "Kooka drop event. First src url=" << urls.first() << " copyTo=" << copyTo
           << " move=" << ( ev->action() == QDropEvent::Move ) << endl;

      if ( ev->action() == QDropEvent::Move )
        copyjob = KIO::move( urls, copyTo, true );
      else
        copyjob = KIO::copy( urls, copyTo, true );
      // ### this looks wrong, but since the item isn't used at all...
      KFileTreeViewItem *curr = currentKFileTreeViewItem();
      storeJob( copyjob, curr, JobDescription::ImportJob );
      connect( copyjob, SIGNAL(result(KIO::Job*)),
	       this, SLOT(slotExportFinished( KIO::Job* )));
   }
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

      /* If image is zero, ok, than there is nothing to unload :) */
      if( image )
      {
	 if( image->subImagesCount() > 0 )
	 {
	    KFileTreeViewItem *child = static_cast<KFileTreeViewItem*>(curr->firstChild());

	    while( child )
	    {
	       KFileTreeViewItem *nextChild = 0;
	       kdDebug(28000) << "Unloading subimage item " << child << endl;
	       slotUnloadItem( child );
	       nextChild = static_cast<KFileTreeViewItem*> (child->nextSibling());
	       delete child;
	       child = nextChild;
	    }
	 }

	 emit( unloadImage( image ));
	 delete image;
	 kfi->removeExtraData( this );
	 slotDecorate( curr );
      }
   }
}

/* ----------------------------------------------------------------------- */
void ScanPackager::slotDeleteItems( )
{
   KFileTreeViewItem *curr = currentKFileTreeViewItem();
   if( ! curr ) return;

   KURL urlToDel = curr->url();
   QListViewItem *nextToSelect = curr->nextSibling();

   kdDebug(28000) << "Deleting: " << urlToDel.prettyURL() << endl;
   bool ask = true; /* for later use */

   int result = KMessageBox::Yes;

   KFileItem *item = curr->fileItem();
   if( ask )
   {
      QString s;
      s = i18n("Do you really want to delete this image?\nIt cannot be restored!" );
      if( item->isDir() )
      {
	 s = i18n("Do you really want to delete the folder %1\nand all the images inside?").arg("");
      }
      result = KMessageBox::questionYesNo(this, s, i18n( "Delete Collection Item"),
					  KStdGuiItem::yes(), KStdGuiItem::no(),
					  "AskForDeleteFiles" );
   }

   /* Since we are currently talking about local files here, NetAccess is OK */
   if( result == KMessageBox::Yes )
   {
      if( KIO::NetAccess::del( urlToDel ))
      {
	 if( nextToSelect )
	    setSelected( nextToSelect, true );
	 /* TODO: remove the directory from the imageNameCombobox */
	 if( curr && item->isDir() )
	 {
	    /* The directory needs to be removed from the name combo */
	    emit(directoryToRemove( curr->branch(), itemDirectory( curr, true ) ));
	 }

      }
      else
	 kdDebug(28000) << "Deleting files failed" << endl;

   }
}

/* ----------------------------------------------------------------------- */
void ScanPackager::slotCreateFolder( )
{
   KLineEditDlg d(i18n("Please enter a name for the new folder:"), QString::null, this);
   d.setCaption(i18n("New Folder"));

   if( d.exec() )
   {
      QString folder = d.text();
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

/* called whenever one branch detects a deleted file */
void ScanPackager::slotDeleteFromBranch( KFileItem* kfi )
{
   emit fileDeleted( kfi );
}

void ScanPackager::contentsDragMoveEvent( QDragMoveEvent *e )
{
   if( !e || ! acceptDrag( e ) )
   {
      e->ignore();
      return;
   }

   QListViewItem *afterme = 0;
   QListViewItem *parent = 0;

   findDrop( e->pos(), parent, afterme );

   // "afterme" is 0 when aiming at a directory itself
   QListViewItem *item = afterme ? afterme : parent;

   if( item )
   {
      bool isDir = static_cast<KFileTreeViewItem*> (item)->isDir();
      if( isDir ) {
         KFileTreeView::contentsDragMoveEvent( e ); // for the autoopen code
         return;
      }
   }
   e->acceptAction();
}


#include "scanpackager.moc"
