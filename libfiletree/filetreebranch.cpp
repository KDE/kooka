/* This file is part of the KDEproject
   Copyright (C) 2000 David Faure <faure@kde.org>
   2000 Carsten Pfeiffer <pfeiffer@kde.org>
   2002 Klaas Freitag <freitag@suse.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "filetreebranch.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <qdir.h>
#include <qdebug.h>
#include <qmimedatabase.h>

#include <kfileitem.h>

#include "filetreeview.h"
#include "libfiletree_logging.h"


#undef DEBUG_LISTING
#undef DEBUG_MAPPING


FileTreeBranch::FileTreeBranch(FileTreeView *parent,
                               const QUrl &url,
                               const QString &name,
                               const QIcon &pix,
                               bool showHidden,
                               FileTreeViewItem *branchRoot)
    : KDirLister(parent),
      m_root(branchRoot),
      m_name(name),
      m_rootIcon(pix),
      m_openRootIcon(pix),
      m_lastFoundItem(nullptr),
      m_recurseChildren(true),
      m_showExtensions(true)
{
    setObjectName("FileTreeBranch");

    QUrl u(url);
    if (u.isLocalFile()) {				// for local files,
        QDir d(u.path());				// ensure path is canonical
        u.setPath(d.canonicalPath());
    }
    m_startURL = u;
    qCDebug(LIBFILETREE_LOG) << "for" << u;

    // if no root is specified, create a new one
    if (m_root == nullptr) m_root = new FileTreeViewItem(parent,
                KFileItem(u, "inode/directory", S_IFDIR),
                this);
    //m_root->setExpandable(true);
    //m_root->setFirstColumnSpanned(true);
    bool sb = blockSignals(true);           // don't want setText() to signal
    m_root->setIcon(0, pix);
    m_root->setText(0, name);
    m_root->setToolTip(0, QString("%1 - %2").arg(name, u.url(QUrl::PreferLocalFile)));
    blockSignals(sb);

    setShowingDotFiles(showHidden);

    connect(this, &KDirLister::itemsAdded, this, &FileTreeBranch::slotItemsAdded);
    connect(this, &KDirLister::itemsDeleted, this, &FileTreeBranch::slotItemsDeleted);
    connect(this, &KDirLister::refreshItems, this, &FileTreeBranch::slotRefreshItems);
    connect(this, &KDirLister::started, this, &FileTreeBranch::slotListerStarted);

    connect(this, QOverload<const QUrl &>::of(&KDirLister::completed), this, &FileTreeBranch::slotListerCompleted);
    connect(this, QOverload<const QUrl &>::of(&KDirLister::canceled), this, &FileTreeBranch::slotListerCanceled);
    connect(this, QOverload<>::of(&KDirLister::clear), this, &FileTreeBranch::slotListerClear);
    connect(this, QOverload<const QUrl &>::of(&KDirLister::clear), this, &FileTreeBranch::slotListerClearUrl);
    connect(this, QOverload<const QUrl &, const QUrl &>::of(&KDirLister::redirection), this, &FileTreeBranch::slotRedirect);

    m_openChildrenURLs.append(u);
}

QUrl FileTreeBranch::rootUrl() const
{
    QUrl u = m_startURL.adjusted(QUrl::StripTrailingSlash);
    u.setPath(u.path()+'/');
    return (u);
}

void FileTreeBranch::setRoot(FileTreeViewItem *r)
{
    m_root = r;
}

FileTreeViewItem *FileTreeBranch::root() const
{
    return (m_root);
}

QString FileTreeBranch::name() const
{
    return (m_name);
}

void FileTreeBranch::setName(const QString &newName)
{
    m_name = newName;
}

QIcon FileTreeBranch::pixmap() const
{
    return (m_rootIcon);
}

QIcon FileTreeBranch::openPixmap() const
{
    return (m_openRootIcon);
}

void FileTreeBranch::setOpen(bool open)
{
    if (root() != nullptr) {
        root()->setExpanded(open);
    }
}

void FileTreeBranch::setOpenPixmap(const QIcon &pix)
{
    m_openRootIcon = pix;
    if (root()->isExpanded()) {
        root()->setIcon(0, pix);
    }
}

void FileTreeBranch::slotListerStarted(const QUrl &url)
{
#ifdef DEBUG_LISTING
    qCDebug(LIBFILETREE_LOG) << "lister started for" << url;
#endif // DEBUG_LISTING

    FileTreeViewItem *item = findItemByUrl(url);
    if (item != nullptr) {
        emit populateStarted(item);
    }
}

// Renames seem to be emitted from KDirLister as a delete of the old followed
// by an add of the new.  Therefore there is no need to check for renaming here.

void FileTreeBranch::slotRefreshItems(const QList<QPair<KFileItem, KFileItem> > &list)
{
#ifdef DEBUG_LISTING
    qCDebug(LIBFILETREE_LOG) << "Refreshing" << list.count() << "items";
#endif // DEBUG_LISTING

    FileTreeViewItemList treeViewItList;		// tree view items updated
    for (int i = 0; i < list.count(); ++i) {
        const KFileItem fi2 = list[i].second;		// not interested in the first
#ifdef DEBUG_LISTING
        qCDebug(LIBFILETREE_LOG) << fi2.url();
#endif // DEBUG_LISTING
        FileTreeViewItem *item = findItemByUrl(fi2.url());
        if (item != nullptr) {
            treeViewItList.append(item);
            item->setIcon(0, QIcon::fromTheme(fi2.iconName()));
            item->setText(0, fi2.text());
        }
    }

    if (treeViewItList.count() > 0) {
        emit changedTreeViewItems(this, treeViewItList);
    }
}

// This would never work in KDE4.  It relies on being able to set the
// KFileItem's extra data to hold its corresponding tree item pointer,
// but since now KFileItem values (not pointers/references) are passed
// around it is not possible to modify the KDirLister's internal KFileItem.
//
// Use findItemByUrl(fi.url()) instead.
//
//FileTreeViewItem *FileTreeBranch::treeItemForFileItem(const KFileItem &fi)
//{
//    if (fi.isNull()) return (nullptr);
//    //qCDebug(LIBFILETREE_LOG) << "for" << fi.url();
//    FileTreeViewItem *ftvi = static_cast<FileTreeViewItem *>(const_cast<void *>(fi.extraData(this)));
//    return (ftvi);
//}

FileTreeViewItem *FileTreeBranch::findItemByUrl(const QUrl &url)
{
    FileTreeViewItem *resultItem = nullptr;

    if (url == m_lastFoundUrl) {			// most likely and fastest first
#ifdef DEBUG_MAPPING
        qCDebug(LIBFILETREE_LOG) << "Found as last" << url;
#endif
        return (m_lastFoundItem);			// no more to do
    } else if (url == m_startURL) {			// see if is the root
#ifdef DEBUG_MAPPING
        qCDebug(LIBFILETREE_LOG) << "Found as root" << url;
#endif
        resultItem = m_root;
    } else if (m_itemMap.contains(url)) {		// see if in our map
#ifdef DEBUG_MAPPING
        qCDebug(LIBFILETREE_LOG) << "Found in map" << url;
#endif
        resultItem = m_itemMap[url];
    } else {						// need to ask the lister
        // See comments on the removed treeItemForFileItem() above.
        // We should never get here, the TVImap should have the data for
        // every item that we create.
        //
        ////qCDebug(LIBFILETREE_LOG) << "searching dirlister for" << url;
        //const KFileItem it = findByUrl(url);
        //if (!it.isNull() )
        //{
        //    //qCDebug(LIBFILETREE_LOG) << "found item url" << it.url();
        //    resultItem = treeItemForFileItem(it);
        //}

#ifdef DEBUG_MAPPING
        qCDebug(LIBFILETREE_LOG) << "Not found" << url;
#endif
    }

    if (resultItem != nullptr) {				// found something
        m_lastFoundItem = resultItem;			// cache for next time
        m_lastFoundUrl = url;				// path this applies to
    }

    return (resultItem);
}

// Find an item by a relative path.  If the branch is known, this
// saves having to convert an input path into an URL and then doing
// lots of comparisons on it as above.
FileTreeViewItem *FileTreeBranch::findItemByPath(const QString &path)
{
#ifdef DEBUG_MAPPING
    qCDebug(LIBFILETREE_LOG) << path;
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    const QStringList pathSplit = path.split('/', Qt::SkipEmptyParts);
#else
    const QStringList pathSplit = path.split('/', QString::SkipEmptyParts);
#endif
    FileTreeViewItem *item = m_root;

    foreach (const QString &part, pathSplit)
    {
        FileTreeViewItem *foundItem = nullptr;
        for (int i = 0; i<item->childCount(); ++i)
        {
            FileTreeViewItem *child = static_cast<FileTreeViewItem *>(item->child(i));
            if (child->text(0)==part)
            {
                foundItem = child;
                break;
            }
        }

        if (foundItem==nullptr)
        {            
#ifdef DEBUG_MAPPING
            qCDebug(LIBFILETREE_LOG) << "didn't find" << part << "under" << item->url();
#endif
            return (nullptr);				// no child with that name
        }

        item = foundItem;
    }

#ifdef DEBUG_MAPPING
    qCDebug(LIBFILETREE_LOG) << "found" << item->url();
#endif
    return (item);
}

void FileTreeBranch::itemRenamed(FileTreeViewItem *item)
{
    QUrl u = m_itemMap.key(item);			// find key for that item
    if (u.isEmpty()) return;				// not in map, ignore
    m_itemMap.remove(u);				// remove old from map
    m_itemMap[item->url()] = item;			// save new item in map
}

// No longer needed, itemsAdded signal passes parent URL
//FileTreeViewItem *FileTreeBranch::parentFTVItem(const KFileItem &fi)
//{
//    if (fi.isNull()) return (nullptr);
//
//    QUrl url = fi.url();
//    //qCDebug(LIBFILETREE_LOG) << "for" << url;
//    url.setFileName(QString());
//    return (findItemByUrl(url));
//}

FileTreeViewItem *FileTreeBranch::createTreeViewItem(FileTreeViewItem *parent,
        const KFileItem &fileItem)
{
    FileTreeViewItem *tvi = nullptr;

    if (parent != nullptr && !fileItem.isNull()) {
        tvi = new FileTreeViewItem(parent, fileItem, this);
        const QString p = fileItem.url().url(QUrl::PreferLocalFile|QUrl::StripTrailingSlash);
        m_itemMap[fileItem.url()] = tvi;
#ifdef DEBUG_MAPPING
        qCDebug(LIBFILETREE_LOG) << "stored in map" << fileItem.url();
#endif
    } else {
#ifdef DEBUG_MAPPING
        qCDebug(LIBFILETREE_LOG) << "no parent/fileitem for new item!";
#endif
    }
    return (tvi);
}

void FileTreeBranch::slotItemsAdded(const QUrl &parent, const KFileItemList &items)
{
#ifdef DEBUG_LISTING
    qCDebug(LIBFILETREE_LOG) << "Adding" << items.count() << "items";
#endif // DEBUG_LISTING

    FileTreeViewItem *parentItem = findItemByUrl(parent);
    if (parentItem == nullptr) {
        qCWarning(LIBFILETREE_LOG) << "parent item not found for" << parent;
        return;
    }

    FileTreeViewItem *newItem;
    FileTreeViewItemList treeViewItList;        // tree view items created
    for (KFileItemList::const_iterator it = items.constBegin();
            it != items.constEnd(); ++it) {
        const KFileItem currItem = (*it);

        /* Only create a new FileTreeViewItem if it does not yet exist */
        if (findItemByUrl(currItem.url()) != nullptr) {
            continue;
        }

        newItem = createTreeViewItem(parentItem, currItem);
        if (newItem == nullptr) {          // should never happen now,
            // 'parent' checked above
            qCWarning(LIBFILETREE_LOG) << "failed to create item for" << currItem.url();
            continue;
        }

        // Cut off the file extension if requested, if it is not a directory
        if (!m_showExtensions && !currItem.isDir()) {
            QString name = currItem.text();
            //int mPoint = name.lastIndexOf('.');
            //if (mPoint>0) name = name.left(mPoint);

            QMimeDatabase db;
            QString ext = db.suffixForFileName(name);
            if (!ext.isEmpty())
            {
                name.chop(ext.length()+1);
                newItem->setText(0, name);
            }
        }

        // TODO: is this useful (for local dirs) even in non-dirOnlyMode?
        //
        /* Now try to find out if there are children for dirs in the treeview */
        /* This stats a directory on the local file system and checks the */
        /* hardlink entry in the stat-buf. This works only for local directories. */
        if (dirOnlyMode() && !m_recurseChildren && currItem.isLocalFile() && currItem.isDir()) {
            QUrl url = currItem.url();
            QString filename = url.toLocalFile();
            /* do the stat trick of Carsten. The problem is, that the hardlink
             *  count only contains directory links. Thus, this method only seem
             * to work in dir-only mode */
#ifdef DEBUG_LISTING
            qCDebug(LIBFILETREE_LOG) << "Doing stat on" << filename;
#endif // DEBUG_LISTING
            struct stat statBuf;
            if (stat(QFile::encodeName(filename).constData(), &statBuf) == 0) {
                int hardLinks = statBuf.st_nlink;  /* Count of dirs */
#ifdef DEBUG_LISTING
                qCDebug(LIBFILETREE_LOG) << "stat succeeded, hardlinks: " << hardLinks;
#endif // DEBUG_LISTING
                // If the link count is > 2, the directory likely has subdirs. If it's < 2
                // it's something weird like a mounted SMB share. In that case we don't know
                // if there are subdirs, thus show it as expandable.

                if (hardLinks != 2) {
                    newItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
                } else {
                    newItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicator);
                }

                if (hardLinks >= 2) { // "Normal" directory with subdirs
                    hardLinks -= 2;
#ifdef DEBUG_LISTING
                    qCDebug(LIBFILETREE_LOG) << "Emitting directoryChildCount" << hardLinks << "for" << url;
#endif // DEBUG_LISTING
                    emit directoryChildCount(newItem, hardLinks);
                }
            } else {
                qCWarning(LIBFILETREE_LOG) << "stat of" << filename << "failed!";
            }
        }

        treeViewItList.append(newItem);
    }

    if (treeViewItList.count() > 0) {
        emit newTreeViewItems(this, treeViewItList);
    }
}

void FileTreeBranch::setChildRecurse(bool t)
{
    m_recurseChildren = t;
    if (!t) {
        m_openChildrenURLs.clear();
    }
}

bool FileTreeBranch::childRecurse()
{
    return (m_recurseChildren);
}

void FileTreeBranch::setShowExtensions(bool visible)
{
    m_showExtensions = visible;
}

bool FileTreeBranch::showExtensions() const
{
    return (m_showExtensions);
}

/*
 * The signal that tells that a directory was deleted may arrive before the signal
 * for its children arrive. Thus, we must walk through the children of a dir and
 * remove them before removing the dir itself.
 */

void FileTreeBranch::slotItemsDeleted(const KFileItemList &items)
{
    for (KFileItemList::const_iterator it = items.constBegin();
            it != items.constEnd(); ++it) {
        const KFileItem fi = (*it);
        itemDeleted(&fi);
    }
}

void FileTreeBranch::itemDeleted(const KFileItem *fi)
{
    if (fi->isNull()) {
        return;
    }
#ifdef DEBUG_LISTING
    qCDebug(LIBFILETREE_LOG) << "for" << fi->url();
#endif // DEBUG_LISTING

    FileTreeViewItem *ftvi = findItemByUrl(fi->url());
    if (ftvi == nullptr) {
#ifdef DEBUG_LISTING
        qCDebug(LIBFILETREE_LOG) << "no tree item!";
#endif // DEBUG_LISTING
        return;
    }

    int nChildren = ftvi->childCount();
    if (nChildren > 0) {
#ifdef DEBUG_LISTING
        qCDebug(LIBFILETREE_LOG) << "child count" << nChildren;
#endif // DEBUG_LISTING
        for (int i = 0; i < nChildren; ++i) {
            FileTreeViewItem *ch = static_cast<FileTreeViewItem *>(ftvi->child(i));
            if (ch != nullptr) {
                itemDeleted(ch->fileItem());
            }
        }
    }

    QUrl u = fi->url();
    if (u == m_lastFoundUrl) {
        m_lastFoundUrl = QUrl();			// invalidate last-found cache
        m_lastFoundItem = nullptr;
    }
    m_itemMap.remove(u);				// remove from item map

    delete ftvi;					// finally remove view item
}

void FileTreeBranch::slotListerCanceled(const QUrl &url)
{
#ifdef DEBUG_LISTING
    qCDebug(LIBFILETREE_LOG) << "lister cancelled for" << url;
#endif // DEBUG_LISTING

    // remove the URL from the children-to-recurse list
    m_openChildrenURLs.removeAll(url);

    // stop animations, etc.
    FileTreeViewItem *item = findItemByUrl(url);
    if (item != nullptr) {
        emit populateFinished(item);
    }
}

void FileTreeBranch::slotListerClear()
{
#ifdef DEBUG_LISTING
    qCDebug(LIBFILETREE_LOG);
#endif // DEBUG_LISTING
    /* this slots needs to clear all listed items, but NOT the root item */
    if (m_root != nullptr) {
        deleteChildrenOf(m_root);
    }
}

void FileTreeBranch::slotListerClearUrl(const QUrl &url)
{
#ifdef DEBUG_LISTING
    qCDebug(LIBFILETREE_LOG) << "for" << url;
#endif // DEBUG_LISTING
    FileTreeViewItem *ftvi = findItemByUrl(url);
    if (ftvi != nullptr) {
        deleteChildrenOf(ftvi);
    }
}

void FileTreeBranch::deleteChildrenOf(QTreeWidgetItem *parent)
{
    // for some strange reason, slotListerClearUrl() sometimes calls us
    // with a nullptr parent.
    if (parent == nullptr) {
        return;
    }

    QList<QTreeWidgetItem *> childs = parent->takeChildren();
    qDeleteAll(childs);
}

void FileTreeBranch::slotRedirect(const QUrl &oldUrl, const QUrl &newUrl)
{
    if (oldUrl.adjusted(QUrl::StripTrailingSlash) ==
        m_startURL.adjusted(QUrl::StripTrailingSlash)) {
        m_startURL = newUrl;
    }
}

void FileTreeBranch::slotListerCompleted(const QUrl &url)
{
#ifdef DEBUG_LISTING
    qCDebug(LIBFILETREE_LOG) << "lister completed for" << url;
#endif // DEBUG_LISTING
    FileTreeViewItem *currParent = findItemByUrl(url);
    if (currParent == nullptr) {
        return;
    }

#ifdef DEBUG_LISTING
    qCDebug(LIBFILETREE_LOG) << "current parent" << currParent
                             << "already listed?" << currParent->alreadyListed();
#endif // DEBUG_LISTING

    emit populateFinished(currParent);
    emit directoryChildCount(currParent, currParent->childCount());

    /* This is a walk through the children of the last populated directory.
     * Here we start the dirlister on every child of the dir and wait for its
     * finish. When it has finished, we go to the next child.
     * This must be done for non local file systems in dirOnly- and Full-Mode
     * and for local file systems only in full mode, because the stat trick
     * (see addItem-Method) does only work for dirs, not for files in the directory.
     */
    /* Set bit that the parent dir was listed completely */
    currParent->setListed(true);

#ifdef DEBUG_LISTING
    qCDebug(LIBFILETREE_LOG) << "recurseChildren" << m_recurseChildren
                             << "isLocalFile" << m_startURL.isLocalFile()
                             << "dirOnlyMode" << dirOnlyMode();
#endif // DEBUG_LISTING

    if (m_recurseChildren && (!m_startURL.isLocalFile() || !dirOnlyMode())) {
        bool wantRecurseUrl = false;
        /* look if the url is in the list for url to recurse */
        foreach (const QUrl &u, m_openChildrenURLs) {
            /* it is only interesting that the url _is_in_ the list. */
            if (u.adjusted(QUrl::StripTrailingSlash) == url.adjusted(QUrl::StripTrailingSlash)) {
                wantRecurseUrl = true;
                break;
            }
        }

#ifdef DEBUG_LISTING
        qCDebug(LIBFILETREE_LOG) << "Recurse for" << url << wantRecurseUrl;
#endif // DEBUG_LISTING
        int nChildren = 0;

        if (wantRecurseUrl && currParent != nullptr) {
            /* now walk again through the tree and populate the children to get +-signs */
            /* This is the starting point. The visible folder has finished,
               processing the children has not yet started */
            nChildren = currParent->childCount();
            if (nChildren == 0) {
                /* This happens if there is no child at all */
#ifdef DEBUG_LISTING
                qCDebug(LIBFILETREE_LOG) << "No children to recurse";
#endif // DEBUG_LISTING
            }

            /* Since we have listed the children to recurse, we can remove the entry
             * in the list of the URLs to see the children.
             */
            m_openChildrenURLs.removeAll(url);
        }

        /* There are some children. We start a dirlister job on every child item
         * which is a directory to find out how much children are in the child
         * of the last opened dir.  Skip non directory entries.
        */

        for (int i = 0; i < nChildren; ++i) {
            const FileTreeViewItem *ch = static_cast<FileTreeViewItem *>(currParent->child(i));
            if (ch->isDir() && !ch->alreadyListed()) {
                const KFileItem *fi = ch->fileItem();
                if (!fi->isNull() && fi->isReadable()) {
                    QUrl recurseUrl = fi->url();
#ifdef DEBUG_LISTING
                    qCDebug(LIBFILETREE_LOG) << "Starting to list" << recurseUrl;
#endif // DEBUG_LISTING
                    openUrl(recurseUrl, KDirLister::Keep);
                }
            }
        }
    }
#ifdef DEBUG_LISTING
    else {
        qCDebug(LIBFILETREE_LOG) << "no need to recurse";
    }
#endif // DEBUG_LISTING
}

/* This slot is called when a tree view item is expanded in the GUI */
bool FileTreeBranch::populate(const QUrl &url, FileTreeViewItem *currItem)
{
    bool ret = false;
    if (currItem == nullptr) {
        return (ret);
    }

#ifdef DEBUG_LISTING
    qCDebug(LIBFILETREE_LOG) << "populating" << url;
#endif // DEBUG_LISTING

    /* Add this url to the list of urls to recurse for children */
    if (m_recurseChildren) {
        m_openChildrenURLs.append(url);
#ifdef DEBUG_LISTING
        qCDebug(LIBFILETREE_LOG) << "Adding as open child";
#endif // DEBUG_LISTING
    }

    if (!currItem->alreadyListed()) {
#ifdef DEBUG_LISTING
        qCDebug(LIBFILETREE_LOG) << "Starting to list";
#endif // DEBUG_LISTING
        ret = openUrl(url, KDirLister::Keep);       // start the lister
    } else {
#ifdef DEBUG_LISTING
        qCDebug(LIBFILETREE_LOG) << "Children already exist";
#endif // DEBUG_LISTING
        slotListerCompleted(url);
        ret = true;
    }

    return (ret);
}
