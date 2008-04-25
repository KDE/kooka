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

#include <qfileinfo.h>
#include <qsignalmapper.h>
#include <qlistview.h>
#include <qdir.h>

#include <kmessagebox.h>
#include <kfiletreeview.h>
#include <ktrader.h>
#include <krun.h>
#include <kpropertiesdialog.h>
#include <kpopupmenu.h>
#include <kaction.h>
#include <kinputdialog.h>
#include <kiconloader.h>
#include <kfiledialog.h>
#include <kurl.h>
#include <kdebug.h>
#include <klocale.h>
#include <kurldrag.h>

#include <kio/netaccess.h>

#include "imgsaver.h"
#include "kookaimage.h"
#include "kookaimagemeta.h"
#include "previewer.h"

#include "scanpackager.h"
#include "scanpackager.moc"


/* ----------------------------------------------------------------------- */
/* Constructor Scan Packager */
ScanPackager::ScanPackager(QWidget *parent)
    : KFileTreeView(parent)
{
   setItemsRenameable(false);				// set later from config
   setDefaultRenameAction( QListView::Reject );

   //header()->setStretchEnabled(true,0);		// do we like this effect?

   addColumn( i18n("Image Name" ));
   setColumnAlignment( 0, AlignLeft );
   setRenameable ( 0, true );

   addColumn( i18n("Size") );
   setColumnAlignment( 1, AlignRight );
   setRenameable ( 1, false );

   addColumn( i18n("Format" ));
   setColumnAlignment( 2, AlignRight );
   setRenameable ( 2, false );

   /* Drag and Drop */
   setDragEnabled( true );
   setDropVisualizer(true);
   setAcceptDrops(true);

   connect(this,SIGNAL(dropped(KFileTreeView *,QDropEvent *,QListViewItem *,QListViewItem *)),
           SLOT(slotUrlsDropped(KFileTreeView *,QDropEvent *,QListViewItem *,QListViewItem *)));

   setRootIsDecorated( false );

   connect( this, SIGNAL( clicked( QListViewItem*)),
	    SLOT( slClicked(QListViewItem*)));

   connect( this, SIGNAL( rightButtonPressed( QListViewItem *, const QPoint &, int )),
	    SLOT( slShowContextMenu(QListViewItem *,const QPoint &)));

   connect( this, SIGNAL(itemRenamed (QListViewItem*, const QString &, int ) ), this,
	    SLOT(slFileRename(QListViewItem*,const QString&)));

   connect(this,SIGNAL(expanded(QListViewItem *)),SLOT(slotItemExpanded(QListViewItem *)));


   img_counter = 1;
   /* Set the current export dir to home */
   m_currCopyDir = QDir::home().absPath();
   m_currImportDir = m_currCopyDir;

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

   openWithMapper = NULL;
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


void ScanPackager::slotDirCount(KFileTreeViewItem* item,int cnt)
{
    if (item==NULL) return;
    if (!item->isDir()) return;

    int imgCount = 0;					// total these separately,
    int dirCount = 0;					// we want individual counts
							// for files and subfolders
    const QListViewItem *ch = item->firstChild();
    while (ch!=NULL)
    {
        const KFileTreeViewItem *ci = static_cast<const KFileTreeViewItem*>(ch);
        if (ci->isDir()) ++dirCount;
        else ++imgCount;
        ch = ch->nextSibling();
    }

    QString cc = "";
    if (dirCount==0)
    {
        if (imgCount==0) cc = i18n("empty");
        else cc = i18n("one image","%n images",imgCount);
    }
    else
    {
        if (imgCount>0) cc = i18n("one image, ","%n images, ",imgCount);
        cc += i18n("1 folder","%n folders",dirCount);
    }

    item->setText(1," "+cc);
}


void ScanPackager::slotItemExpanded(QListViewItem *item)
{
    if (item==NULL) return;

    KFileTreeViewItem *it = static_cast<KFileTreeViewItem*>(item);
    if (!it->isDir()) return;

    QListViewItem *ch = item->firstChild();
    while (ch!=NULL)
    {
        KFileTreeViewItem *ci = static_cast<KFileTreeViewItem *>(ch);
        if (ci->isDir() && !ci->alreadyListed()) ci->branch()->populate(ci->url(),ci);
        ch = ch->nextSibling();
    }
}


void ScanPackager::slotDecorate( KFileTreeViewItem* item )
{
   if( !item ) return;
   if( item->isDir())
   {
      // done in extra slot.
      //kdDebug(28000) << "Decorating directory!" << endl;
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
	 if ( kfi )
	 {
	    item->setText(1, KIO::convertSize( kfi->size() ));
	 }
      }

      /* Image format */
      QString format = getImgFormat( item );
      item->setText( 2, " "+format );
   }

   // This code is quite similar to m_nextUrlToSelect in KFileTreeView::slotNewTreeViewItems
   // When scanning a new image, we wait for the KDirLister to notice the new file,
   // and then we have the KFileTreeViewItem that we need to display the image.
   if ( ! m_nextUrlToShow.isEmpty() )
   {
       if( m_nextUrlToShow.equals(item->url(), true ))
       {
           m_nextUrlToShow = KURL(); // do this first to prevent recursion
           slClicked( item );
           setCurrentItem(item);     // neccessary in case of new file from D&D
       }
   }
}




void ScanPackager::slotDecorate( KFileTreeBranch* branch, const KFileTreeViewItemList& list )
{
   (void) branch;
   //kdDebug(28000) << "decorating slot for list !" << endl;

   KFileTreeViewItemListIterator it( list );

   bool end = false;
   for( ; !end && it.current(); ++it )
   {
      KFileTreeViewItem *kftvi = *it;
      slotDecorate( kftvi );
      emit fileChanged( kftvi->fileItem() );
   }
}


void ScanPackager::updateParent(const KFileTreeViewItem *curr)
{
    KFileTreeBranch *branch = branches().at(0);		/* There should be at least one */
    if (branch==NULL) return;

    QString strdir = itemDirectory(curr);
    // if(strdir.endsWith(QString("/"))) strdir.truncate( strdir.length() - 1 );
    kdDebug(28000) << k_funcinfo << "Updating directory " << strdir << endl;
    KURL dir = KURL::fromPathOrURL(strdir);
    branch->updateDirectory(dir);

    KFileTreeViewItem *parent = branch->findTVIByURL(dir);
    if (parent!=NULL) parent->setOpen(true);		/* Ensure parent is expanded */
}


void ScanPackager::slFileRename(QListViewItem *it,const QString &newName)
{
    if (it==NULL) return;
    KFileTreeViewItem *item = static_cast<KFileTreeViewItem *>(it);

    bool success = (!newName.isEmpty());

    KURL urlFrom = item->url();
    KURL urlTo(urlFrom);
    urlTo.setFileName("");				/* clean filename, apply new name */
    urlTo.setFileName(newName);

    if (success)
    {
        if (urlFrom==urlTo)
        {
            kdDebug(28000) << "Renaming to same!" << endl;
            success = false;
        }
        else
        {
	 /* clear selection, because the renamed image comes in through
	  * kdirlister again
	  */
            // slotUnloadItem(item);			// unnecessary, bug 68532
							// because of "note new URL" below
            kdDebug(28000) << k_funcinfo
                           << "Renaming " << urlFrom.prettyURL()
                           << " -> " << urlTo.prettyURL() << endl;

            //setSelected(item,false);

            success = ImgSaver::renameImage(urlFrom,urlTo,false,this);
            if (success)				// rename the file
            {
                item->fileItem()->setURL(urlTo);	// note new URL
                emit fileRenamed(item->fileItem(),urlTo);
            }
        }
    }

    if (!success)
    {
        kdDebug(28000) << "renaming failed" << endl;
        item->setText(0,urlFrom.fileName());		// restore the name
    }

//    setSelected(item,true);				// restore the selection
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
      return QString::null;
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
 * image gallery, the relative directory under the branch.  Now it is to assemble
 * a complete path from the data, find out which KFileTreeViewItem is associated
 * with it and call slClicked with it.
 */

void ScanPackager::slotSelectDirectory(const QString &branchName,const QString &relPath)
{
    kdDebug(28000) << k_funcinfo << "branch=" << branchName << " path=" << relPath << endl;

    KFileTreeViewItem *kfi;
    if (!branchName.isEmpty()) kfi = findItem(branchName,relPath);
    else kfi = findItem(branches().at(0),relPath);
    if (kfi==NULL) return;

    ensureItemVisible(kfi);
    setCurrentItem(kfi);
    slClicked(kfi);					// load thumbnails, etc.
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
	 emit showImage(NULL,true);
      }
      else
      {
	 /* if not a dir, load the image if necessary. This is done by loadImageForItem,
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
	 emit( galleryPathChanged( item->branch(), relativUrl ));

	 if (item->isDir())
	 {
	    emit showThumbnails(item);
	 }
	 else
	 {
	    emit showThumbnails(static_cast<KFileTreeViewItem*>(item->parent()));
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
      emit showImage(image,false);
   }
}


KookaImage* ScanPackager::getCurrImage(bool loadOnDemand)
{
    KFileTreeViewItem *curr = currentKFileTreeViewItem();
    if (curr==NULL) return (NULL);			// no current item
    if (curr->isDir()) return (NULL);			// is a directory

    KFileItem *kfi = curr->fileItem();
    if (kfi==NULL) return (NULL);
    if (kfi->extraData(this)==NULL)			// see if already loaded
    {
        if (!loadOnDemand) return (NULL);		// not loaded, and don't want to
        slClicked(curr);				// select/load this image
    }

    return (static_cast<KookaImage*>(kfi->extraData(this)));
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

void ScanPackager::addImage(const QImage *img,KookaImageMeta *meta)
{
    if (img==NULL) return;
    ImgSaveStat is_stat = ISS_OK;

    KFileTreeViewItem *curr = currentKFileTreeViewItem();
    if (curr==NULL)					// into root if nothing is selected
    {
        KFileTreeBranch *branch = branches().at(0);	// there should be at least one
        if (branch!=NULL)
        {
            curr = findItem(branch,i18n("Incoming/"));	// if user has created this????
            if (curr==NULL) curr = branch->root();
        }

        if (curr==NULL) return;				// something very odd has happened!
        setSelected(curr,true);
    }

    KURL dir = KURL::fromPathOrURL(itemDirectory(curr));
							// find out where new image will go
    ImgSaver imgsaver(this,dir);
    is_stat = imgsaver.saveImage(img);			// try to save the image
    if (is_stat!=ISS_OK)				// image saving failed
    {
        if (is_stat==ISS_SAVE_CANCELED) return;		// user cancelled, just ignore

        KMessageBox::error(this,i18n("<qt>Could not save the image<br><b>%2</b><br>%1")
                           .arg(imgsaver.errorString(is_stat))
                                .arg(imgsaver.lastURL().prettyURL()),
                           i18n("Save Error"));
        return;
    }

    /* Add the new image to the list of new images */
    KURL lurl = imgsaver.lastURL();
    slotSetNextUrlToSelect(lurl);
    m_nextUrlToShow = lurl;

    updateParent(curr);
//   QString s;
//   /* Count amount of children of the father */
//   QListViewItem *paps = curr->parent();
//   if( curr->isDir() ) /* take only father if the is no directory */
//      paps = curr;
//
//   if( paps )
//   {
//      int childcount = paps->childCount();
//      s = i18n("%1 images").arg(childcount);
//      paps->setText( 1, s);
//      setOpen( paps, true );
//   }

}

/* ----------------------------------------------------------------------- */
/* selects and opens the file with the given name. This is used to restore the
 * last displayed image by its name.
 */
void ScanPackager::slSelectImage(const KURL &name)
{
    KFileTreeViewItem *found = spFindItem(UrlSearch,name.url());
    if (found)
    {
        ensureItemVisible(found);
        setCurrentItem(found);
        slClicked(found);
    }
}


KFileTreeViewItem *ScanPackager::spFindItem(SearchType type,const QString &name,const KFileTreeBranch *branch)
{
    /* Prepare a list of branches to go through. If the parameter branch is set, search
     * only in the parameter branch. If it is zero, search all branches returned by
     * kfiletreeview.branches()
     */

    KFileTreeBranchList branchList;
    if (branch!=NULL) branchList.append(branch);
    else branchList = branches();

    KFileItem *kfi = NULL;
    KFileTreeViewItem *foundItem = NULL;
    KURL url;

    KFileTreeBranch *branchloop;			// Loop until kfi is defined
    for (KFileTreeBranchIterator it(branchList); kfi==NULL && it.current(); ++it)
    {
        branchloop = *it;
        switch (type)
        {
case Dummy:
default:    break;

case NameSearch:
	    kdDebug(28000) << k_funcinfo << "name search for " << name << endl;
	    kfi = branchloop->findByName(name);
	    break;

case UrlSearch:
            url = name;					// ensure canonical path
            url.setPath(QDir(url.path()).canonicalPath());
	    kdDebug(28000) << k_funcinfo << "URL search for " << url << endl;
	    kfi = branchloop->findByURL(url);
	    break;
        }
    }

    if (kfi!=NULL)					// found something
    {
        foundItem = static_cast<KFileTreeViewItem *>(kfi->extraData(branchloop));
        kdDebug(28000) << k_funcinfo << "Success " << foundItem->path() << endl;
    }

    return (foundItem);
}

/* ----------------------------------------------------------------------- */
void ScanPackager::slShowContextMenu(QListViewItem *lvi,const QPoint &p)
{
    KFileTreeViewItem *curr = NULL;

    if (lvi!=NULL)
    {
        curr = currentKFileTreeViewItem();
        if (curr->isDir()) setSelected(curr,true);
    }

    if (m_contextMenu!=NULL) m_contextMenu->exec(p);
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
	 ImgSaver::copyImage( fromUrl, fileName );

         /* remember the filename for the next export */
	 fileName.setFileName( QString());
	 m_currCopyDir = fileName.url( );
      }
   }
}


void ScanPackager::slotImportFile()
{
    KFileTreeViewItem *curr = currentKFileTreeViewItem();
    if( ! curr ) return;

    KURL impTarget = curr->url();

    if( ! curr->isDir() )
    {
        KFileTreeViewItem *pa = static_cast<KFileTreeViewItem*>(curr->parent());
        impTarget = pa->url();
    }
    kdDebug(28000) << "Importing to " << impTarget.url() << endl;

    KURL impUrl = KFileDialog::getImageOpenURL ( m_currImportDir, this, i18n("Import Image File to Gallery"));

    if( ! impUrl.isEmpty() )
    {
        m_currImportDir = impUrl.url();
        impTarget.addPath( impUrl.fileName());  // append the name of the sourcefile to the path
        m_nextUrlToShow = impTarget;
        ImgSaver::copyImage( impUrl, impTarget );
    }
}




//  Using this form of the drop signal so we can check whether the drop
//  is on top of a directory (in which case we want to move/copy into it).

void ScanPackager::slotUrlsDropped(KFileTreeView *me,QDropEvent *ev,
                                   QListViewItem *parent,QListViewItem *after)
{
    KFileTreeViewItem *pa = static_cast<KFileTreeViewItem*>(parent);
    KFileTreeViewItem *af = static_cast<KFileTreeViewItem*>(after);

    KURL::List urls;
    KURLDrag::decode(ev,urls);
    if (urls.empty()) return;

    //kdDebug(28000) << k_funcinfo << "parent=" << (pa==NULL?"null":pa->url()) << endl;
    //kdDebug(28000) << "  after=" << (af==NULL?"null":af->url()) << endl;
    //kdDebug(28000) << "  srcs=" << urls.count() << " first=" << urls.first() << endl;
    
    KFileTreeViewItem *onto = (af!=NULL) ? af : pa;
    if (onto==NULL) return;

    KURL dest = onto->url();
    if (!onto->isDir()) dest.setFileName(QString::null);
    dest.adjustPath(1);
    kdDebug(28000) << k_funcinfo << "resolved destination " << dest << endl;

    /* first make the last url to copy to the one to select next */
    KURL nextSel = dest;
    nextSel.addPath(urls.back().fileName(false));
    m_nextUrlToShow = nextSel;

    if (ev->action()==QDropEvent::Move) KIO::move(urls,dest,true);
    else KIO::copy(urls,dest,true);
}


void ScanPackager::slotCanceled( KIO::Job* )
{
  kdDebug(28000) << i18n("Canceled by user") << endl;
}


/* ----------------------------------------------------------------------- */
void ScanPackager::slotUnloadItems()
{
    KFileTreeViewItem *curr = currentKFileTreeViewItem();
    emit showImage(NULL,false);
    slotUnloadItem(curr);
}


void ScanPackager::slotUnloadItem(KFileTreeViewItem *curr)
{
    if (curr==NULL) return;

    if (curr->isDir())					// is a directory
    {
        KFileTreeViewItem *child = static_cast<KFileTreeViewItem*>(curr->firstChild());
        while (child!=NULL)
        {
            slotUnloadItem(child);			// recursively unload contents
            child = static_cast<KFileTreeViewItem*>(child->nextSibling());
        }
    }
    else						// is a file/image
    {
        KFileItem *kfi = curr->fileItem();
        KookaImage *image = static_cast<KookaImage*>(kfi->extraData(this));

        if (image==NULL) return;			// ok, nothing to unload

        if (image->subImagesCount()>0)			// image with subimages
        {
	    KFileTreeViewItem *child = static_cast<KFileTreeViewItem*>(curr->firstChild());
	    while (child!=NULL)				// recursively unload subimages
	    {
                KFileTreeViewItem *nextChild = NULL;
                slotUnloadItem(child);
                nextChild = static_cast<KFileTreeViewItem*>(child->nextSibling());
                delete child;
                child = nextChild;
	    }
        }

        emit unloadImage(image);
        delete image;
        kfi->removeExtraData(this);
        slotDecorate(curr);
    }
}



void ScanPackager::slotItemProperties()
{
    KFileTreeViewItem *curr = currentKFileTreeViewItem();
    if (curr==NULL) return;

    (void) new KPropertiesDialog(curr->url(),this);
}


/* ----------------------------------------------------------------------- */

void ScanPackager::slotDeleteItems()
{
    KFileTreeViewItem *curr = currentKFileTreeViewItem();
    if (curr==NULL) return;

    KURL urlToDel = curr->url();
    QListViewItem *nextToSelect = curr->nextSibling();	// select this afterwards
    bool ask = true;					// for future expansion
    bool isDir = (curr->fileItem()->isDir());		// deleting a folder

    if (ask)
    {
        QString s;
        QString dontAskKey;

        if (isDir)
        {
            s = i18n("<qt>Do you really want to permanently delete the folder<br>"
                     "<b>%1</b><br>"
                     "and all of its contents? It cannot be restored!").arg(urlToDel.pathOrURL());
            dontAskKey = "AskForDeleteDirs";
        }
        else
        {
            s = i18n("<qt>Do you really want to permanently delete the image<br>"
                     "<b>%1</b>?<br>"
                     "It cannot be restored!").arg(urlToDel.pathOrURL());
            dontAskKey = "AskForDeleteFiles";
        }

        if (KMessageBox::warningContinueCancel(this,s,i18n("Delete Gallery Item"),
                                               KStdGuiItem::del(),
                                               dontAskKey)!=KMessageBox::Continue) return;
    }

    kdDebug(28000) << k_funcinfo << "Deleting " << urlToDel.prettyURL() << endl;
    /* Since we are currently talking about local files here, NetAccess is OK */
    if (!KIO::NetAccess::del(urlToDel,NULL))
    {
        KMessageBox::error(this,i18n("<qt>Unable to delete the image or folder<br>"
                                     "<b>%2</b><br><br>"
                                     "%1").arg(KIO::NetAccess::lastErrorString(),urlToDel.pathOrURL()));
        return;
    }

    if (nextToSelect!=NULL) setSelected(nextToSelect,true);

    updateParent(curr);

    if (isDir)
    {
        /* The directory needs to be removed from the name combo */
        emit galleryDirectoryRemoved(curr->branch(),itemDirectory(curr,true));
    }
}




void ScanPackager::slotRenameItems( )
{
   KFileTreeViewItem *curr = currentKFileTreeViewItem();
   if (curr!=NULL) rename(curr,0);
}




/* ----------------------------------------------------------------------- */
void ScanPackager::slotCreateFolder( )
{
   bool ok;
   QString folder = KInputDialog::getText( i18n( "New Folder" ),
         i18n( "Name for the new folder:" ), QString::null,
         &ok, this );

   if( ok )
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

	    if( ! KIO::NetAccess::mkdir( url, 0, -1 ))
	    {
	       kdDebug(28000) << "ERR: creation of " << url.prettyURL() << " failed !" << endl;
	    }
	    else
	    {
	       /* created successfully */
	       /* open the branch if necessary and select the new folder */

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
ScanPackager::~ScanPackager()
{
    kdDebug(28000) << k_funcinfo << endl;
}

/* called whenever one branch detects a deleted file */
void ScanPackager::slotDeleteFromBranch( KFileItem* kfi )
{
   emit fileDeleted( kfi );
}

void ScanPackager::contentsDragMoveEvent( QDragMoveEvent *e )
{
   if( ! acceptDrag( e ) )
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


void ScanPackager::setAllowRename(bool on)
{
    kdDebug(28000) << k_funcinfo << "allow=" << on << endl;
    setItemsRenameable(on);
}






void ScanPackager::showOpenWithMenu(KActionMenu *menu)
{
    kdDebug(28000) << k_funcinfo << endl;

    KFileTreeViewItem *curr = currentKFileTreeViewItem();
    QString mimeType = KMimeType::findByURL(curr->url())->name();
    kdDebug(28000) << "Trying to open <" << curr->url().prettyURL()<< "> which is " << mimeType << endl;

    openWithOffers = KTrader::self()->query(mimeType,"Type=='Application'");

    if (openWithMapper==NULL)
    {
        openWithMapper = new QSignalMapper(this);
        connect(openWithMapper,SIGNAL(mapped(int)),SLOT(slotOpenWith(int)));
    }

    menu->popupMenu()->clear();

    int i = 0;
    for (KTrader::OfferList::ConstIterator it = openWithOffers.begin();
         it!=openWithOffers.end(); ++it, ++i)
    {
        KService::Ptr service = (*it);
        kdDebug(28000) << "  offer: " << (*it)->name() << endl;

        QString actionName((*it)->name().replace("&","&&"));
        KAction *act = new KAction(actionName,(*it)->pixmap(KIcon::Small),0,
                                   openWithMapper,SLOT(map()),
                                   menu->parentCollection());
        openWithMapper->setMapping(act,i);
        menu->insert(act);
    }

    menu->popupMenu()->insertSeparator();
    KAction *act = new KAction(i18n("Other..."),0,openWithMapper,SLOT(map()),
                               menu->parentCollection());
    openWithMapper->setMapping(act,i);
    menu->insert(act);
}


void ScanPackager::slotOpenWith(int idx)
{
    kdDebug(28000) << k_funcinfo << "idx=" << idx << endl;

    KFileTreeViewItem *ftvi = currentKFileTreeViewItem();
    if (ftvi==NULL) return;
    KURL::List urllist(ftvi->url());

    if (idx<((int) openWithOffers.count()))		// application from the menu
    {
        KService::Ptr ptr = openWithOffers[idx];
        KRun::run(*ptr,urllist);
    }
    else KRun::displayOpenWithDialog(urllist);		// last item = "Other..."
}
