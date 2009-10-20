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
#include "scanpackager.moc"

#include <qfileinfo.h>
#include <qsignalmapper.h>
#include <q3listview.h>
#include <qdir.h>
#include <qevent.h>
#include <qapplication.h>

#include <kmessagebox.h>
#include <k3filetreeview.h>
#include <kmimetypetrader.h>
#include <krun.h>
#include <kpropertiesdialog.h>
#include <kmenu.h>
#include <kaction.h>
#include <kactionmenu.h>
#include <kinputdialog.h>
#include <kiconloader.h>
#include <kfiledialog.h>
#include <kurl.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kstandardguiitem.h>
#include <kimageio.h>

#include <kio/global.h>
#include <kio/netaccess.h>

#include "libkscan/previewer.h"

#include "imgsaver.h"
#include "kookaimage.h"
#include "kookaimagemeta.h"


// TODO: port to KDE4, although from kdelibs/KDE4PORTING.html:
//
// "No replacement yet, apart from the more low-level KDirModel"
//
// K3FileTreeViewItem is not the same as KDE3's KFileTreeViewItem in that
// fileItem() used to return a KFileItem *, allowing the item to be modified
// through the pointer.  Now it returns a KFileItem which is a value copy of the
// internal one, not a pointer to it - so the internal KFileItem cannot
// be modified.  This means that we can't store information in the extra data
// of the KFileItem of a K3FileTreeViewItem.  Including, unfortunately, our
// KookaImage pointer :-(
//
// This is a consequence of commit 719513, "Making KFileItemList value based".
//
// We therefore maintain our own map of item URL -> item information.  There
// is an entry in this map for each loaded image.



// ------------------------------------------------------------------------

class PackagerInfo
{
public:
    PackagerInfo(KookaImage *img, K3FileTreeViewItem *it = NULL);

    // Default constructor, needed to be able to use a value container.
    // The compiler-generated copy constructor and assignment operators
    // will be OK to use.
    explicit PackagerInfo();

    bool isValid() const { return (mValid); }

    K3FileTreeViewItem *item() const { return (mItem); }
    KookaImage *image() const { return (mImage); }

private:
    bool mValid;
    KookaImage *mImage;
    K3FileTreeViewItem *mItem;
};


PackagerInfo::PackagerInfo(KookaImage *img, K3FileTreeViewItem *it)
{
    mValid = true;
    mImage = img;
    mItem = it;
}


PackagerInfo::PackagerInfo()
{
    mValid = false;
    mImage = NULL;
    mItem = NULL;
}




// ------------------------------------------------------------------------

ScanPackager::ScanPackager(QWidget *parent)
    : K3FileTreeView(parent)
{
    setItemsRenameable(false);				// set later from config
    setDefaultRenameAction(Q3ListView::Reject);

   //header()->setStretchEnabled(true,0);		// do we like this effect?

   addColumn( i18n("Image Name" ));
   setColumnAlignment( 0, Qt::AlignLeft );
   setRenameable ( 0, true );

   addColumn( i18n("Size") );
   setColumnAlignment( 1, Qt::AlignRight );
   setRenameable ( 1, false );

   addColumn( i18n("Format" ));
   setColumnAlignment( 2, Qt::AlignRight );
   setRenameable ( 2, false );

   /* Drag and Drop */
   setDragEnabled( true );
   setDropVisualizer(true);
   setAcceptDrops(true);

   connect(this,SIGNAL(dropped(K3FileTreeView *,QDropEvent *,Q3ListViewItem *,Q3ListViewItem *)),
           SLOT(slotUrlsDropped(K3FileTreeView *,QDropEvent *,Q3ListViewItem *,Q3ListViewItem *)));

   setRootIsDecorated( false );

   connect( this, SIGNAL( clicked( Q3ListViewItem*)),
	    SLOT( slotClicked(Q3ListViewItem*)));

   connect( this, SIGNAL( rightButtonPressed( Q3ListViewItem *, const QPoint &, int )),
	    SLOT( slotShowContextMenu(Q3ListViewItem *,const QPoint &)));

   connect( this, SIGNAL(itemRenamed (Q3ListViewItem*, const QString &, int ) ), this,
	    SLOT(slotFileRename(Q3ListViewItem*,const QString&)));

   connect(this,SIGNAL(expanded(Q3ListViewItem *)),SLOT(slotItemExpanded(Q3ListViewItem *)));


   img_counter = 1;

    /* Preload frequently used icons */
    KIconLoader *loader = KIconLoader::global();
    mPixFloppy = KIcon("media-floppy").pixmap(KIconLoader::SizeSmall);
    mPixGray   = KIcon("palette-gray").pixmap(KIconLoader::SizeSmall);
    mPixBw     = KIcon("palette-lineart").pixmap(KIconLoader::SizeSmall);
    mPixColor  = KIcon("palette-color").pixmap(KIconLoader::SizeSmall);

   m_startup = true;

   /* create a context menu and set the title */
   m_contextMenu = new KMenu();
   m_contextMenu->addTitle(i18n("Gallery"));

   openWithMapper = NULL;
//   openWithActions = NULL;
}


ScanPackager::~ScanPackager()
{
    kDebug();
    mInfoMap.clear();
}




const PackagerInfo ScanPackager::infoForUrl(const KUrl &url)
{
    if (!mInfoMap.contains(url))
    {
        kDebug() << "no info in map for" << url;
        return (PackagerInfo());			// a null/invalid PackagerInfo
    }

    return (mInfoMap[url]);
}



KookaImage *ScanPackager::imageForItem(const K3FileTreeViewItem *item)
{
    if (item==NULL) return (NULL);

    KookaImage *img = NULL;
    KUrl u = item->url();
    if (!mInfoMap.contains(u))
    {
        kDebug() << "no info in map for" << u;
        return (NULL);
    }

    img = mInfoMap[u].image();
    kDebug() << "got image" << img << "from map for for" << u;
    return (img);
}




void ScanPackager::openRoots()
{
   /* standard root always exists, ImgRoot creates it */
   KUrl rootUrl(Previewer::galleryRoot());
   kDebug() << "Open standard root " << rootUrl.url();

   openRoot( rootUrl, true );
   m_defaultBranch->setOpen(true);

   /* open more configurable image repositories TODO */
}


KFileTreeBranch *ScanPackager::openRoot(const KUrl &root, bool open)
{
    KIconLoader *loader = KIconLoader::global();

   /* working on the global branch. FIXME */
   m_defaultBranch = addBranch( root, i18n("Kooka Gallery"),
				loader->loadIcon( "folder", KIconLoader::Small ),
				false /* do not showHidden */ );

   // Q_CHECK_PTR( m_defaultBranch );
   m_defaultBranch->setOpenPixmap( loader->loadIcon( "folder-image", KIconLoader::Small ));

   setDirOnlyMode( m_defaultBranch, false );
   m_defaultBranch->setShowExtensions( true ); // false );

   connect( m_defaultBranch, SIGNAL( newTreeViewItems( KFileTreeBranch*, const K3FileTreeViewItemList& )),
	    this, SLOT( slotDecorate(KFileTreeBranch*, const K3FileTreeViewItemList& )));

   connect( m_defaultBranch, SIGNAL( directoryChildCount( K3FileTreeViewItem* , int )),
	    this, SLOT( slotDirCount( K3FileTreeViewItem *, int )));

   connect( m_defaultBranch, SIGNAL( deleteItem( KFileItem* )),
	    this, SLOT( slotDeleteFromBranch(KFileItem*)));

   connect( m_defaultBranch, SIGNAL( populateFinished( K3FileTreeViewItem * )),
	    this, SLOT( slotStartupFinished( K3FileTreeViewItem * )));


   return( m_defaultBranch );
}

void ScanPackager::slotStartupFinished( K3FileTreeViewItem *it )
{
    if (m_startup && (it==m_defaultBranch->root()))
    {
        kDebug();

        /* If nothing is selected, select the root. */
        if (currentKFileTreeViewItem()==NULL)
        {
            m_defaultBranch->root()->setSelected(true);
        }

        m_startup = false;
    }
}


void ScanPackager::slotDirCount(K3FileTreeViewItem* item,int cnt)
{
    if (item==NULL) return;
    if (!item->isDir()) return;

    int imgCount = 0;					// total these separately,
    int dirCount = 0;					// we want individual counts
							// for files and subfolders
    const Q3ListViewItem *ch = item->firstChild();
    while (ch!=NULL)
    {
        const K3FileTreeViewItem *ci = static_cast<const K3FileTreeViewItem*>(ch);
        if (ci->isDir()) ++dirCount;
        else ++imgCount;
        ch = ch->nextSibling();
    }

    QString cc = "";
    if (dirCount==0)
    {
        if (imgCount==0) cc = i18n("empty");
        else cc = i18np("one image","%1 images",imgCount);
    }
    else
    {
        if (imgCount>0) cc = i18np("one image, ","%1 images, ",imgCount);
        cc += i18np("1 folder","%1 folders",dirCount);
    }

    item->setText(1," "+cc);
}


void ScanPackager::slotItemExpanded(Q3ListViewItem *item)
{
    if (item==NULL) return;

    K3FileTreeViewItem *it = static_cast<K3FileTreeViewItem*>(item);
    if (!it->isDir()) return;

    Q3ListViewItem *ch = item->firstChild();
    while (ch!=NULL)
    {
        K3FileTreeViewItem *ci = static_cast<K3FileTreeViewItem *>(ch);
        if (ci->isDir() && !ci->alreadyListed()) ci->branch()->populate(ci->url(),ci);
        ch = ch->nextSibling();
    }
}


void ScanPackager::slotDecorate(K3FileTreeViewItem *item)
{
    if (item==NULL) return;

    if (!item->isDir())					// dir is done in another slot
    {
        KFileItem kfi = item->fileItem();

        QString format = getImgFormat(item);		// this is safe for any file
        item->setText(2,(" "+format.toUpper()+" "));

        const KookaImage *img = imageForItem(item);
        if (img!=NULL)					// image appears to be loaded
        {						// set image depth pixmap
            if (img->depth()==1) item->setPixmap(0, mPixBw);
            else
            {
                if (img->isGrayscale()) item->setPixmap(0, mPixGray);
                else item->setPixmap(0, mPixColor);
            }
							// set image size column
            QString t = i18n("%1 x %2", img->width(), img->height());
            item->setText(1,t);
        }
        else						// not yet loaded, show file info
        {
            if (!format.isEmpty())			// if a valid image file
            {
                item->setPixmap(0, mPixFloppy);
                if (!kfi.isNull()) item->setText(1, KIO::convertSize(kfi.size()));
            }
            else
            {
                item->setPixmap(0, KIO::pixmapForUrl(item->url(), 0, KIconLoader::Small));
            }
        }
    }

    // This code is quite similar to m_nextUrlToSelect in K3FileTreeView::slotNewTreeViewItems
    // When scanning a new image, we wait for the KDirLister to notice the new file,
    // and then we have the K3FileTreeViewItem that we need to display the image.
    if (!m_nextUrlToShow.isEmpty())
    {
        if (m_nextUrlToShow.equals(item->url(), KUrl::CompareWithoutTrailingSlash))
        {
            m_nextUrlToShow = KUrl();			// do this first to prevent recursion
            slotClicked(item);
            setCurrentItem(item);			// neccessary in case of new file from D&D
        }
    }
}


void ScanPackager::slotDecorate(KFileTreeBranch *branch, const K3FileTreeViewItemList &list)
{
    K3FileTreeViewItemListIterator it(list);

    while (it.current())
    {
        K3FileTreeViewItem *kftvi = *it;
        slotDecorate(kftvi);
        emit fileChanged(&kftvi->fileItem());
        ++it;
    }
}


void ScanPackager::updateParent(const K3FileTreeViewItem *curr)
{
    KFileTreeBranch *branch = branches().at(0);		/* There should be at least one */
    if (branch==NULL) return;

    QString strdir = itemDirectory(curr);
    // if(strdir.endsWith(QString("/"))) strdir.truncate( strdir.length() - 1 );
    kDebug() << "Updating directory " << strdir;
    KUrl dir(strdir);
    branch->updateDirectory(dir);

    K3FileTreeViewItem *parent = branch->findTVIByUrl(dir);
    if (parent!=NULL) parent->setOpen(true);		/* Ensure parent is expanded */
}


// TODO: this modifies the KFileItem, see comment at top
void ScanPackager::slotFileRename(Q3ListViewItem *it,const QString &newName)
{
    if (it==NULL) return;
    K3FileTreeViewItem *item = static_cast<K3FileTreeViewItem *>(it);

    bool success = (!newName.isEmpty());

    KUrl urlFrom = item->url();
    KUrl urlTo(urlFrom);
//    urlTo.setFileName("");				/* clean filename, apply new name */
    urlTo.setFileName(newName);

    if (success)
    {
        if (urlFrom==urlTo)
        {
            kDebug() << "Renaming to same!";
            success = false;
        }
        else
        {
	 /* clear selection, because the renamed image comes in through
	  * kdirlister again
	  */
            // slotUnloadItem(item);			// unnecessary, bug 68532
							// because of "note new URL" below
            kDebug() << "Renaming " << urlFrom.prettyUrl()
                           << " -> " << urlTo.prettyUrl();

            //setSelected(item,false);

            success = ImgSaver::renameImage(urlFrom,urlTo,false,this);
            if (success)				// rename the file
            {
                item->fileItem().setUrl(urlTo);	// note new URL
                //emit fileRenamed(item->fileItem(),urlTo);
                emit fileRenamed(item,urlTo.fileName());
            }
        }
    }

    if (!success)
    {
        kDebug() << "renaming failed";
        item->setText(0,urlFrom.fileName());		// restore the name
    }

//    setSelected(item,true);				// restore the selection
}


/* ----------------------------------------------------------------------- */
/*
 * Method that checks if the new filename a user enters while renaming an image is valid.
 * It checks for a proper extension.
 */
QString ScanPackager::buildNewFilename(const QString &cmplFilename, const QString &currFormat ) const
{
   /* cmplFilename = new name the user wishes.
    * currFormat   = the current format of the image.
    * if the new filename has a valid extension, which is the same as the
    * format of the current, fine. A ''-String has to be returned.
    */
   QFileInfo fiNew( cmplFilename );
   QString base = fiNew.baseName();
   QString newExt = fiNew.suffix().toLower();
   QString nowExt = KookaImage::extensionForFormat(currFormat);
   QString ext = "";

   kDebug() << "Filename wanted:"<< cmplFilename << "ext" << nowExt << "->" << newExt;

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
QString ScanPackager::itemDirectory(const K3FileTreeViewItem* item, bool relativ) const
{
    if (item==NULL)
    {
        kDebug() << "Error: without item";
        return (QString::null);
    }

    // TODO: can manupilate using KUrl?
    QString relativUrl= (item->url()).prettyUrl();

   if( ! item->isDir() )
   {
      // Cut off the filename in case it is not a dir
      relativUrl.truncate( relativUrl.lastIndexOf( '/' )+1);
   }
   else
   {
      /* add a "/" to the directory if not there */
      if( ! relativUrl.endsWith( "/" ) )
	 relativUrl.append( "/" );
   }



   if (relativ)
   {
      KFileTreeBranch *branch = item->branch();
      if (branch!=NULL)
      {
	 kDebug() << "Relative URL:" << relativUrl;
	 QString rootUrl = (branch->rootUrl()).prettyUrl();  // directory of branch root

	 if( relativUrl.startsWith( rootUrl ))
	 {
	    relativUrl.remove( 0, rootUrl.length() );

	    if( relativUrl.isEmpty() ) relativUrl = "/"; // The root
	 }
	 else
	 {
	    kDebug() << "Error: Item URL does not start with root URL" << rootUrl;
	 }
      }
   }
   return (relativUrl);
}


/* ----------------------------------------------------------------------- */
/* This slot receives a string from the gallery-path combobox shown under the
 * image gallery, the relative directory under the branch.  Now it is to assemble
 * a complete path from the data, find out which K3FileTreeViewItem is associated
 * with it and call slotClicked with it.
 */

void ScanPackager::slotSelectDirectory(const QString &branchName,const QString &relPath)
{
    kDebug() << "branch" << branchName << "path" << relPath;

    K3FileTreeViewItem *kfi;
    if (!branchName.isEmpty()) kfi = findItem(branchName,relPath);
    else kfi = findItem(branches().at(0),relPath);
    if (kfi==NULL) return;

    ensureItemVisible(kfi);
    setCurrentItem(kfi);
    slotClicked(kfi);					// load thumbnails, etc.
}


/* ----------------------------------------------------------------------- */
/* This slot is called when clicking on an item.                           */
void ScanPackager::slotClicked(Q3ListViewItem *newItem)
{
    K3FileTreeViewItem *item = static_cast<K3FileTreeViewItem*>(newItem);

    if (item==NULL) return;				// can click over no item

    kDebug();
    emit showItem(item);				// tell the (new) thumbnail view

    //  Check if directory, hide image for now, later show a thumb view?
    if (item->isDir())					// is it a directory?
    {
	 emit showImage(NULL,true);			// unload current image
    }
    else						// not a directory
    {
        //  Load the image if necessary. This is done by loadImageForItem,
        //  which is async (TODO). The image finally arrives in slotImageArrived.
        QApplication::setOverrideCursor(Qt::WaitCursor);
        emit aboutToShowImage(item->url());
        loadImageForItem(item);
        QApplication::restoreOverrideCursor();
    }

    //  Indicate the new directory if it has changed
    QString wholeDir = itemDirectory(item,false);	// not relative to root
    if (currSelectedDir!=wholeDir)
    {
        currSelectedDir = wholeDir;
        QString relativUrl = itemDirectory(item,true);
        kDebug() << "Emitting" << relativUrl << "as new relative URL";
        //  Emit the signal with branch and the relative path
        emit galleryPathChanged(item->branch(),relativUrl);

        // Show new thumbnail directory
        if (item->isDir()) emit showThumbnails(item);
        else emit showThumbnails(static_cast<K3FileTreeViewItem *>(item->parent()));
    }
}


// TODO: this tries to modify the KFileItem, see comment at top
// needs to be adapted for new URL->info map, if we do actually support subimages -
// maybe create a URL for the subimage with its fragment ID being the subimage number,
// and put that into the map as usual.
void ScanPackager::loadImageForItem(K3FileTreeViewItem *item)
{
    if (item==NULL) return;

    KFileItem kfi = item->fileItem();
    if (kfi.isNull()) return;

    kDebug() << "loading" << item->url();

    QString format = getImgFormat(item);		// check for valid image format
    if (format.isEmpty())
    {
        kDebug() << "not a valid image format!";
        return;
    }

    KookaImage *img = imageForItem(item);
    if (img==NULL)					// image not already loaded
    {
        // The image needs to be loaded. Possibly it is a multi-page image.
        // If it is, the kookaImage has a subImageCount larger than one. We
        // create an subimage-item for every subimage, but do not yet load
        // them.

        img = new KookaImage();
        if (img==NULL || !img->loadFromUrl(item->url()))
        {
            kDebug() << "loading failed!";
            return;
        }

        img->setFileItem(&kfi);				// store the fileitem

        kDebug() << "subImage-count" << img->subImagesCount();
        if (img->subImagesCount()>1)			// look for subimages,
        {						// create items for them
            KIconLoader *loader = KIconLoader::global();

            // Start at the image with index 1, that makes one less than
            // are actually in the image. But image 0 was already created above.
            K3FileTreeViewItem *prevItem = NULL;
            for (int i = 1; i<img->subImagesCount(); i++)
            {
                kDebug() << "Creating subimage" << i;
                KFileItem newKfi(kfi);
                K3FileTreeViewItem *subImgItem = new K3FileTreeViewItem(item,newKfi,item->branch());

                if (prevItem!=NULL) subImgItem->moveItem(prevItem);
                prevItem = subImgItem;

                subImgItem->setPixmap(0, loader->loadIcon("editcopy", KIconLoader::Small));
                subImgItem->setText(0, i18n("Sub-image %1", i));
                KookaImage *subImgImg = new KookaImage(i, img);
                subImgImg->setFileItem(&newKfi);
                newKfi.setExtraData(this, subImgImg);
            }
        }
    }

    if (img->isSubImage())				// this is a subimage
    {
        kDebug() << "it is a subimage";
        if (img->isNull())				// if not already loaded,
        {
            kDebug() << "extracting subimage";
            img->extractNow();				// load it now
        }
    }

    slotImageArrived(item, img);
}


/* Hit this slot with a file for a kfiletreeviewitem. */
void ScanPackager::slotImageArrived(K3FileTreeViewItem *item, KookaImage *image)
{
    if (item==NULL || image==NULL) return;

    PackagerInfo pi(image, item);
    kDebug() << "saving image" << image << "in map for" << item->url();
    mInfoMap[item->url()] = pi;

    slotDecorate(item);
    emit showImage(image, false);
}


const KookaImage *ScanPackager::getCurrImage(bool loadOnDemand)
{
    K3FileTreeViewItem *curr = currentKFileTreeViewItem();
    if (curr==NULL) return (NULL);			// no current item
    if (curr->isDir()) return (NULL);			// is a directory

    KookaImage *img = imageForItem(curr);		// see if already loaded
    if (img==NULL)					// no, try to do that
    {
        if (!loadOnDemand) return (NULL);		// not loaded, and don't want to
        slotClicked(curr);				// select/load this image
        img = imageForItem(curr);			// and get image for it
    }

    return (img);
}


QString ScanPackager::getCurrImageFileName( bool withPath = true ) const
{
   QString result = "";

   K3FileTreeViewItem *curr = currentKFileTreeViewItem();
   if( ! curr )
   {
      kDebug() << "nothing selected!";
   }
   else
   {
      if( withPath )
      {
	 result = localFileName(curr);
      }
      else
      {
	 KUrl url( localFileName(curr));
	 url = curr->url();
	 result = url.fileName();
      }
   }
   return( result );
}


QByteArray ScanPackager::getImgFormat(const K3FileTreeViewItem *item)
{
    if (item==NULL) return ("");

    KFileItem kfi = item->fileItem();
    if (kfi.isNull()) return ("");

    // Check that this is a plausible image format (MIME type = "image/anything")
    // before trying to get the image type.
    QString mimetype = kfi.mimetype();
    if (!mimetype.startsWith("image/")) return ("");

    return (KookaImage::formatForUrl(kfi.url()).toLocal8Bit());
}


QString ScanPackager::localFileName(const K3FileTreeViewItem *item)
{
    if (item==NULL) return (QString::null);

    bool isLocal = false;
    KUrl u = item->fileItem().mostLocalUrl(isLocal);
    if (!isLocal) return (QString::null);

    return (u.path());
}


/* Called if the image exists but was changed by image manipulation func   */
void ScanPackager::slotCurrentImageChanged(const QImage *img)
{
    K3FileTreeViewItem *curr = currentKFileTreeViewItem();
    if (curr==NULL)
    {
        kDebug() << "nothing selected!";
        return;
    }

    /* Not applicable to directories */
    if (curr->isDir()) return;

    /* unload image and free memory */
    slotUnloadItem(curr);

    const QString filename = localFileName(curr);
    const QByteArray format = getImgFormat(curr);
    if (format.isEmpty())				// not an image, should never happen
    {
        kDebug() << "not a valid image format!";
        return;
    }

    ImgSaver saver(this);
    ImgSaveStat is_stat = ISS_OK;
    is_stat = saver.saveImage(img, filename, format);
    if (is_stat!=ISS_OK)
    {
        KMessageBox::sorry(this, i18n("The updated image could not be saved.\n%1.",
                                      saver.errorString(is_stat)),
                           i18n("Save Error"));
    }

    if (img!=NULL && !img->isNull())
    {
        KFileItem kfi = curr->fileItem();
        emit imageChanged(&kfi);
        KookaImage *newImage = new KookaImage(*img);
        slotImageArrived(curr, newImage);
    }
}


/* ----------------------------------------------------------------------- */
/* This slot takes a new scanned Picture and saves it.
 * It urgently needs to make a deep copy of the image !
 */

void ScanPackager::addImage(const QImage *img, KookaImageMeta *meta)
{
    if (img==NULL) return;
    ImgSaveStat is_stat = ISS_OK;

    K3FileTreeViewItem *curr = currentKFileTreeViewItem();
    if (curr==NULL)					// into root if nothing is selected
    {
        KFileTreeBranch *branch = branches().at(0);	// there should be at least one
        if (branch!=NULL)
        {
            curr = findItem(branch, i18n("Incoming/"));	// if user has created this????
            if (curr==NULL) curr = branch->root();
        }

        if (curr==NULL) return;				// something very odd has happened!
        setSelected(curr,true);
    }

    KUrl dir(itemDirectory(curr));			// where new image will go
    ImgSaver imgsaver(this, dir);
    is_stat = imgsaver.saveImage(img);			// try to save the image
    if (is_stat!=ISS_OK)				// image saving failed
    {
        if (is_stat==ISS_SAVE_CANCELED) return;		// user cancelled, just ignore

        KMessageBox::error(this, i18n("<qt>Could not save the image<br><filename>%2</filename><br><br>%1",
                                      imgsaver.errorString(is_stat),
                                      imgsaver.lastURL().prettyUrl()),
                           i18n("Image Save Error"));
        return;
    }

    /* Add the new image to the list of new images */
    KUrl lurl = imgsaver.lastURL();
    slotSetNextUrlToSelect(lurl);
    m_nextUrlToShow = lurl;

    updateParent(curr);
}

/* ----------------------------------------------------------------------- */
/* selects and opens the file with the given name. This is used to restore the
 * last displayed image by its name.
 */
void ScanPackager::slotSelectImage(const KUrl &name)
{
    K3FileTreeViewItem *found = spFindItem(UrlSearch,name.url());
    if (found==NULL) found = m_defaultBranch->root();

    ensureItemVisible(found);
    setCurrentItem(found);
    slotClicked(found);
}


K3FileTreeViewItem *ScanPackager::spFindItem(SearchType type, const QString &name, const KFileTreeBranch *branch)
{
    /* Prepare a list of branches to go through. If the parameter branch is set, search
     * only in the parameter branch. If it is zero, search all branches returned by
     * kfiletreeview.branches()
     */

    KFileTreeBranchList branchList;
    if (branch!=NULL) branchList.append(branch);
    else branchList = branches();

    KFileItem kfi;					// a null KFileItem
    K3FileTreeViewItem *foundItem = NULL;
    KUrl url;

    KFileTreeBranch *branchloop;			// Loop until kfi is defined
    for (KFileTreeBranchList::const_iterator it = branchList.constBegin();
         it!=branchList.constEnd() && kfi.isNull(); ++it)
    {
        branchloop = (*it);
        switch (type)
        {
case Dummy:
default:    break;

case NameSearch:
	    kDebug() << "name search for" << name;
	    kfi = branchloop->findByName(name);
	    break;

case UrlSearch:
            url = name;					// ensure canonical path
            url.setPath(QDir(url.path()).canonicalPath());
	    kDebug() << "URL search for" << url;
	    kfi = branchloop->findByUrl(url);
	    break;
        }
    }

    if (!kfi.isNull())					// found something
    {
        kDebug() << "found kfi for" << kfi.url();
        PackagerInfo pi = infoForUrl(kfi.url());
        if (!pi.isValid())
        {
            kDebug() << "could not get extra data!"; 
        }
        else
        {
            foundItem = pi.item();
            kDebug() << "found path" << foundItem->path();
        }
    }

    return (foundItem);
}


/* ----------------------------------------------------------------------- */
void ScanPackager::slotShowContextMenu(Q3ListViewItem *lvi,const QPoint &p)
{
    K3FileTreeViewItem *curr = NULL;

    if (lvi!=NULL)
    {
        curr = currentKFileTreeViewItem();
        if (curr->isDir()) setSelected(curr,true);
    }

    if (m_contextMenu!=NULL) m_contextMenu->exec(p);
}


void ScanPackager::slotExportFile()
{
    K3FileTreeViewItem *curr = currentKFileTreeViewItem();
    if (curr==NULL) return;

    if (curr->isDir())
    {
        kDebug() << "Not yet implemented!";
        return;
    }

    KUrl fromUrl(curr->url());

    QString filter;
    QByteArray format = getImgFormat(curr);
    if (!format.isEmpty()) filter = "*."+KookaImage::extensionForFormat(format)+"\n";
    filter += "*|"+i18n("All Files");

    QString initial = "kfiledialog:///exportImage/"+fromUrl.fileName();
    KUrl fileName = KFileDialog::getSaveUrl(KUrl(initial), filter, this);
    if (!fileName.isValid()) return;			// didn't get a file name
    if (fromUrl==fileName) return;			// can't save over myself

    /* Since it is asynchron, we will never know if it succeeded. */
    ImgSaver::copyImage(fromUrl,fileName);
}


void ScanPackager::slotImportFile()
{
    K3FileTreeViewItem *curr = currentKFileTreeViewItem();
    if( ! curr ) return;

    KUrl impTarget = curr->url();

    if( ! curr->isDir() )
    {
        K3FileTreeViewItem *pa = static_cast<K3FileTreeViewItem*>(curr->parent());
        impTarget = pa->url();
    }
    kDebug() << "Importing to" << impTarget;

    KUrl impUrl = KFileDialog::getImageOpenUrl(KUrl("kfiledialog:///importImage"), this, i18n("Import Image File to Gallery"));
    if (impUrl.isEmpty()) return;

    impTarget.addPath(impUrl.fileName());		// append the name of the sourcefile to the path
    m_nextUrlToShow = impTarget;
    ImgSaver::copyImage(impUrl, impTarget);
}




//  Using this form of the drop signal so we can check whether the drop
//  is on top of a directory (in which case we want to move/copy into it).

void ScanPackager::slotUrlsDropped(K3FileTreeView *me, QDropEvent *ev,
                                   Q3ListViewItem *parent, Q3ListViewItem *after)
{
    K3FileTreeViewItem *pa = static_cast<K3FileTreeViewItem *>(parent);
    K3FileTreeViewItem *af = static_cast<K3FileTreeViewItem *>(after);

    KUrl::List urls = KUrl::List::fromMimeData(ev->mimeData());
    if (urls.isEmpty()) return;

    //kDebug() << "parent" << (pa==NULL ? "NULL" : pa->url())
    //         << "after" << (af==NULL ? "NULL" : af->url())
    //         << "srcs" << urls.count() << "first" << urls.first();
    
    K3FileTreeViewItem *onto = (af!=NULL) ? af : pa;
    if (onto==NULL) return;

    KUrl dest = onto->url();
    if (!onto->isDir()) dest.setFileName(QString::null);
    dest.adjustPath(KUrl::AddTrailingSlash);
    kDebug() << "resolved destination" << dest;

    /* first make the last url to copy to the one to select next */
    KUrl nextSel = dest;
    nextSel.addPath(urls.back().fileName(false));
    m_nextUrlToShow = nextSel;

    // TODO: top level window as 3rd parameter?
    if (ev->dropAction()==Qt::MoveAction) KIO::NetAccess::move(urls, dest, this);
    else KIO::NetAccess::dircopy(urls, dest, this);
}


void ScanPackager::slotCanceled( KIO::Job* )
{
    kDebug();
}


/* ----------------------------------------------------------------------- */
void ScanPackager::slotUnloadItems()
{
    K3FileTreeViewItem *curr = currentKFileTreeViewItem();
    emit showImage(NULL,false);
    slotUnloadItem(curr);
}


void ScanPackager::slotUnloadItem(K3FileTreeViewItem *curr)
{
    if (curr==NULL) return;

    if (curr->isDir())					// is a directory
    {
        K3FileTreeViewItem *child = static_cast<K3FileTreeViewItem*>(curr->firstChild());
        while (child!=NULL)
        {
            slotUnloadItem(child);			// recursively unload contents
            child = static_cast<K3FileTreeViewItem*>(child->nextSibling());
        }
    }
    else						// is a file/image
    {
        KFileItem kfi = curr->fileItem();
        const KookaImage *image = imageForItem(curr);
        if (image==NULL) return;			// ok, nothing to unload

        if (image->subImagesCount()>0)			// image with subimages
        {
	    K3FileTreeViewItem *child = static_cast<K3FileTreeViewItem*>(curr->firstChild());
	    while (child!=NULL)				// recursively unload subimages
	    {
                K3FileTreeViewItem *nextChild = NULL;
                slotUnloadItem(child);
                nextChild = static_cast<K3FileTreeViewItem*>(child->nextSibling());
                delete child;
                child = nextChild;
	    }
        }

        emit unloadImage(image);
        delete image;

        mInfoMap.remove(curr->url());
        kDebug() << "removed from map" << curr->url();
        slotDecorate(curr);
    }
}



void ScanPackager::slotItemProperties()
{
    K3FileTreeViewItem *curr = currentKFileTreeViewItem();
    if (curr==NULL) return;

    (void) new KPropertiesDialog(curr->url(),this);
}


/* ----------------------------------------------------------------------- */

void ScanPackager::slotDeleteItems()
{
    K3FileTreeViewItem *curr = currentKFileTreeViewItem();
    if (curr==NULL) return;

    KUrl urlToDel = curr->url();			// item to be deleted
    Q3ListViewItem *nextToSelect = curr->nextSibling();	// select this afterwards
    bool isDir = (curr->fileItem().isDir());		// deleting a folder?

    QString s;
    QString dontAskKey;
    if (isDir)
    {
        s = i18n("<qt>Do you really want to permanently delete the folder<br>"
                 "<filename>%1</filename><br>"
                 "and all of its contents? It cannot be restored!", urlToDel.pathOrUrl());
        dontAskKey = "AskForDeleteDirs";
    }
    else
    {
        s = i18n("<qt>Do you really want to permanently delete the image<br>"
                 "<filename>%1</filename>?<br>"
                 "It cannot be restored!", urlToDel.pathOrUrl());
        dontAskKey = "AskForDeleteFiles";
    }

    if (KMessageBox::warningContinueCancel(this, s, 
                                           i18n("Delete Gallery Item"),
                                           KStandardGuiItem::del(),
                                           KStandardGuiItem::cancel(),
                                           dontAskKey)!=KMessageBox::Continue) return;

    slotUnloadItem(curr);
    kDebug() << "Deleting" << urlToDel;
    /* Since we are currently talking about local files here, NetAccess is OK */
    if (!KIO::NetAccess::del(urlToDel,NULL))
    {
        KMessageBox::error(this, i18n("<qt>Could not delete the image or folder<br><filename>%2</filename><br><br>%1",
                                      KIO::NetAccess::lastErrorString(),
                                      urlToDel.prettyUrl()),
                           i18n("File Delete Error"));
        return;
    }

    updateParent(curr);					// update parent folder count
    if (isDir)						// remove from the name combo
    {
        emit galleryDirectoryRemoved(curr->branch(),itemDirectory(curr,true));
    }

#if 0
    if (nextToSelect!=NULL) setSelected(nextToSelect,true);
    //  TODO: if doing the above, also need to signal to update thumbnail
    //  as below.
    //
    //  But doing that leads to inconsistency between deleting the last item
    //  in a folder (nothing is selected afterwards) and deleting anything
    //  else (the next image is selected and loaded).  So leaving this
    //  commented out for now.
    curr = currentKFileTreeViewItem();
    kDebug() << "new selection after delete" << (curr==NULL ? "NULL" : curr->url().prettyURL());
    if (curr!=NULL) emit showItem(curr);
#endif
}




void ScanPackager::slotRenameItems( )
{
   K3FileTreeViewItem *curr = currentKFileTreeViewItem();
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

	 K3FileTreeViewItem *it = currentKFileTreeViewItem();
	 if( it )
	 {
	    KUrl url = it->url();

	    /* If a directory is selected, the filename needs not to be deleted */
	    if( ! it->isDir())
	       url.setFileName( "" );
	    /* add the folder name from user input */
	    url.addPath( folder );
	    kDebug() << "Creating folder" << url;

	    /* Since the new directory arrives in the packager in the newItems-slot, we set a
	     * variable urlToSelectOnArrive here. The newItems-slot will honor it and select
	     * the treeviewitem with that url.
	     */
	    slotSetNextUrlToSelect( url );

	    if( ! KIO::NetAccess::mkdir( url, 0, -1 ))
	    {
	       kDebug() << "Error: creation of" << url << "failed!";
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

   s = i18n("image %1", img_counter++);
   return( s );
}


/* called whenever one branch detects a deleted file */
void ScanPackager::slotDeleteFromBranch( KFileItem* kfi )
{
   emit fileDeleted( kfi );
}


void ScanPackager::contentsDragMoveEvent(QDragMoveEvent *ev)
{
    if (!acceptDrag(ev))
    {
        ev->ignore();
        return;
    }

    Q3ListViewItem *afterme = NULL;
    Q3ListViewItem *parent = NULL;

    findDrop(ev->pos(), parent, afterme);

    // "afterme" is NULL when aiming at a directory itself
    Q3ListViewItem *item = (afterme!=NULL) ? afterme : parent;
    if (item!=NULL)
    {
        bool isDir = static_cast<K3FileTreeViewItem*>(item)->isDir();
        if (isDir)
        {						// for the autoopen code
            K3FileTreeView::contentsDragMoveEvent(ev);
            return;
        }
    }

   ev->accept();
}


void ScanPackager::setAllowRename(bool on)
{
    kDebug() << "to" << on;
    setItemsRenameable(on);
}



void ScanPackager::showOpenWithMenu(KActionMenu *menu)
{
    K3FileTreeViewItem *curr = currentKFileTreeViewItem();
    QString mimeType = KMimeType::findByUrl(curr->url())->name();
    kDebug() << "Trying to open" << curr->url() << "which is" << mimeType;

    openWithOffers = KMimeTypeTrader::self()->query(mimeType);

    if (openWithMapper==NULL)
    {
        openWithMapper = new QSignalMapper(this);
        connect(openWithMapper, SIGNAL(mapped(int)), SLOT(slotOpenWith(int)));
    }

//    if (openWithActions==NULL) openWithActions = new KActionCollection(this);
							// separate from application's actions
//    openWithActions->clear();				// delete any previous actions
    menu->menu()->clear();

    int i = 0;
    for (KService::List::ConstIterator it = openWithOffers.begin();
         it!=openWithOffers.end(); ++it, ++i)
    {
        KService::Ptr service = (*it);
        kDebug() << "> offer:" << (*it)->name();

        QString actionName((*it)->name().replace("&","&&"));
        KAction *act = new KAction(KIcon((*it)->icon()), actionName, this);
        connect(act, SIGNAL(triggered()), openWithMapper, SLOT(map()));
        openWithMapper->setMapping(act, i);
        menu->addAction(act);
    }

    menu->menu()->addSeparator();
    KAction *act = new KAction(i18n("Other..."), this);
    connect(act, SIGNAL(triggered()), openWithMapper, SLOT(map()));
    openWithMapper->setMapping(act, i);
    menu->addAction(act);
}


void ScanPackager::slotOpenWith(int idx)
{
    kDebug() << "idx" << idx;

    K3FileTreeViewItem *ftvi = currentKFileTreeViewItem();
    if (ftvi==NULL) return;
    KUrl::List urllist(ftvi->url());

    if (idx<openWithOffers.count())			// application from the menu
    {
        KService::Ptr ptr = openWithOffers[idx];
        // TODO: should the two KRun's have top level window as 3rd parameter?
        KRun::run(*ptr, urllist, this);
    }
    else KRun::displayOpenWithDialog(urllist, this);	// last item = "Other..."
}
