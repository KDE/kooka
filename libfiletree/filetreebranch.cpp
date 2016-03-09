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

#include <kfileitem.h>
#include <KIconLoader>
#include <KMimeType>

#include "filetreeview.h"


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
      m_lastFoundPath(QString::null),
      m_lastFoundItem(NULL),
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
    m_startPath = QString::null;            // recalculate when needed
    //qDebug() << "for" << u;

    // if no root is specified, create a new one
    if (m_root == NULL) m_root = new FileTreeViewItem(parent,
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

    connect(this, &FileTreeBranch::itemsAdded, this, &FileTreeBranch::slotItemsAdded);
    connect(this, &FileTreeBranch::itemsDeleted, this, &FileTreeBranch::slotItemsDeleted);
    connect(this, &FileTreeBranch::refreshItems, this, &FileTreeBranch::slotRefreshItems);
    connect(this, &FileTreeBranch::started, this, &FileTreeBranch::slotListerStarted);

    connect(this, static_cast<void (FileTreeBranch::*)(const QUrl &)>(&FileTreeBranch::completed),
            this, &FileTreeBranch::slotListerCompleted);

    connect(this, static_cast<void (FileTreeBranch::*)(const QUrl &)>(&FileTreeBranch::canceled),
            this, &FileTreeBranch::slotListerCanceled);

    connect(this, static_cast<void (FileTreeBranch::*)()>(&FileTreeBranch::clear),
            this, &FileTreeBranch::slotListerClear);

    connect(this, static_cast<void (FileTreeBranch::*)(const QUrl &)>(&FileTreeBranch::clear),
            this, &FileTreeBranch::slotListerClearUrl);

    connect(this, static_cast<void (FileTreeBranch::*)(const QUrl &, const QUrl &)>(&FileTreeBranch::redirection),
            this, &FileTreeBranch::slotRedirect);

    m_openChildrenURLs.append(u);
}

QUrl FileTreeBranch::rootUrl() const
{
    return (m_startURL);
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
    if (root() != NULL) {
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
    qDebug() << "lister started for" << url;
#endif // DEBUG_LISTING

    FileTreeViewItem *item = findItemByUrl(url);
    if (item != NULL) {
        emit populateStarted(item);
    }
}

// Renames seem to be emitted from KDirLister as a delete of the old followed
// by an add of the new.  Therefore there is no need to check for renaming here.

void FileTreeBranch::slotRefreshItems(const QList<QPair<KFileItem, KFileItem> > &list)
{
#ifdef DEBUG_LISTING
    qDebug() << "Refreshing" << list.count() << "items";
#endif // DEBUG_LISTING

    FileTreeViewItemList treeViewItList;		// tree view items updated
    for (int i = 0; i < list.count(); ++i) {
        const KFileItem fi2 = list[i].second;		// not interested in the first
#ifdef DEBUG_LISTING
        qDebug() << fi2.url();
#endif // DEBUG_LISTING
        FileTreeViewItem *item = findItemByUrl(fi2.url());
        if (item != NULL) {
            treeViewItList.append(item);
            //PORT QT5 item->setIcon(0, fi2.pixmap(KIconLoader::SizeSmall));
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
//    if (fi.isNull()) return (NULL);
//    //qDebug() << "for" << fi.url();
//    FileTreeViewItem *ftvi = static_cast<FileTreeViewItem *>(const_cast<void *>(fi.extraData(this)));
//    return (ftvi);
//}

FileTreeViewItem *FileTreeBranch::findItemByUrl(const QUrl &url)
{
    FileTreeViewItem *resultItem = NULL;
    QString p = url.adjusted(QUrl::StripTrailingSlash).path();

    if (m_startPath.isEmpty()) {
        m_startPath = m_startURL.adjusted(QUrl::StripTrailingSlash).path();
    }
    // recalculate if needed
    if (p == m_lastFoundPath) {         // do the most likely and
        // fastest lookup first
#ifdef DEBUG_MAPPING
        //qDebug() << "Found as last" << url;
#endif
        return (m_lastFoundItem);           // no more to do
    } else if (p == m_startPath) {          // see if is the root
#ifdef DEBUG_MAPPING
        //qDebug() << "Found as root" << url;
#endif
        resultItem = m_root;
    } else if (m_TVImap.contains(p)) {      // see if in our map
#ifdef DEBUG_MAPPING
        //qDebug() << "Found in map" << url;
#endif
        resultItem = m_TVImap[p];
    } else {                    // need to ask the lister
        // See comments on the removed treeItemForFileItem() above.
        // We should never get here, the TVImap should have the data for
        // every item that we create.
        //
        ////qDebug() << "searching dirlister for" << url;
        //const KFileItem it = findByUrl(url);
        //if (!it.isNull() )
        //{
        //    //qDebug() << "found item url" << it.url();
        //    resultItem = treeItemForFileItem(it);
        //}

#ifdef DEBUG_MAPPING
        //qDebug() << "Not found" << url;
#endif
    }

    if (resultItem != NULL) {           // found something
        m_lastFoundItem = resultItem;           // cache for next time
        m_lastFoundPath = p;                // path this applies to
    }

    return (resultItem);
}

void FileTreeBranch::itemRenamed(FileTreeViewItem *item)
{
    QString p = m_TVImap.key(item);         // find key for that item
    if (p.isEmpty()) {
        return;    // not in map, ignore
    }

    m_TVImap.remove(p);                 // remove old key from map
    p = item->url().adjusted(QUrl::StripTrailingSlash).path();    // make key for new name
    m_TVImap[p] = item;                 // save new item in map
}

// No longer needed, itemsAdded signal passes parent URL
//FileTreeViewItem *FileTreeBranch::parentFTVItem(const KFileItem &fi)
//{
//    if (fi.isNull()) return (NULL);
//
//    QUrl url = fi.url();
//    //qDebug() << "for" << url;
//    url.setFileName(QString::null);
//    return (findItemByUrl(url));
//}

FileTreeViewItem *FileTreeBranch::createTreeViewItem(FileTreeViewItem *parent,
        const KFileItem &fileItem)
{
    FileTreeViewItem *tvi = NULL;

    if (parent != NULL && !fileItem.isNull()) {
        tvi = new FileTreeViewItem(parent, fileItem, this);
        //QT5 m_TVImap[fileItem.url().path(QUrl::RemoveTrailingSlash)] = tvi;
#ifdef DEBUG_MAPPING
        //qDebug() << "stored in map" << fileItem.url().path(QUrl::RemoveTrailingSlash);
#endif
    } else { //qDebug() << "no parent/fileitem for new item!";
    }
    return (tvi);
}

void FileTreeBranch::slotItemsAdded(const QUrl &parent, const KFileItemList &items)
{
    //qDebug() << "Adding" << items.count() << "items";

    FileTreeViewItem *parentItem = findItemByUrl(parent);
    if (parentItem == NULL) {
        //qDebug() << "parent item not found for" << parent;
        return;
    }

    FileTreeViewItem *newItem;
    FileTreeViewItemList treeViewItList;        // tree view items created
    for (KFileItemList::const_iterator it = items.constBegin();
            it != items.constEnd(); ++it) {
        const KFileItem currItem = (*it);

        /* Only create a new FileTreeViewItem if it does not yet exist */
        if (findItemByUrl(currItem.url()) != NULL) {
            continue;
        }

        newItem = createTreeViewItem(parentItem, currItem);
        if (newItem == NULL) {          // should never happen now,
            // 'parent' checked above
            //qDebug() << "failed to create item for" << currItem.url();
            continue;
        }

        // Cut off the file extension if requested, if it is not a directory
        if (!m_showExtensions && !currItem.isDir()) {
            QString name = currItem.text();
            //int mPoint = name.lastIndexOf('.');
            //if (mPoint>0) name = name.left(mPoint);
            newItem->setText(0, KMimeType::extractKnownExtension(name));
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
            qDebug() << "Doing stat on" << filename;
#endif // DEBUG_LISTING
            //qDebug() << "Doing stat on" << filename;
            struct stat statBuf;
            if (stat(filename.toLocal8Bit(), &statBuf) == 0) {
                int hardLinks = statBuf.st_nlink;  /* Count of dirs */
#ifdef DEBUG_LISTING
                qDebug() << "stat succeeded, hardlinks: " << hardLinks;
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
                    qDebug() << "Emitting directoryChildCount" << hardLinks << "for" << url;
#endif // DEBUG_LISTING
                    emit directoryChildCount(newItem, hardLinks);
                }
            } else {
                //qDebug() << "stat of" << filename << "failed!";
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
    qDebug() << "for" << fi->url();
#endif // DEBUG_LISTING

    FileTreeViewItem *ftvi = findItemByUrl(fi->url());
    if (ftvi == NULL) {
#ifdef DEBUG_LISTING
        qDebug() << "no tree item!";
#endif // DEBUG_LISTING
        return;
    }

    int nChildren = ftvi->childCount();
    if (nChildren > 0) {
#ifdef DEBUG_LISTING
        qDebug() << "child count" << nChildren;
#endif // DEBUG_LISTING
        for (int i = 0; i < nChildren; ++i) {
            FileTreeViewItem *ch = static_cast<FileTreeViewItem *>(ftvi->child(i));
            if (ch != NULL) {
                itemDeleted(ch->fileItem());
            }
        }
    }
#if 0 //PORT QT5
    QString p = fi->url().path(QUrl::RemoveTrailingSlash);
    if (m_lastFoundPath == p) {
        m_lastFoundPath = QString::null;        // invalidate last-found cache
        m_lastFoundItem = NULL;
    }
    m_TVImap.remove(p);                 // remove from item map
#endif
    delete ftvi;                    // finally remove view item
}

void FileTreeBranch::slotListerCanceled(const QUrl &url)
{
#ifdef DEBUG_LISTING
    qDebug() << "lister cancelled for" << url;
#endif // DEBUG_LISTING

    // remove the URL from the children-to-recurse list
    m_openChildrenURLs.removeAll(url);

    // stop animations, etc.
    FileTreeViewItem *item = findItemByUrl(url);
    if (item != NULL) {
        emit populateFinished(item);
    }
}

void FileTreeBranch::slotListerClear()
{
#ifdef DEBUG_LISTING
    qDebug();
#endif // DEBUG_LISTING
    /* this slots needs to clear all listed items, but NOT the root item */
    if (m_root != NULL) {
        deleteChildrenOf(m_root);
    }
}

void FileTreeBranch::slotListerClearUrl(const QUrl &url)
{
#ifdef DEBUG_LISTING
    qDebug() << "for" << url;
#endif // DEBUG_LISTING
    FileTreeViewItem *ftvi = findItemByUrl(url);
    if (ftvi != NULL) {
        deleteChildrenOf(ftvi);
    }
}

void FileTreeBranch::deleteChildrenOf(QTreeWidgetItem *parent)
{
    // for some strange reason, slotListerClearUrl() sometimes calls us
    // with a NULL parent.
    if (parent == NULL) {
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
        m_startPath = QString::null;            // recalculate when next needed
    }
}

void FileTreeBranch::slotListerCompleted(const QUrl &url)
{
#ifdef DEBUG_LISTING
    qDebug() << "lister completed for" << url;
#endif // DEBUG_LISTING
    FileTreeViewItem *currParent = findItemByUrl(url);
    if (currParent == NULL) {
        return;
    }

#ifdef DEBUG_LISTING
    qDebug() << "current parent" << currParent
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
    qDebug() << "recurseChildren" << m_recurseChildren
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
        qDebug() << "Recurse for" << url << wantRecurseUrl;
#endif // DEBUG_LISTING
        int nChildren = 0;

        if (wantRecurseUrl && currParent != NULL) {
            /* now walk again through the tree and populate the children to get +-signs */
            /* This is the starting point. The visible folder has finished,
               processing the children has not yet started */
            nChildren = currParent->childCount();
            if (nChildren == 0) {
                /* This happens if there is no child at all */
#ifdef DEBUG_LISTING
                qDebug() << "No children to recurse";
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
                    qDebug() << "Starting to list" << recurseUrl;
#endif // DEBUG_LISTING
                    openUrl(recurseUrl, KDirLister::Keep);
                }
            }
        }
    }
#ifdef DEBUG_LISTING
    else {
        qDebug() << "no need to recurse";
    }
#endif // DEBUG_LISTING
}

/* This slot is called when a tree view item is expanded in the GUI */
bool FileTreeBranch::populate(const QUrl &url, FileTreeViewItem *currItem)
{
    bool ret = false;
    if (currItem == NULL) {
        return (ret);
    }

#ifdef DEBUG_LISTING
    qDebug() << "populating" << url;
#endif // DEBUG_LISTING

    /* Add this url to the list of urls to recurse for children */
    if (m_recurseChildren) {
        m_openChildrenURLs.append(url);
#ifdef DEBUG_LISTING
        qDebug() << "Adding as open child";
#endif // DEBUG_LISTING
    }

    if (!currItem->alreadyListed()) {
#ifdef DEBUG_LISTING
        qDebug() << "Starting to list";
#endif // DEBUG_LISTING
        ret = openUrl(url, KDirLister::Keep);       // start the lister
    } else {
#ifdef DEBUG_LISTING
        qDebug() << "Children already exist";
#endif // DEBUG_LISTING
        slotListerCompleted(url);
        ret = true;
    }

    return (ret);
}
