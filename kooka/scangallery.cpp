/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 1999-2016 Klaas Freitag <Klaas.Freitag@gmx.de>	*
 *                          Jonathan Marten <jjm@keelhaul.me.uk>	*
 *									*
 *  Kooka is free software; you can redistribute it and/or modify it	*
 *  under the terms of the GNU Library General Public License as	*
 *  published by the Free Software Foundation and appearing in the	*
 *  file COPYING included in the packaging of this file;  either	*
 *  version 2 of the License, or (at your option) any later version.	*
 *									*
 *  As a special exception, permission is given to link this program	*
 *  with any version of the KADMOS OCR/ICR engine (a product of		*
 *  reRecognition GmbH, Kreuzlingen), and distribute the resulting	*
 *  executable without including the source code for KADMOS in the	*
 *  source distribution.						*
 *									*
 *  This program is distributed in the hope that it will be useful,	*
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of	*
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*
 *  GNU General Public License for more details.			*
 *									*
 *  You should have received a copy of the GNU General Public		*
 *  License along with this program;  see the file COPYING.  If		*
 *  not, see <http://www.gnu.org/licenses/>.				*
 *									*
 ************************************************************************/

#include "scangallery.h"

#include <qfileinfo.h>
#include <qdir.h>
#include <qevent.h>
#include <qapplication.h>
#include <qheaderview.h>
#include <qmenu.h>
#include <qdebug.h>

#include <kglobal.h>
#include <kmessagebox.h>
#include <kpropertiesdialog.h>
#include <kinputdialog.h>
#include <kiconloader.h>
#include <kfiledialog.h>
#include <klocalizedstring.h>
#include <kstandardguiitem.h>
#include <kimageio.h>
#include <kconfigskeleton.h>

#include <kio/global.h>
#include <kio/copyjob.h>
#include <kio/jobuidelegate.h>
// TODO: eliminate the below (deprecated)
#include <kio/netaccess.h>

#include "imgsaver.h"
#include "kookaimage.h"
#include "kookapref.h"
#include "kookasettings.h"

#include "imagemetainfo.h"


// FileTreeViewItem is not the same as KDE3's KFileTreeViewItem in that
// fileItem() used to return a KFileItem *, allowing the item to be modified
// through the pointer.  Now it returns a KFileItem which is a value copy of the
// internal one, not a pointer to it - so the internal KFileItem cannot
// be modified.  This means that we can't store information in the extra data
// of the KFileItem of a FileTreeViewItem.  Including, unfortunately, our
// KookaImage pointer :-(
//
// This is a consequence of commit 719513, "Making KFileItemList value based".
//
// We store the image pointer in the 'clientData' of the item (of the ported
// FileTreeView) instead.
// TODO: use FileTreeViewItem->data(Qt::UserRole)

ScanGallery::ScanGallery(QWidget *parent)
    : FileTreeView(parent)
{
    setObjectName("ScanGallery");

    //header()->setStretchEnabled(true,0);      // do we like this effect?

    setColumnCount(3);
    setRootIsDecorated(false);
    //setSortingEnabled(true);
    //sortByColumn(0, Qt::AscendingOrder);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    QStringList labels;
    labels << i18n("Name");
    labels << i18n("Size");
    labels << i18n("Format");
    setHeaderLabels(labels);

    headerItem()->setTextAlignment(0, Qt::AlignLeft);
    headerItem()->setTextAlignment(1, Qt::AlignLeft);
    headerItem()->setTextAlignment(2, Qt::AlignLeft);

    // Drag and Drop
    setDragEnabled(true);               // allow drags out
    model()->setSupportedDragActions(Qt::CopyAction | Qt::MoveAction);

    setAcceptDrops(true);               // allow drops in
    connect(this, SIGNAL(dropped(QDropEvent*,FileTreeViewItem*)),
            SLOT(slotUrlsDropped(QDropEvent*,FileTreeViewItem*)));

    connect(this, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
            SLOT(slotItemHighlighted(QTreeWidgetItem*)));
    connect(this, SIGNAL(itemActivated(QTreeWidgetItem*,int)),
            SLOT(slotItemActivated(QTreeWidgetItem*)));
    connect(this, SIGNAL(fileRenamed(FileTreeViewItem*,QString)),
            SLOT(slotFileRenamed(FileTreeViewItem*,QString)));
    connect(this, SIGNAL(itemExpanded(QTreeWidgetItem*)),
            SLOT(slotItemExpanded(QTreeWidgetItem*)));

    /* Preload frequently used icons */
    // KF5 PORT: check
    KIconLoader::global()->addAppDir("libkookascan");   // access to library icons
    mPixFloppy = KIconLoader::global()->loadIcon("media-floppy", KIconLoader::NoGroup, KIconLoader::SizeSmall);
    mPixGray   = KIconLoader::global()->loadIcon("palette-gray", KIconLoader::NoGroup, KIconLoader::SizeSmall);
    mPixBw     = KIconLoader::global()->loadIcon("palette-lineart", KIconLoader::NoGroup, KIconLoader::SizeSmall);
    mPixColor  = KIconLoader::global()->loadIcon("palette-color", KIconLoader::NoGroup, KIconLoader::SizeSmall);

    m_startup = true;
    m_currSelectedDir = QUrl();
    mSaver = NULL;
    mSavedTo = NULL;

    /* create a context menu and set the title */
    m_contextMenu = new QMenu();
    m_contextMenu->addSection(i18n("Gallery"));
}

ScanGallery::~ScanGallery()
{
    delete mSaver;
    //qDebug();
}

static QString columnStatesKey(int forIndex)
{
    return (QString("GalleryState%1").arg(forIndex));
}

void ScanGallery::saveHeaderState(int forIndex) const
{
    QString key = columnStatesKey(forIndex);
    qDebug() << "to" << key;
    const KConfigSkeletonItem *ski = KookaSettings::self()->columnStatesItem();
    Q_ASSERT(ski!=NULL);
    KConfigGroup grp = KookaSettings::self()->config()->group(ski->group());
    grp.writeEntry(key, header()->saveState().toBase64());
    grp.sync();
}

void ScanGallery::restoreHeaderState(int forIndex)
{
    QString key = columnStatesKey(forIndex);
    qDebug() << "from" << key;
    const KConfigSkeletonItem *ski = KookaSettings::self()->columnStatesItem();
    Q_ASSERT(ski!=NULL);
    const KConfigGroup grp = KookaSettings::self()->config()->group(ski->group());

    QString state = grp.readEntry(key, "");
    if (state.isEmpty()) return;

    QHeaderView *hdr = header();
    // same workaround as needed in Akregator (even with Qt 4.6),
    // see r918196 and r1001242 to kdepim/akregator/src/articlelistview.cpp
    hdr->resizeSection(hdr->logicalIndex(hdr->count() - 1), 1);
    hdr->restoreState(QByteArray::fromBase64(state.toAscii()));
}

void ScanGallery::openRoots()
{
    /* standard root always exists, ImgRoot creates it */
    KUrl rootUrl(KookaPref::galleryRoot());
    //qDebug() << "Standard root" << rootUrl.url();

    m_defaultBranch = openRoot(rootUrl, i18n("Kooka Gallery"));
    m_defaultBranch->setOpen(true);

    /* open more configurable image repositories, configuration TODO */
    //openRoot(KUrl(getenv("HOME")), i18n("Home Directory"));
}

FileTreeBranch *ScanGallery::openRoot(const QUrl &root, const QString &title)
{
    FileTreeBranch *branch = addBranch(root, title);

    branch->setOpenPixmap(KIconLoader::global()->loadIcon("folder-image", KIconLoader::Small));
    branch->setShowExtensions(true);

    setDirOnlyMode(branch, false);

    connect(branch, SIGNAL(newTreeViewItems(FileTreeBranch*,FileTreeViewItemList)),
            SLOT(slotDecorate(FileTreeBranch*,FileTreeViewItemList)));

    connect(branch, SIGNAL(changedTreeViewItems(FileTreeBranch*,FileTreeViewItemList)),
            SLOT(slotDecorate(FileTreeBranch*,FileTreeViewItemList)));

    connect(branch, SIGNAL(directoryChildCount(FileTreeViewItem*,int)),
            SLOT(slotDirCount(FileTreeViewItem*,int)));

    connect(branch, SIGNAL(populateFinished(FileTreeViewItem*)),
            SLOT(slotStartupFinished(FileTreeViewItem*)));

    return (branch);
}

void ScanGallery::slotStartupFinished(FileTreeViewItem *item)
{
    if (!m_startup) {
        return;    // already done
    }
    if (item != m_defaultBranch->root()) {
        return;    // not the 1st branch root
    }

    //qDebug();

    if (highlightedFileTreeViewItem() == NULL) {    // nothing currently selected,
        // select the branch root
        item->setSelected(true);
        emit galleryPathChanged(m_defaultBranch, "/");  // tell the history combo
    }

    m_startup = false;                  // don't do this again
}

void ScanGallery::contextMenuEvent(QContextMenuEvent *ev)
{
    ev->accept();
    if (m_contextMenu != NULL) {
        m_contextMenu->exec(ev->globalPos());
    }
}

static ImageFormat getImgFormat(const FileTreeViewItem *item)
{
    if (item == NULL) {
        return (ImageFormat(""));
    }

    const KFileItem *kfi = item->fileItem();
    if (kfi->isNull()) {
        return (ImageFormat(""));
    }

    // Check that this is a plausible image format (MIME type = "image/anything")
    // before trying to get the image type.
    QString mimetype = kfi->mimetype();
    if (!mimetype.startsWith("image/")) {
        return (ImageFormat(""));
    }

    return (ImageFormat::formatForUrl(kfi->url()));
}

static QString localFileName(const FileTreeViewItem *item)
{
    if (item == NULL) {
        return (QString::null);
    }

    bool isLocal = false;
    KUrl u = item->fileItem()->mostLocalUrl(isLocal);
    if (!isLocal) {
        return (QString::null);
    }

    return (u.toLocalFile());
}

static KookaImage *imageForItem(const FileTreeViewItem *item)
{
    if (item == NULL) return (NULL);			// get loaded image if any
    return (static_cast<KookaImage *>(item->clientData()));
}

void ScanGallery::slotItemHighlighted(QTreeWidgetItem *curr)
{
    FileTreeViewItem *item = static_cast<FileTreeViewItem *>(curr);
    //qDebug() << item->url();

    if (item->isDir()) {
        emit showImage(NULL, true);    // clear displayed image
    } else {
        KookaImage *img = imageForItem(item);
        emit showImage(img, false);         // clear or redisplay image
    }

    emit itemHighlighted(item->url(), item->isDir());
}

void ScanGallery::slotItemActivated(QTreeWidgetItem *curr)
{
    FileTreeViewItem *item = static_cast<FileTreeViewItem *>(curr);
    //qDebug() << item->url();

    //  Check if directory, hide image for now, later show a thumb view?
    if (item->isDir()) {                // is it a directory?
        emit showImage(NULL, true);            // unload current image
    } else {                    // not a directory
        //  Load the image if necessary. This is done by loadImageForItem,
        //  which is async (TODO). The image finally arrives in slotImageArrived.
        QApplication::setOverrideCursor(Qt::WaitCursor);
        emit aboutToShowImage(item->url());
        loadImageForItem(item);
        QApplication::restoreOverrideCursor();
    }

    //  Notify the new directory, if it has changed
    KUrl newDir = itemDirectory(item);
    if (m_currSelectedDir != newDir) {
        m_currSelectedDir = newDir;
        emit galleryPathChanged(item->branch(), itemDirectoryRelative(item));
    }
}

// These 2 slots are called when an item is clicked/activated in the thumbnail view.

void ScanGallery::slotHighlightItem(const QUrl &url)
{
    //qDebug() << url;

    FileTreeViewItem *found = findItemByUrl(url);
    if (found == NULL) {
        return;
    }

    bool b = blockSignals(true);
    scrollToItem(found);
    setCurrentItem(found);
    blockSignals(b);

    // Need to do this to update/clear the displayed image.  Causes a signal
    // to be sent back to the thumbnail view, but this is benign and fortunately
    // does not cause not a loop.
    slotItemHighlighted(found);
}

void ScanGallery::slotActivateItem(const QUrl &url)
{
    //qDebug() << url;

    FileTreeViewItem *found = findItemByUrl(url);
    if (found == NULL) {
        return;
    }

    slotItemActivated(found);
}

// This slot is called when an image has been changed by some external means
// (e.g. an image transformation).  The item image is reloaded only if it
// is still currently selected.

void ScanGallery::slotUpdatedItem(const QUrl &url)
{
    FileTreeViewItem *found = findItemByUrl(url);
    if (found == NULL) {
        return;
    }

    if (found->isSelected()) {              // only if still selected
        slotUnloadItem(found);              // ensure unloaded for updating
        slotItemActivated(found);           // load the new image
    }
}

void ScanGallery::slotDirCount(FileTreeViewItem *item, int cnt)
{
    if (item == NULL) {
        return;
    }
    if (!item->isDir()) {
        return;
    }

    int imgCount = 0;                   // total these separately,
    int dirCount = 0;                   // we want individual counts
    int fileCount = 0;                  // for files and subfolders

    for (int i = 0; i < item->childCount(); ++i) {
        FileTreeViewItem *ci = static_cast<FileTreeViewItem *>(item->child(i));
        if (ci->isDir()) {
            ++dirCount;
        } else {
            if (ImageFormat::formatForMime(ci->fileItem()->determineMimeType()).isValid()) {
                ++imgCount;
            } else {
                ++fileCount;
            }
        }
    }

    QString cc = "";
    if (dirCount == 0) {
        if ((imgCount + fileCount) == 0) {
            cc = i18n("empty");
        } else {
            if (fileCount == 0) {
                cc = i18np("one image", "%1 images", imgCount);
            } else {
                cc = i18np("one file", "%1 files", (imgCount + fileCount));
            }

        }
    } else {
        if (fileCount > 0) {
            cc = i18np("one file, ", "%1 files, ", (imgCount + fileCount));
        } else if (imgCount > 0) {
            cc = i18np("one image, ", "%1 images, ", imgCount);
        }

        cc += i18np("1 folder", "%1 folders", dirCount);
    }

    item->setText(1, (" " + cc));
}

void ScanGallery::slotItemExpanded(QTreeWidgetItem *item)
{
    if (item == NULL) {
        return;
    }
    if (!(static_cast<FileTreeViewItem *>(item))->isDir()) {
        return;
    }

    for (int i = 0; i < item->childCount(); ++i) {
        FileTreeViewItem *ci = static_cast<FileTreeViewItem *>(item->child(i));
        if (ci->isDir() && !ci->alreadyListed()) {
            ci->branch()->populate(ci->url(), ci);
        }
    }
}

void ScanGallery::slotDecorate(FileTreeViewItem *item)
{
    if (item == NULL) {
        return;
    }

    if (!item->isDir()) {               // dir is done in another slot
        ImageFormat format = getImgFormat(item);    // this is safe for any file
        item->setText(2, (QString(" " + format.name() + " ")));

        const KookaImage *img = imageForItem(item);
        if (img != NULL) {              // image appears to be loaded
            // set image depth pixmap
            if (img->depth() == 1) {
                item->setIcon(0, mPixBw);
            } else {
                if (img->isGrayscale()) {
                    item->setIcon(0, mPixGray);
                } else {
                    item->setIcon(0, mPixColor);
                }
            }
            // set image size column
            QString t = i18n(" %1 x %2", img->width(), img->height());
            item->setText(1, t);
        } else {                    // not yet loaded, show file info
            if (format.isValid()) {         // if a valid image file
                item->setIcon(0, mPixFloppy);
                const KFileItem *kfi = item->fileItem();
                if (!kfi->isNull()) {
                    item->setText(1, (" " + KIO::convertSize(kfi->size())));
                }
            } else {
                //QT5 item->setIcon(0, KIO::pixmapForUrl(item->url(), 0, KIconLoader::Small));
            }
        }
    }

    // This code is quite similar to m_nextUrlToSelect in FileTreeView::slotNewTreeViewItems
    // When scanning a new image, we wait for the KDirLister to notice the new file,
    // and then we have the FileTreeViewItem that we need to display the image.
    if (!m_nextUrlToShow.isEmpty()) {
        if (m_nextUrlToShow.adjusted(QUrl::StripTrailingSlash) ==
            item->url().adjusted(QUrl::StripTrailingSlash)) {
            m_nextUrlToShow = QUrl();           // do this first to prevent recursion
            slotItemActivated(item);
            setCurrentItem(item);           // neccessary in case of new file from D&D
        }
    }
}

void ScanGallery::slotDecorate(FileTreeBranch *branch, const FileTreeViewItemList &list)
{
    //qDebug() << "count" << list.count();
    for (FileTreeViewItemList::const_iterator it = list.constBegin();
            it != list.constEnd(); ++it) {
        FileTreeViewItem *ftvi = (*it);
        slotDecorate(ftvi);
        emit fileChanged(ftvi->fileItem());
    }
}

void ScanGallery::updateParent(const FileTreeViewItem *curr)
{
    FileTreeBranch *branch = branches().at(0);      /* There should be at least one */
    if (branch == NULL) {
        return;
    }

    KUrl dir = itemDirectory(curr);
    //qDebug() << "Updating directory" << dir;
    branch->updateDirectory(dir);

    FileTreeViewItem *parent = branch->findItemByUrl(dir);
    if (parent != NULL) {
        parent->setExpanded(true);    /* Ensure parent is expanded */
    }
}

// "Rename" action triggered in the GUI

void ScanGallery::slotRenameItems()
{
    FileTreeViewItem *curr = highlightedFileTreeViewItem();
    if (curr != NULL) {
        editItem(curr, 0);
    }
}

// Renaming has finished

bool ScanGallery::slotFileRenamed(FileTreeViewItem *item, const QString &newName)
{
    if (item->isRoot()) {
        return (false);    // cannot rename root here
    }

    KUrl urlFrom = item->url();
    //qDebug() << "url" << urlFrom << "->" << newName;
    QString oldName = urlFrom.fileName();

    KUrl urlTo(urlFrom);
    urlTo.setFileName(newName);

    /* clear selection, because the renamed image comes in through
     * kdirlister again
     */
    // slotUnloadItem(item);                // unnecessary, bug 68532
    // because of "note new URL" below
    //qDebug() << "Renaming " << urlFrom << "->" << urlTo;

    //setSelected(item,false);

    bool success = ImgSaver::renameImage(urlFrom, urlTo, true, this);
    if (success) {                  // rename the file
        item->setUrl(urlTo);                // note new URL
        emit fileRenamed(item->fileItem(), newName);
    } else {
        //qDebug() << "renaming failed";
        item->setText(0, oldName);          // restore original name
    }

//    setSelected(item,true);               // restore the selection
    return (success);
}

// TODO: this function does not appear to be used
/* ----------------------------------------------------------------------- */
/*
 * Method that checks if the new filename a user enters while renaming an image is valid.
 * It checks for a proper extension.
 */

static QString buildNewFilename(const QString &cmplFilename, const ImageFormat &currFormat)
{
    /* cmplFilename = new name the user wishes.
     * currFormat   = the current format of the image.
     * if the new filename has a valid extension, which is the same as the
     * format of the current, fine. A ''-String has to be returned.
     */
    QFileInfo fiNew(cmplFilename);
    QString base = fiNew.baseName();
    QString newExt = fiNew.suffix().toLower();
    QString nowExt = currFormat.extension();
    QString ext = "";

    //qDebug() << "Filename wanted:" << cmplFilename << "ext" << nowExt << "->" << newExt;

    if (newExt.isEmpty()) {
        /* ok, fine -> return the currFormat-Extension */
        ext = base + "." + nowExt;
    } else if (newExt == nowExt) {
        /* also good, no reason to put another extension */
        ext = cmplFilename;
    } else {
        /* new Ext. differs from the current extension. Later. */
        KMessageBox::sorry(NULL, i18n("You entered a file extension that differs from the existing one. That is not yet possible. Converting 'on the fly' is planned for a future release.\n"
                                      "Kooka corrects the extension."),
                           i18n("On the Fly Conversion"));
        ext = base + "." + nowExt;
    }
    return (ext);
}

/* ----------------------------------------------------------------------- */
/* The absolute URL of the item (if it is a directory), or its parent (if
   it is a file).
*/
QUrl ScanGallery::itemDirectory(const FileTreeViewItem *item) const
{
    if (item == NULL) {
        //qDebug() << "no item";
        return (QUrl());
    }

    QUrl u = item->url();
    if (!item->isDir()) {
        u = u.adjusted(QUrl::RemoveFilename);		// not a directory, remove file name
    } else {
        u = u.adjusted(QUrl::StripTrailingSlash);	// is a directory, ensure ends with "/"
        u.setPath(u.path()+'/');
    }
    return (u);
}

/* ----------------------------------------------------------------------- */
/* As above, but relative to the root of its branch.  The result does not
   begin with a leading slash, except that a single "/" means the root.
   If there is some problem (no branch, or the root/item URLs do not match),
   the full path is returned.
*/
QString ScanGallery::itemDirectoryRelative(const FileTreeViewItem *item) const
{
    const KUrl u = itemDirectory(item);
    const FileTreeBranch *branch = item->branch();
    if (branch == NULL) {
        return (u.path());    // no branch, can this ever happen?
    }

    QString rootUrl = branch->rootUrl().url(QUrl::StripTrailingSlash)+'/';
    QString itemUrl = u.url();
    //qDebug() << "itemurl" << itemUrl << "rooturl" << rootUrl;
    if (itemUrl.startsWith(rootUrl)) {
        itemUrl.remove(0, rootUrl.length());        // remove root URL prefix
        //qDebug() << "->" << itemUrl;
        if (itemUrl.isEmpty()) {
            itemUrl = "/";    // it is the root
        }
        //qDebug() << "->" << itemUrl;
    } else {
        //qDebug() << "item URL" << itemUrl << "does not start with root URL" << rootUrl;
    }

    return (itemUrl);
}

/* ----------------------------------------------------------------------- */
/* This slot receives a string from the gallery-path combobox shown under the
 * image gallery, the relative directory under the branch.  Now it is to assemble
 * a complete path from the data, find out the FileTreeViewItem associated
 * with it and call slotClicked with it.
 */

void ScanGallery::slotSelectDirectory(const QString &branchName, const QString &relPath)
{
    //qDebug() << "branch" << branchName << "path" << relPath;

    FileTreeViewItem *item;
    if (!branchName.isEmpty()) {
        item = findItemInBranch(branchName, relPath);
    } else {
        item = findItemInBranch(branches().at(0), relPath);
    }
    // assume the 1st/only branch
    if (item == NULL) {
        return;
    }

    scrollToItem(item);
    setCurrentItem(item);
    slotItemActivated(item);                // load thumbnails, etc.
}

void ScanGallery::loadImageForItem(FileTreeViewItem *item)
{
    if (item == NULL) {
        return;
    }

    const KFileItem *kfi = item->fileItem();
    if (kfi->isNull()) {
        return;
    }

    //qDebug() << "loading" << item->url();

    QString ret = QString::null;            // no error so far

    ImageFormat format = getImgFormat(item);        // check for valid image format
    if (!format.isValid()) {
        ret = i18n("Not a valid image format");
    } else {
        KookaImage *img = imageForItem(item);
        if (img == NULL) {              // image not already loaded
            // The image needs to be loaded. Possibly it is a multi-page image.
            // If it is, the kookaImage has a subImageCount larger than one. We
            // create an subimage-item for every subimage, but do not yet load
            // them.

            img = new KookaImage();
            ret = img->loadFromUrl(item->url());
            if (ret.isEmpty()) {            // image loaded OK
                img->setFileItem(kfi);          // store the fileitem

                //qDebug() << "subImage-count" << img->subImagesCount();
                if (img->subImagesCount() > 1) {    // look for subimages,
                    // create items for them
                    KIconLoader *loader = KIconLoader::global();

                    // Start at the image with index 1, that makes one less than
                    // are actually in the image. But image 0 was already created above.
                    FileTreeViewItem *prevItem = NULL;
                    for (int i = 1; i < img->subImagesCount(); i++) {
                        //qDebug() << "Creating subimage" << i;
                        KFileItem newKfi(*kfi);
                        FileTreeViewItem *subImgItem = new FileTreeViewItem(item, newKfi, item->branch());

                        // TODO: what's the equivalent?
                        //if (prevItem!=NULL) subImgItem->moveItem(prevItem);
                        prevItem = subImgItem;

                        subImgItem->setIcon(0, loader->loadIcon("editcopy", KIconLoader::Small));
                        subImgItem->setText(0, i18n("Sub-image %1", i));
                        KookaImage *subImgImg = new KookaImage(i, img);
                        subImgImg->setFileItem(&newKfi);
                        subImgItem->setClientData(subImgImg);
                    }
                }

                if (img->isSubImage()) {        // this is a subimage
                    //qDebug() << "it is a subimage";
                    if (img->isNull()) {        // if not already loaded,
                        //qDebug() << "extracting subimage";
                        img->extractNow();      // load it now
                    }
                }

                slotImageArrived(item, img);
            } else {
                delete img;             // nothing to load
            }
        }
    }

    if (!ret.isEmpty()) KMessageBox::error(this,    // image loading failed
                                               i18n("<qt>"
                                                       "<p>Unable to load the image<br>"
                                                       "<filename>%2</filename><br>"
                                                       "<br>"
                                                       "%1",
                                                       ret,
                                                       item->url().url(QUrl::PreferLocalFile)),
                                               i18n("Image Load Error"));
}

/* Hit this slot with a file for a kfiletreeviewitem. */
void ScanGallery::slotImageArrived(FileTreeViewItem *item, KookaImage *image)
{
    if (item == NULL || image == NULL) {
        return;
    }

    item->setClientData(image);             // note image for item
    slotDecorate(item);
    emit showImage(image, false);
}

const KookaImage *ScanGallery::getCurrImage(bool loadOnDemand)
{
    FileTreeViewItem *curr = highlightedFileTreeViewItem();
    if (curr == NULL) {
        return (NULL);    // no current item
    }
    if (curr->isDir()) {
        return (NULL);    // is a directory
    }

    KookaImage *img = imageForItem(curr);       // see if already loaded
    if (img == NULL) {              // no, try to do that
        if (!loadOnDemand) {
            return (NULL);    // not loaded, and don't want to
        }
        slotItemActivated(curr);            // select/load this image
        img = imageForItem(curr);           // and get image for it
    }

    return (img);
}

QString ScanGallery::getCurrImageFileName(bool withPath) const
{
    QString result = "";

    FileTreeViewItem *curr = highlightedFileTreeViewItem();
    if (! curr) {
        //qDebug() << "nothing selected!";
    } else {
        if (withPath) {
            result = localFileName(curr);
        } else {
            // TODO: the next 2 lines don't make sense
            KUrl url(localFileName(curr));
            url = curr->url();
            result = url.fileName();
        }
    }
    return (result);
}

bool ScanGallery::prepareToSave(const ImageMetaInfo *info)
{
    if (info == NULL) {
        //qDebug() << "no image info";
    } else {
        //qDebug() << "type" << info->getImageType();
    }

    delete mSaver; mSaver = NULL;           // recreate with clean info

    // Resolve where to save the new image when it arrives
    FileTreeViewItem *curr = highlightedFileTreeViewItem();
    if (curr == NULL) {             // into root if nothing is selected
        FileTreeBranch *branch = branches().at(0);  // there should be at least one
        if (branch != NULL) {
            // if user has created this????
            curr = findItemInBranch(branch, i18n("Incoming/"));
            if (curr == NULL) {
                curr = branch->root();
            }
        }

        if (curr == NULL) {
            return (false);    // should never happen
        }
        curr->setSelected(true);
    }

    mSavedTo = curr;                    // note for selecting later

    // Create the ImgSaver to use later
    KUrl dir(itemDirectory(curr));          // where new image will go
    mSaver = new ImgSaver(dir);             // create saver to use later

    if (info != NULL) {             // have image information,
        // tell saver about it
        ImgSaver::ImageSaveStatus stat = mSaver->setImageInfo(info);
        if (stat == ImgSaver::SaveStatusCanceled) {
            return (false);
        }
    }

    return (true);                  // all ready to save
}

QUrl ScanGallery::saveURL() const
{
    if (mSaver == NULL) {
        return (QUrl());
    }
    // TODO: relative to root
    return (mSaver->saveURL());
}

/* ----------------------------------------------------------------------- */
/* This slot takes a new scanned Picture and saves it.  */

void ScanGallery::addImage(const QImage *img, const ImageMetaInfo *info)
{
    if (img == NULL) {
        return;    // nothing to save!
    }
    //qDebug() << "size" << img->size() << "depth" << img->depth();

    if (mSaver == NULL) {
        prepareToSave(NULL);    // if not done already
    }
    if (mSaver == NULL) {
        return;    // should never happen
    }

    ImgSaver::ImageSaveStatus isstat = mSaver->saveImage(img);
    // try to save the image
    KUrl lurl = mSaver->lastURL();          // record where it ended up

    if (isstat != ImgSaver::SaveStatusOk &&     // image saving failed
            isstat != ImgSaver::SaveStatusCanceled) {   // user cancelled, just ignore
        KMessageBox::error(this, i18n("<qt>Could not save the image<br><filename>%2</filename><br><br>%1",
                                      mSaver->errorString(isstat),
                                      lurl.prettyUrl()),
                           i18n("Image Save Error"));
    }

    delete mSaver; mSaver = NULL;           // now finished with this

    if (isstat == ImgSaver::SaveStatusOk) {     // image was saved OK,
        // select the new image
        slotSetNextUrlToSelect(lurl);
        m_nextUrlToShow = lurl;
        if (mSavedTo != NULL) {
            updateParent(mSavedTo);
        }
    }
}

// Selects and loads the image with the given URL. This is used to restore the
// last displayed image on startup.

void ScanGallery::slotSelectImage(const QUrl &url)
{
    FileTreeViewItem *found = findItemByUrl(url);
    if (found == NULL) {
        found = m_defaultBranch->root();
    }

    scrollToItem(found);
    setCurrentItem(found);
    slotItemActivated(found);
}

FileTreeViewItem *ScanGallery::findItemByUrl(const QUrl &url, FileTreeBranch *branch)
{
    QUrl u(url);
    if (u.scheme() == "file") {				// for local files,
        QDir d(url.path());				// ensure path is canonical
        u.setPath(d.canonicalPath());
    }
    //qDebug() << "URL search for" << u;

    // Prepare a list of branches to search.  If the parameter 'branch'
    // is set, search only in the specified branch. If it is NULL, search
    // all branches.
    FileTreeBranchList branchList;
    if (branch != NULL) {
        branchList.append(branch);
    } else {
        branchList = branches();
    }

    FileTreeViewItem *foundItem = NULL;
    for (FileTreeBranchList::const_iterator it = branchList.constBegin();
            it != branchList.constEnd(); ++it) {
        FileTreeBranch *branchloop = (*it);
        FileTreeViewItem *ftvi = branchloop->findItemByUrl(u);
        if (ftvi != NULL) {
            foundItem = ftvi;
            //qDebug() << "found item for" << ftvi->url();
            break;
        }
    }

    return (foundItem);
}

void ScanGallery::slotExportFile()
{
    FileTreeViewItem *curr = highlightedFileTreeViewItem();
    if (curr == NULL) {
        return;
    }

    if (curr->isDir()) {
        //qDebug() << "Not yet implemented!";
        return;
    }

    KUrl fromUrl(curr->url());

    QString filter;
    ImageFormat format = getImgFormat(curr);
    if (format.isValid()) {
        filter = "*." + format.extension() + "|" + format.mime().comment() + "\n";
    }
// TODO: do we need the below?
    filter += "*|" + i18n("All Files");

    QString initial = "kfiledialog:///exportImage/" + fromUrl.fileName();
    KUrl fileName = KFileDialog::getSaveUrl(KUrl(initial), filter, this);
    if (!fileName.isValid()) {
        return;    // didn't get a file name
    }
    if (fromUrl == fileName) {
        return;    // can't save over myself
    }

    /* Since it is asynchron, we will never know if it succeeded. */
    ImgSaver::copyImage(fromUrl, fileName);
}

void ScanGallery::slotImportFile()
{
    FileTreeViewItem *curr = highlightedFileTreeViewItem();
    if (! curr) {
        return;
    }

    KUrl impTarget = curr->url();

    if (! curr->isDir()) {
        FileTreeViewItem *pa = static_cast<FileTreeViewItem *>(curr->parent());
        impTarget = pa->url();
    }
    //qDebug() << "Importing to" << impTarget;

    KUrl impUrl = KFileDialog::getImageOpenUrl(KUrl("kfiledialog:///importImage"), this, i18n("Import Image File to Gallery"));
    if (impUrl.isEmpty()) {
        return;
    }

    impTarget.addPath(impUrl.fileName());       // append the name of the sourcefile to the path
    m_nextUrlToShow = impTarget;
    ImgSaver::copyImage(impUrl, impTarget);
}

void ScanGallery::slotUrlsDropped(QDropEvent *ev, FileTreeViewItem *item)
{
    KUrl::List urls = ev->mimeData()->urls();
    if (urls.isEmpty()) {
        return;
    }

    //qDebug() << "onto" << (item == NULL ? "NULL" : item->url().prettyUrl())
    //<< "srcs" << urls.count() << "first" << urls.first();

    if (item == NULL) {
        return;
    }
    KUrl dest = item->url();

    // Check whether the drop is on top of a directory (in which case we
    // want to move/copy into it) or a file (move/copy into its containing
    // directory).
    if (!item->isDir()) {
        dest.setFileName(QString::null);
    }
    dest.adjustPath(KUrl::AddTrailingSlash);
    //qDebug() << "resolved destination" << dest;

    // Make the last URL to copy the one to select next
    KUrl nextSel = dest;
    nextSel.addPath(urls.back().fileName(KUrl::ObeyTrailingSlash));
    m_nextUrlToShow = nextSel;

    KIO::Job *job;
    // TODO: top level window as 3rd parameter?
    if (ev->dropAction() == Qt::MoveAction) {
        job = KIO::move(urls, dest);
    } else {
        job = KIO::copy(urls, dest);
    }
    connect(job, SIGNAL(result(KJob*)), SLOT(slotJobResult(KJob*)));
}

void ScanGallery::slotJobResult(KJob *job)
{
    //qDebug() << "error" << job->error();
    if (job->error()) {
        job->uiDelegate()->showErrorMessage();
    }
}

/* ----------------------------------------------------------------------- */
void ScanGallery::slotUnloadItems()
{
    FileTreeViewItem *curr = highlightedFileTreeViewItem();
    emit showImage(NULL, false);
    slotUnloadItem(curr);
}

void ScanGallery::slotUnloadItem(FileTreeViewItem *curr)
{
    if (curr == NULL) {
        return;
    }

    if (curr->isDir()) {                // is a directory
        for (int i = 0; i < curr->childCount(); ++i) {
            FileTreeViewItem *child = static_cast<FileTreeViewItem *>(curr->child(i));
            slotUnloadItem(child);          // recursively unload contents
        }
    } else {                    // is a file/image
        const KookaImage *image = imageForItem(curr);
        if (image == NULL) {
            return;    // ok, nothing to unload
        }

        if (image->subImagesCount() > 0) {      // image with subimages
            while (curr->childCount() > 0) {    // recursively unload subimages
                FileTreeViewItem *child = static_cast<FileTreeViewItem *>(curr->takeChild(0));
                slotUnloadItem(child);
                delete child;
            }
        }

        emit unloadImage(image);
        delete image;

        curr->setClientData(NULL);          // clear image from item
        slotDecorate(curr);
    }
}

void ScanGallery::slotItemProperties()
{
    FileTreeViewItem *curr = highlightedFileTreeViewItem();
    if (curr == NULL) {
        return;
    }
    KPropertiesDialog::showDialog(curr->url(), this);
}

/* ----------------------------------------------------------------------- */

void ScanGallery::slotDeleteItems()
{
    FileTreeViewItem *curr = highlightedFileTreeViewItem();
    if (curr == NULL) {
        return;
    }

    KUrl urlToDel = curr->url();            // item to be deleted
    bool isDir = curr->isDir();             // deleting a folder?
    QTreeWidgetItem *nextToSelect = curr->treeWidget()->itemBelow(curr);
    // select this afterwards
    QString s;
    QString dontAskKey;
    if (isDir) {
        s = i18n("<qt>Do you really want to permanently delete the folder<br>"
                 "<filename>%1</filename><br>"
                 "and all of its contents? It cannot be restored.", urlToDel.pathOrUrl());
        dontAskKey = "AskForDeleteDirs";
    } else {
        s = i18n("<qt>Do you really want to permanently delete the image<br>"
                 "<filename>%1</filename>?<br>"
                 "It cannot be restored.", urlToDel.pathOrUrl());
        dontAskKey = "AskForDeleteFiles";
    }

    if (KMessageBox::warningContinueCancel(this, s,
                                           i18n("Delete Gallery Item"),
                                           KStandardGuiItem::del(),
                                           KStandardGuiItem::cancel(),
                                           dontAskKey) != KMessageBox::Continue) {
        return;
    }

    slotUnloadItem(curr);
    //qDebug() << "Deleting" << urlToDel;
    /* Since we are currently talking about local files here, NetAccess is OK */
    if (!KIO::NetAccess::del(urlToDel, NULL)) {
        KMessageBox::error(this, i18n("<qt>Could not delete the image or folder<br><filename>%2</filename><br><br>%1",
                                      KIO::NetAccess::lastErrorString(),
                                      urlToDel.prettyUrl()),
                           i18n("File Delete Error"));
        return;
    }

    updateParent(curr);                 // update parent folder count
    if (isDir) {                    // remove from the name combo
        emit galleryDirectoryRemoved(curr->branch(), itemDirectoryRelative(curr));
    }

#if 0
    if (nextToSelect != NULL) {
        setSelected(nextToSelect, true);
    }
    //  TODO: if doing the above, also need to signal to update thumbnail
    //  as below.
    //
    //  But doing that leads to inconsistency between deleting the last item
    //  in a folder (nothing is selected afterwards) and deleting anything
    //  else (the next image is selected and loaded).  So leaving this
    //  commented out for now.
    curr = highlightedFileTreeViewItem();
    //qDebug() << "new selection after delete" << (curr == NULL ? "NULL" : curr->url().prettyURL());
    if (curr != NULL) {
        emit showItem(curr->fileItem());
    }
#endif
}

/* ----------------------------------------------------------------------- */
void ScanGallery::slotCreateFolder()
{
    bool ok;
    QString folder = KInputDialog::getText(i18n("New Folder"),
                                           i18n("Name for the new folder:"), QString::null,
                                           &ok, this);

    if (ok) {
        /* KIO create folder goes here */

        FileTreeViewItem *it = highlightedFileTreeViewItem();
        if (it) {
            KUrl url = it->url();

            /* If a directory is selected, the filename needs not to be deleted */
            if (! it->isDir()) {
                url.setFileName("");
            }
            /* add the folder name from user input */
            url.addPath(folder);
            //qDebug() << "Creating folder" << url;

            /* Since the new directory arrives in the packager in the newItems-slot, we set a
             * variable urlToSelectOnArrive here. The newItems-slot will honor it and select
             * the treeviewitem with that url.
             */
            slotSetNextUrlToSelect(url);

            if (! KIO::NetAccess::mkdir(url, 0, -1)) {
                //qDebug() << "Error: creation of" << url << "failed!";
            } else {
                /* created successfully */
                /* open the branch if necessary and select the new folder */

            }
        }
    }
}

void ScanGallery::setAllowRename(bool on)
{
    //qDebug() << "to" << on;
    setEditTriggers(on ? QAbstractItemView::DoubleClicked : QAbstractItemView::NoEditTriggers);
}
