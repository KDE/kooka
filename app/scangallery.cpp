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
#include <qinputdialog.h>
#include <qfiledialog.h>
#include <qmimedata.h>

#include <kmessagebox.h>
#include <kpropertiesdialog.h>
#include <klocalizedstring.h>
#include <kstandardguiitem.h>
#include <kconfigskeleton.h>

#include <kio/global.h>
#include <kio/copyjob.h>
#include <kio/deletejob.h>
#include <kio/mkdirjob.h>
#include <kio/pixmaploader.h>
#include <kio/jobuidelegate.h>

#include "imgsaver.h"
#include "galleryroot.h"
#include "kookasettings.h"

#include "scanimage.h"
#include "imagefilter.h"
#include "scanicons.h"
#include "recentsaver.h"


#undef DEBUG_LOADING


// FileTreeViewItem is not the same as KDE3's KFileTreeViewItem in that
// fileItem() used to return a KFileItem *, allowing the item to be modified
// through the pointer.  Now it returns a KFileItem which is a value copy of the
// internal one, not a pointer to it - so the internal KFileItem cannot
// be modified.  This means that we can't store information in the extra data
// of the KFileItem of a FileTreeViewItem.  Including, unfortunately, our
// ScanImage pointer :-(
//
// This is a consequence of commit 719513, "Making KFileItemList value based".
//
// The image pointer is stored as the Qt::UserRole data of the item
// (of the ported FileTreeView) instead.

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
    setAcceptDrops(true);               // allow drops in
    connect(this, SIGNAL(dropped(QDropEvent*,FileTreeViewItem*)),
            SLOT(slotUrlsDropped(QDropEvent*,FileTreeViewItem*)));

    connect(this, SIGNAL(itemSelectionChanged()),
            SLOT(slotItemHighlighted()));
    connect(this, SIGNAL(itemActivated(QTreeWidgetItem*,int)),
            SLOT(slotItemActivated(QTreeWidgetItem*)));
    connect(this, SIGNAL(fileRenamed(FileTreeViewItem*,QString)),
            SLOT(slotFileRenamed(FileTreeViewItem*,QString)));
    connect(this, SIGNAL(itemExpanded(QTreeWidgetItem*)),
            SLOT(slotItemExpanded(QTreeWidgetItem*)));

    m_startup = true;
    m_currSelectedDir = QUrl();
    mSaver = nullptr;
    mSavedTo = nullptr;

    /* create a context menu and set the title */
    m_contextMenu = new QMenu(this);
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
    //qDebug() << "to" << key;
    const KConfigSkeletonItem *ski = KookaSettings::self()->columnStatesItem();
    Q_ASSERT(ski!=nullptr);
    KConfigGroup grp = KookaSettings::self()->config()->group(ski->group());
    grp.writeEntry(key, header()->saveState().toBase64());
    grp.sync();
}

void ScanGallery::restoreHeaderState(int forIndex)
{
    QString key = columnStatesKey(forIndex);
    //qDebug() << "from" << key;
    const KConfigSkeletonItem *ski = KookaSettings::self()->columnStatesItem();
    Q_ASSERT(ski!=nullptr);
    const KConfigGroup grp = KookaSettings::self()->config()->group(ski->group());

    QString state = grp.readEntry(key, "");
    if (state.isEmpty()) return;

    QHeaderView *hdr = header();
    // same workaround as needed in Akregator (even with Qt 4.6),
    // see r918196 and r1001242 to kdepim/akregator/src/articlelistview.cpp
    hdr->resizeSection(hdr->logicalIndex(hdr->count() - 1), 1);
    hdr->restoreState(QByteArray::fromBase64(state.toLocal8Bit()));
}

void ScanGallery::openRoots()
{
    /* standard root always exists, ImgRoot creates it */
    QUrl rootUrl = QUrl::fromLocalFile(GalleryRoot::root());
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

    if (highlightedFileTreeViewItem() == nullptr) {    // nothing currently selected,
        // select the branch root
        item->setSelected(true);
        emit galleryPathChanged(m_defaultBranch, "/");  // tell the history combo
    }

    m_startup = false;                  // don't do this again
}

void ScanGallery::contextMenuEvent(QContextMenuEvent *ev)
{
    ev->accept();
    if (m_contextMenu != nullptr) {
        m_contextMenu->exec(ev->globalPos());
    }
}

static ImageFormat getImgFormat(const FileTreeViewItem *item)
{
    if (item == nullptr) {
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


static ScanImage::Ptr imageForItem(const FileTreeViewItem *item)
{
    if (item==nullptr) return (nullptr);		// get loaded image if any
    return (item->data(0, Qt::UserRole).value<ScanImage::Ptr>());
}


void ScanGallery::slotItemHighlighted(QTreeWidgetItem *curr)
{
    if (curr==nullptr)
    {
        QList<QTreeWidgetItem *> selItems = selectedItems();
        if (!selItems.isEmpty()) curr = selItems.first();
    }
    FileTreeViewItem *item = static_cast<FileTreeViewItem *>(curr);
    if (item==nullptr) return;

    if (item->isDir()) emit showImage(nullptr, true);	// clear displayed image
    else
    {
        ScanImage::Ptr img = imageForItem(item);
        emit showImage(img, false);			// clear or redisplay image
    }

    emit itemHighlighted(item->url(), item->isDir());
}


void ScanGallery::slotItemActivated(QTreeWidgetItem *curr)
{
    FileTreeViewItem *item = static_cast<FileTreeViewItem *>(curr);
    //qDebug() << item->url();

    //  Check if directory, hide image for now, later show a thumb view?
    if (item->isDir()) {                // is it a directory?
        emit showImage(nullptr, true);            // unload current image
    } else {                    // not a directory
        //  Load the image if necessary. This is done by loadImageForItem,
        //  which is async (TODO). The image finally arrives in slotImageArrived.
        QApplication::setOverrideCursor(Qt::WaitCursor);
        emit aboutToShowImage(item->url());
        loadImageForItem(item);
        QApplication::restoreOverrideCursor();
    }

    //  Notify the new directory, if it has changed
    QUrl newDir = itemDirectory(item);
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
    if (found == nullptr) {
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
    if (found == nullptr) {
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
    if (found == nullptr) {
        return;
    }

    if (found->isSelected()) {              // only if still selected
        slotUnloadItem(found);              // ensure unloaded for updating
        slotItemActivated(found);           // load the new image
    }
}

void ScanGallery::slotDirCount(FileTreeViewItem *item, int cnt)
{
    if (item == nullptr) {
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
    if (item == nullptr) {
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
    if (item == nullptr) return;

#ifdef DEBUG_LOADING
    qDebug() << item->url();
#endif // DEBUG_LOADING
    const bool isSubImage = item->url().hasFragment();	// is this a sub-image?

    if (!item->isDir())					// directories are done elsewhere
    {
        ImageFormat format = getImgFormat(item);	// this is safe for any file
        if (!isSubImage)				// no format for subimages
        {
            item->setText(2, (QString(" %1 ").arg(format.name())));
        }

        ScanImage::Ptr img = imageForItem(item);
        if (!img.isNull())				// image is loaded
        {
            // set image depth pixmap as appropriate
            QIcon icon;
            if (img->depth() == 1) icon = ScanIcons::self()->icon(ScanIcons::BlackWhite);
            else
            {
                if (img->isGrayscale()) icon = ScanIcons::self()->icon(ScanIcons::Greyscale);
                else icon = ScanIcons::self()->icon(ScanIcons::Colour);
            }
            item->setIcon(0, icon);

            if (img->subImagesCount() == 0)		// size except for containers
            {
                QString t = i18n(" %1 x %2", img->width(), img->height());
                item->setText(1, t);
            }
        }
        else						// image not loaded, show file info
        {
            if (format.isValid())			// if a valid image file
            {
                if (isSubImage)				// subimages don't show size
                {
                    item->setIcon(0, QIcon::fromTheme("edit-copy"));
                    item->setText(1, QString());
                }
                else
                {
                    item->setIcon(0, QIcon::fromTheme("media-floppy"));
                    const KFileItem *kfi = item->fileItem();
                    if (!kfi->isNull()) item->setText(1, (" " + KIO::convertSize(kfi->size())));
                }
            }
            else					// not an image file
            {						// show its standard MIME type
                item->setIcon(0, KIO::pixmapForUrl(item->url(), 0, KIconLoader::Small));
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
            setCurrentItem(item);           // necessary in case of new file from D&D
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
    if (branch == nullptr) {
        return;
    }

    QUrl dir = itemDirectory(curr);
    //qDebug() << "Updating directory" << dir;
    branch->updateDirectory(dir);

    FileTreeViewItem *parent = branch->findItemByUrl(dir);
    if (parent != nullptr) {
        parent->setExpanded(true);    /* Ensure parent is expanded */
    }
}

// "Rename" action triggered in the GUI

void ScanGallery::slotRenameItems()
{
    FileTreeViewItem *curr = highlightedFileTreeViewItem();
    if (curr != nullptr) {
        editItem(curr, 0);
    }
}

// Renaming has finished

bool ScanGallery::slotFileRenamed(FileTreeViewItem *item, const QString &newName)
{
    if (item->isRoot()) return (false);			// cannot rename root here

    QUrl urlFrom = item->url();
    //qDebug() << "url" << urlFrom << "->" << newName;
    QString oldName = urlFrom.fileName();

    QUrl urlTo(urlFrom.resolved(QUrl(newName)));

    /* clear selection, because the renamed image comes in through
     * kdirlister again
     */
    // slotUnloadItem(item);                // unnecessary, bug 68532
    // because of "note new URL" below
    qDebug() << "Renaming " << urlFrom << "->" << urlTo;

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
        KMessageBox::sorry(nullptr, i18n("You entered a file extension that differs from the existing one. That is not yet possible. Converting 'on the fly' is planned for a future release.\n"
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
    if (item == nullptr) {
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
    const QUrl u = itemDirectory(item);
    const FileTreeBranch *branch = item->branch();
    if (branch == nullptr) {
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
    if (!branchName.isEmpty())				// find in specified branch
    {
        item = findItemInBranch(branchName, relPath);
    }
    else						// assume the 1st/only branch
    {
        item = findItemInBranch(branches().at(0), relPath);
    }
    if (item == nullptr) return;			// not found in branch

    scrollToItem(item);
    setCurrentItem(item);
    slotItemActivated(item);				// load thumbnails, etc.
}

void ScanGallery::loadImageForItem(FileTreeViewItem *item)
{
    if (item == nullptr) return;
    const KFileItem *kfi = item->fileItem();
    if (kfi->isNull()) return;

#ifdef DEBUG_LOADING
    qDebug() << "loading" << item->url();
#endif // DEBUG_LOADING
    QString ret;					// no error so far

    ImageFormat format = getImgFormat(item);		// check for valid image format
    if (!format.isValid())
    {
        ret = i18n("Not a supported image format");
    }
    else						// valid image
    {
        ScanImage::Ptr img = imageForItem(item);
        if (img.isNull())				// image not already loaded
        {
#ifdef DEBUG_LOADING
            qDebug() << "need to load image";
#endif // DEBUG_LOADING

            // The image needs to be loaded. Possibly it is a multi-page image.
            // If it is, the ScanImage has a subImageCount larger than one. We
            // create an subimage item for every subimage, but do not yet load
            // them.

            img.reset(new ScanImage(item->url()));
            if (img->errorString().isEmpty())		// image loaded OK
            {
                if (img->subImagesCount()>1)		// see if it has subimages
                {
#ifdef DEBUG_LOADING
                    qDebug() << "subimage count" << img->subImagesCount();
#endif // DEBUG_LOADING
                    if (item->childCount()==0)		// check not already created
                    {
#ifdef DEBUG_LOADING
                        qDebug() << "need to create subimages";
#endif // DEBUG_LOADING
                        // Create items for each subimage
                        QIcon subImgIcon = QIcon::fromTheme("edit-copy");

                        // Sub-images start counting from 1, ScanImage adjusts
                        // that back to the 0-based TIFF directory index.
                        for (int i = 1; i<=img->subImagesCount(); i++)
                        {
                            KFileItem newKfi(*kfi);

                            // Set the URL to mark this as a subimage.  The subimage
                            // number is set as the URL fragment;  this is detected by
                            // ScanImage::loadFromUrl() and used to extract the
                            // submimage.
                            QUrl u = newKfi.url();
                            u.setFragment(QString::number(i));
                            newKfi.setUrl(u);

                            // Create the item without a parent and then
                            // add it to the parent item later, so that
                            // the setText() below does not trigger a rename.
                            FileTreeViewItem *subImgItem = new FileTreeViewItem(
                                static_cast<FileTreeViewItem *>(nullptr), newKfi, item->branch());

                            subImgItem->setText(0, i18n("Sub-image %1", i));
                            subImgItem->setIcon(0, subImgIcon);
                            item->addChild(subImgItem);
                        }
                    }
                }
            }
            else
            {						// image loading failed
                qDebug() << "Failed to load image," << img->errorString();
                img.clear();				// don't try to use it below
            }
        }
#ifdef DEBUG_LOADING
        else qDebug() << "have an image already";
#endif // DEBUG_LOADING

        if (!img.isNull())				// already loaded, or loaded above
        {
            slotImageArrived(item, img);		// display the image
        }
    }

    if (!ret.isEmpty())					// image loading failed
    {
        KMessageBox::error(this,
                           xi18nc("@info", "Unable to load the image <filename>%2</filename><nl/>%1",
                                  ret, item->url().url(QUrl::PreferLocalFile)),
                           i18n("Image Load Error"));
    }
}


/* Hit this slot with a file for a kfiletreeviewitem. */
void ScanGallery::slotImageArrived(FileTreeViewItem *item, ScanImage::Ptr img)
{
    if (item==nullptr || img.isNull()) return;
							// note image for item
    item->setData(0, Qt::UserRole, QVariant::fromValue(img));
    slotDecorate(item);
    emit showImage(img, false);
}


ScanImage::Ptr ScanGallery::getCurrImage(bool loadOnDemand)
{
    FileTreeViewItem *curr = highlightedFileTreeViewItem();
    if (curr==nullptr) return (nullptr);		// no current item
    if (curr->isDir()) return (nullptr);		// is a directory

    ScanImage::Ptr img = imageForItem(curr);		// see if already loaded
    if (img.isNull())					// no, try to do that
    {
        if (!loadOnDemand) return (nullptr);		// not loaded, and don't want to
        slotItemActivated(curr);			// select/load this image
        img = imageForItem(curr);			// and get image for it
    }

    return (img);
}


QString ScanGallery::currentImageFileName() const
{
    QString result = "";

    const FileTreeViewItem *curr = highlightedFileTreeViewItem();
    if (curr==nullptr) return (QString());

    bool isLocal = false;
    const QUrl u = curr->fileItem()->mostLocalUrl(&isLocal);
    if (!isLocal) return (QString());
    return (u.toLocalFile());
}


bool ScanGallery::prepareToSave(ScanImage::ImageType type)
{
    qDebug() << "type" << type;

    delete mSaver; mSaver = nullptr;			// recreate a clean instance

    // Resolve where to save the new image when it arrives
    FileTreeViewItem *curr = highlightedFileTreeViewItem();
    if (curr==nullptr)					// into root if nothing is selected
    {
        FileTreeBranch *branch = branches().at(0);	// there should be at least one
        if (branch!=nullptr)
        {
            // if user has created this????
            curr = findItemInBranch(branch, i18n("Incoming/"));
            if (curr==nullptr) curr = branch->root();
        }

        if (curr==nullptr) return (false);		// should never happen
        curr->setSelected(true);
    }

    mSavedTo = curr;					// note for selecting later

    // Create the saver to use after the scan is complete
    QUrl dir(itemDirectory(curr));			// where new image will go
    mSaver = new ImgSaver(dir);				// create saver to use later
    // Pass the initial image information to the saver
    ImgSaver::ImageSaveStatus stat = mSaver->setImageInfo(type);
    if (stat==ImgSaver::SaveStatusCanceled) return (false);

    return (true);					// all ready to save
}

QUrl ScanGallery::saveURL() const
{
    if (mSaver == nullptr) {
        return (QUrl());
    }
    // TODO: relative to root
    return (mSaver->saveURL());
}

/* ----------------------------------------------------------------------- */
/* This slot takes a new scanned Picture and saves it.  */

void ScanGallery::addImage(ScanImage::Ptr img)
{
    if (img.isNull()) return;				// no image to add
							// if not done already
    if (mSaver==nullptr) prepareToSave(ScanImage::None);
    if (mSaver==nullptr) return;			// should never happen

    ImgSaver::ImageSaveStatus isstat = mSaver->saveImage(img.data());
							// try to save the image
    const QUrl lurl = mSaver->lastURL();		// find out where it ended up

    if (isstat!=ImgSaver::SaveStatusOk &&		// image saving failed
        isstat!=ImgSaver::SaveStatusCanceled)		// user cancelled, just ignore
    {
        KMessageBox::error(this, xi18nc("@info", "Could not save the image<nl/><filename>%2</filename><nl/>%1",
                                        mSaver->errorString(isstat),
                                        lurl.toDisplayString(QUrl::PreferLocalFile)),
                           i18n("Image Save Error"));
    }

    delete mSaver; mSaver = nullptr;			// now finished with this

    if (isstat==ImgSaver::SaveStatusOk)			// image was saved,
    {							// select the new image
        slotSetNextUrlToSelect(lurl);
        m_nextUrlToShow = lurl;
        if (mSavedTo!=nullptr) updateParent(mSavedTo);
    }
}

// Selects and loads the image with the given URL. This is used to restore the
// last displayed image on startup.

void ScanGallery::slotSelectImage(const QUrl &url)
{
    FileTreeViewItem *found = findItemByUrl(url);
    if (found == nullptr) {
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
    // is set, search only in the specified branch. If it is nullptr, search
    // all branches.
    FileTreeBranchList branchList;
    if (branch != nullptr) {
        branchList.append(branch);
    } else {
        branchList = branches();
    }

    FileTreeViewItem *foundItem = nullptr;
    for (FileTreeBranchList::const_iterator it = branchList.constBegin();
            it != branchList.constEnd(); ++it) {
        FileTreeBranch *branchloop = (*it);
        FileTreeViewItem *ftvi = branchloop->findItemByUrl(u);
        if (ftvi != nullptr) {
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
    if (curr == nullptr) {
        return;
    }

    if (curr->isDir()) {
        //qDebug() << "Not yet implemented!";
        return;
    }

    QUrl fromUrl(curr->url());

    QString filter;
    ImageFormat format = getImgFormat(curr);
    if (format.isValid()) filter = format.mime().filterString();
    else filter = i18n("All Files (*)");

    RecentSaver saver("exportImage");
    QUrl fileName = QFileDialog::getSaveFileUrl(this, i18nc("@title:window", "Export Image"),
                                                saver.recentUrl(fromUrl.fileName()), filter);
    if (!fileName.isValid()) return;			// didn't get a file name
    if (fileName==fromUrl) return;			// can't save over myself
    saver.save(fileName);

    // Since the copy operation is asynchronous,
    // we will never know if it succeeds.
    ImgSaver::copyImage(fromUrl, fileName);
}

void ScanGallery::slotImportFile()
{
    FileTreeViewItem *curr = highlightedFileTreeViewItem();
    if (curr==nullptr) return;

    QUrl impTarget = curr->url();
    if (!curr->isDir()) {
        FileTreeViewItem *pa = static_cast<FileTreeViewItem *>(curr->parent());
        impTarget = pa->url();
    }

    QString filter = ImageFilter::qtFilterString(ImageFilter::Reading, ImageFilter::AllImages|ImageFilter::AllFiles);

    RecentSaver saver("importImage");
    QUrl impUrl = QFileDialog::getOpenFileUrl(this, i18n("Import Image File to Gallery"),
                                              saver.recentUrl(), filter);
    if (!impUrl.isValid()) return;
    saver.save(impUrl);
							// use the name of the source file
    impTarget = impTarget.resolved(QUrl(impUrl.fileName()));
    m_nextUrlToShow = impTarget;
    qDebug() << "Importing" << impUrl << "->" << impTarget;
    ImgSaver::copyImage(impUrl, impTarget);
}

void ScanGallery::slotUrlsDropped(QDropEvent *ev, FileTreeViewItem *item)
{
    QList<QUrl> urls = ev->mimeData()->urls();
    if (urls.isEmpty()) {
        return;
    }

    //qDebug() << "onto" << (item == nullptr ? "nullptr" : item->url().prettyUrl())
    //<< "srcs" << urls.count() << "first" << urls.first();

    if (item == nullptr) return;
    QUrl dest = item->url();

    // Check whether the drop is on top of a directory (in which case we
    // want to move/copy into it) or a file (move/copy into its containing
    // directory).
    if (!item->isDir()) dest = dest.adjusted(QUrl::RemoveFilename);
    qDebug() << "resolved destination" << dest;

    // Make the last URL to copy the one to select next
    QUrl nextSel = dest.resolved(QUrl(urls.back().fileName()));
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
    emit showImage(nullptr, false);
    slotUnloadItem(curr);
}


void ScanGallery::slotUnloadItem(FileTreeViewItem *curr)
{
    if (curr==nullptr) return;

    if (curr->isDir())					// is a directory
    {
        for (int i = 0; i<curr->childCount(); ++i)
        {
            FileTreeViewItem *child = static_cast<FileTreeViewItem *>(curr->child(i));
            slotUnloadItem(child);			// recursively unload contents
        }
    }
    else						// is a file/image
    {
        ScanImage::Ptr img = imageForItem(curr);
        if (img.isNull()) return;			// nothing to unload

        if (img->subImagesCount()>0)			// image with subimages
        {
            while (curr->childCount()>0)		// recursively unload subimages
            {
                FileTreeViewItem *child = static_cast<FileTreeViewItem *>(curr->takeChild(0));
                slotUnloadItem(child);
                delete child;
            }
        }

        emit unloadImage(img);
							// unreference image from item
        curr->setData(0, Qt::UserRole, QVariant::fromValue(nullptr));
        slotDecorate(curr);
    }
}

void ScanGallery::slotItemProperties()
{
    FileTreeViewItem *curr = highlightedFileTreeViewItem();
    if (curr == nullptr) {
        return;
    }
    KPropertiesDialog::showDialog(curr->url(), this);
}

/* ----------------------------------------------------------------------- */

void ScanGallery::slotDeleteItems()
{
    FileTreeViewItem *curr = highlightedFileTreeViewItem();
    if (curr == nullptr) {
        return;
    }

    QUrl urlToDel = curr->url();			// item to be deleted
    bool isDir = curr->isDir();				// deleting a folder?
    QTreeWidgetItem *nextToSelect = curr->treeWidget()->itemBelow(curr);
    // select this afterwards
    QString s;
    QString dontAskKey;
    if (isDir) {
        s = xi18nc("@info", "Do you really want to permanently delete the folder<nl/>"
                   "<filename>%1</filename><nl/>"
                   "and all of its contents? It cannot be restored.", urlToDel.url(QUrl::PreferLocalFile));
        dontAskKey = "AskForDeleteDirs";
    } else {
        s = xi18nc("@info", "Do you really want to permanently delete the image<nl/>"
                   "<filename>%1</filename>?<nl/>"
                   "It cannot be restored.", urlToDel.url(QUrl::PreferLocalFile));
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
    qDebug() << "Deleting" << urlToDel;
    KIO::DeleteJob *job = KIO::del(urlToDel);
    if (!job->exec())
    {
        KMessageBox::error(this, xi18nc("@info", "Could not delete the image or folder<nl/><filename>%2</filename><nl/>%1",
                                        job->errorString(),
                                        urlToDel.url(QUrl::PreferLocalFile)),
                           i18n("File Delete Error"));
        return;
    }

    updateParent(curr);					// update parent folder count
    if (isDir) {					// remove from the name combo
        emit galleryDirectoryRemoved(curr->branch(), itemDirectoryRelative(curr));
    }

#if 0
    if (nextToSelect != nullptr) {
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
    //qDebug() << "new selection after delete" << (curr == nullptr ? "nullptr" : curr->url().prettyURL());
    if (curr != nullptr) {
        emit showItem(curr->fileItem());
    }
#endif
}

/* ----------------------------------------------------------------------- */
void ScanGallery::slotCreateFolder()
{
    QString folder = QInputDialog::getText(this, i18n("New Folder"),
                                           i18n("Name for the new folder:"));
    if (folder.isEmpty()) return;

    FileTreeViewItem *item = highlightedFileTreeViewItem();
    if (item==nullptr) return;

    // The GUI ensures that the action is only enabled if the current
    // item is a directory.  Hence, we can assume that it is and ensure
    // that its path ends with a slash before setting the file name.
    QUrl url = item->url().adjusted(QUrl::StripTrailingSlash);
    url.setPath(url.path()+'/');
    url = url.resolved(QUrl(folder));
    qDebug() << "Creating folder" << url;

    /* Since the new directory arrives in the packager in the newItems-slot, we set a
     * variable urlToSelectOnArrive here. The newItems-slot will honor it and select
     * the treeviewitem with that url.
     */
    slotSetNextUrlToSelect(url);

    KIO::MkdirJob *job = KIO::mkdir(url);
    if (!job->exec())
    {
        KMessageBox::error(this, xi18nc("@info", "Could not create the folder<nl/><filename>%2</filename><nl/>%1",
                                        job->errorString(), url.url(QUrl::PreferLocalFile)),
                           i18n("Folder Create Error"));
    }
}

void ScanGallery::setAllowRename(bool on)
{
    //qDebug() << "to" << on;
    setEditTriggers(on ? QAbstractItemView::DoubleClicked : QAbstractItemView::NoEditTriggers);
}
