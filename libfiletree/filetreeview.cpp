/* This file is part of the KDEproject
   Copyright (C) 2000 David Faure <faure@kde.org>
                 2000 Carsten Pfeiffer <pfeiffer@kde.org>

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

#include "filetreeview.h"

#include <stdlib.h>

#include <qevent.h>
#include <qdir.h>
#include <qapplication.h>
#include <qtimer.h>
#include <qmimedata.h>

#include <kfileitem.h>

#include "filetreeviewitem.h"
#include "filetreebranch.h"
#include "libfiletree_logging.h"


#undef DEBUG_LISTING


FileTreeView::FileTreeView(QWidget *parent)
    : QTreeWidget(parent)
{
    setObjectName("FileTreeView");
    qCDebug(LIBFILETREE_LOG);

    setSelectionMode(QAbstractItemView::SingleSelection);
    setExpandsOnDoubleClick(false);			// we'll handle this ourselves
    setEditTriggers(QAbstractItemView::NoEditTriggers);	// maybe changed later

    m_wantOpenFolderPixmaps = true;
    m_currentBeforeDropItem = nullptr;
    m_dropItem = nullptr;
    m_busyCount = 0;

    m_autoOpenTimer = new QTimer(this);
    m_autoOpenTimer->setInterval((QApplication::startDragTime() * 3) / 2);
    connect(m_autoOpenTimer, &QTimer::timeout, this, &FileTreeView::slotAutoOpenFolder);

    // The slotExecuted only opens a path, while the slotExpanded populates it
    connect(this, &QTreeWidget::itemActivated, this, &FileTreeView::slotExecuted);
    connect(this, &QTreeWidget::itemExpanded, this, &FileTreeView::slotExpanded);
    connect(this, &QTreeWidget::itemCollapsed, this, &FileTreeView::slotCollapsed);
    connect(this, &QTreeWidget::itemDoubleClicked, this, &FileTreeView::slotDoubleClicked);
    connect(this, &QTreeWidget::itemSelectionChanged, this, &FileTreeView::slotSelectionChanged);
    connect(this, &QTreeWidget::itemEntered, this, &FileTreeView::slotOnItem);

    connect(model(), &QAbstractItemModel::dataChanged, this, &FileTreeView::slotDataChanged);

    m_openFolderPixmap = QIcon::fromTheme("folder-open");
}

FileTreeView::~FileTreeView()
{
    // We must make sure that the FileTreeTreeViewItems are deleted _before_ the
    // branches are deleted. Otherwise, the KFileItems would be destroyed
    // and the FileTreeViewItems will still hold dangling pointers to them.
    hide();
    clear();

    qDeleteAll(m_branches);
    m_branches.clear();
}

// This is used when dragging and dropping out of the view to somewhere else.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
QMimeData *FileTreeView::mimeData(const QList<QTreeWidgetItem *> &items) const
#else
QMimeData *FileTreeView::mimeData(const QList<QTreeWidgetItem *> items) const
#endif
{
    QMimeData *mimeData = new QMimeData();
    QList<QUrl> urlList;

    for (const QTreeWidgetItem *item : std::as_const(items))
    {
        const FileTreeViewItem *ftvi = static_cast<const FileTreeViewItem *>(item);
#ifdef DEBUG_LISTING
        qCDebug(LIBFILETREE_LOG) << ftvi->url();
#endif // DEBUG_LISTING
        urlList.append(ftvi->url());
    }

    mimeData->setUrls(urlList);
    return (mimeData);
}

// Dragging and dropping into the view.
void FileTreeView::setDropItem(QTreeWidgetItem *item)
{
    if (item != nullptr) {
        m_dropItem = item;
        // TODO: make auto-open an option, don't start timer if not enabled
        m_autoOpenTimer->start();
    } else {
        m_dropItem = nullptr;
        m_autoOpenTimer->stop();
    }
}

void FileTreeView::dragEnterEvent(QDragEnterEvent *ev)
{
    if (!ev->mimeData()->hasUrls()) {       // not an URL drag
        ev->ignore();
        return;
    }

    ev->acceptProposedAction();

    QList<QTreeWidgetItem *> items = selectedItems();
    m_currentBeforeDropItem = (items.count() > 0 ? items.first() : nullptr);
    setDropItem(itemAt(ev->pos()));
}

void FileTreeView::dragMoveEvent(QDragMoveEvent *ev)
{
    if (!ev->mimeData()->hasUrls()) {       // not an URL drag
        ev->ignore();
        return;
    }

    QTreeWidgetItem *item = itemAt(ev->pos());
    if (item == nullptr || item->isDisabled()) {   // over a valid item?
        // no, ignore drops on it
        setDropItem(nullptr);              // clear drop item
        return;
    }

    //FileTreeViewItem *ftvi = static_cast<FileTreeViewItem *>(item);
    //if (!ftvi->isDir()) item = item->parent();    // if file, highlight parent dir

    setCurrentItem(item);               // temporarily select it
    if (item != m_dropItem) {
        setDropItem(item);    // changed, update drop item
    }

    ev->accept();
}

void FileTreeView::dragLeaveEvent(QDragLeaveEvent *ev)
{
    if (m_currentBeforeDropItem != nullptr) {      // there was a current item
        // before the drag started
        setCurrentItem(m_currentBeforeDropItem);    // restore its selection
        scrollToItem(m_currentBeforeDropItem);
    } else if (m_dropItem != nullptr) {        // item selected by drag
        m_dropItem->setSelected(false);         // clear that selection
    }

    m_currentBeforeDropItem = nullptr;
    setDropItem(nullptr);
}

void FileTreeView::dropEvent(QDropEvent *ev)
{
    if (!ev->mimeData()->hasUrls()) {       // not an URL drag
        ev->ignore();
        return;
    }

    if (m_dropItem == nullptr) {
        return;    // invalid drop target
    }

    FileTreeViewItem *item = static_cast<FileTreeViewItem *>(m_dropItem);
#ifdef DEBUG_LISTING
    qCDebug(LIBFILETREE_LOG) << "onto" << item->url();
#endif // DEBUG_LISTING
    setDropItem(nullptr);					// stop timer now
							// also clears m_dropItem!
    emit dropped(ev, item);
    ev->accept();
}

void FileTreeView::slotCollapsed(QTreeWidgetItem *tvi)
{
    FileTreeViewItem *item = static_cast<FileTreeViewItem *>(tvi);
    if (item != nullptr && item->isDir()) {
        item->setIcon(0, itemIcon(item));
    }
}

void FileTreeView::slotExpanded(QTreeWidgetItem *tvi)
{
    FileTreeViewItem *item = static_cast<FileTreeViewItem *>(tvi);
    if (item == nullptr) {
        return;
    }

#ifdef DEBUG_LISTING
    qCDebug(LIBFILETREE_LOG) << item->text(0);
#endif // DEBUG_LISTING
    FileTreeBranch *branch = item->branch();

    // Check if the branch needs to be populated now
    if (item->isDir() && branch != nullptr && item->childCount() == 0) {
#ifdef DEBUG_LISTING
        qCDebug(LIBFILETREE_LOG) << "need to populate" << item->url();
#endif // DEBUG_LISTING
        if (!branch->populate(item->url(), item)) {
            qCWarning(LIBFILETREE_LOG) << "Branch populate" << item->url() << "failed";
        }
    }

    // set pixmap for open folders
    if (item->isDir() && item->isExpanded()) {
        item->setIcon(0, itemIcon(item));
    }
}

// Called when an item is single- or double-clicked, according to the
// configured selection model.
//
// If the item is a branch root, we don't want to expand/collapse it on
// a single click, but just to select it.  An explicit double click will
// do the expand/collapse.

void FileTreeView::slotExecuted(QTreeWidgetItem *item)
{
    if (item == nullptr) {
        return;
    }

    FileTreeViewItem *ftvi = static_cast<FileTreeViewItem *>(item);
    if (ftvi != nullptr && ftvi->isDir() && !ftvi->isRoot()) {
        item->setExpanded(!item->isExpanded());
    }
}

void FileTreeView::slotDoubleClicked(QTreeWidgetItem *item)
{
    if (item == nullptr) {
        return;
    }

    FileTreeViewItem *ftvi = static_cast<FileTreeViewItem *>(item);
    if (ftvi != nullptr && ftvi->isRoot()) {
        item->setExpanded(!item->isExpanded());
    }
}

void FileTreeView::slotAutoOpenFolder()
{
    m_autoOpenTimer->stop();

#ifdef DEBUG_LISTING
    qCDebug(LIBFILETREE_LOG) << "children" << m_dropItem->childCount() << "expanded" << m_dropItem->isExpanded();
#endif // DEBUG_LISTING
    if (m_dropItem->childCount() == 0) {
        return;						// nothing to expand
    }
    if (m_dropItem->isExpanded()) {
        return;						// already expanded
    }

    m_dropItem->setExpanded(true);			// expand the item
}

void FileTreeView::slotSelectionChanged()
{
    if (m_dropItem != nullptr) {           // don't do this during the dragmove
    }
}

void FileTreeView::slotDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    if (topLeft.column() != 0) return;				// not the file name
    if (topLeft.row() != bottomRight.row()) return;		// not a single row
    if (topLeft.column() != bottomRight.column()) return;	// not a single column

    FileTreeViewItem *item = static_cast<FileTreeViewItem *>(itemFromIndex(topLeft));
    if (item->url().hasFragment()) return;		// ignore for sub-images

    QString oldName = item->url().fileName();
    QString newName = item->text(0);

    if (oldName == newName) return;			// no change of name
    if (newName.isEmpty()) return;			// no new name

    emit fileRenamed(item, newName);
    item->branch()->itemRenamed(item);			// update branch's item map
}

FileTreeBranch *FileTreeView::addBranch(const QUrl &path, const QString &name,
                                        bool showHidden)
{
    const QIcon &folderPix = QIcon::fromTheme("inode-directory");
    return (addBranch(path, name, folderPix, showHidden));
}

FileTreeBranch *FileTreeView::addBranch(const QUrl &path, const QString &name,
                                        const QIcon &pix, bool showHidden)
{
    qCDebug(LIBFILETREE_LOG) << path << name;

    /* Open a new branch */
    FileTreeBranch *newBranch = new FileTreeBranch(this, path, name, pix,
            showHidden);
    return (addBranch(newBranch));
}

FileTreeBranch *FileTreeView::addBranch(FileTreeBranch *newBranch)
{
    connect(newBranch, &FileTreeBranch::populateStarted, this, &FileTreeView::slotStartAnimation);
    connect(newBranch, &FileTreeBranch::populateFinished, this, &FileTreeView::slotStopAnimation);
    connect(newBranch, &FileTreeBranch::newTreeViewItems, this, &FileTreeView::slotNewTreeViewItems);

    m_branches.append(newBranch);
    return (newBranch);
}

const FileTreeBranch *FileTreeView::branch(const QString &searchName) const
{
#ifdef DEBUG_LISTING
    qCDebug(LIBFILETREE_LOG) << "searching for" << searchName;
#endif // DEBUG_LISTING
    for (const FileTreeBranch *branch : std::as_const(m_branches))
    {
        const QString bname = branch->name();
#ifdef DEBUG_LISTING
        qCDebug(LIBFILETREE_LOG) << "branch" << bname;
#endif // DEBUG_LISTING
        if (bname == searchName) {
#ifdef DEBUG_LISTING
            qCDebug(LIBFILETREE_LOG) << "Found requested branch";
#endif // DEBUG_LISTING
            return (branch);
        }
    }

    return (nullptr);
}

const FileTreeBranchList &FileTreeView::branches() const
{
    return (m_branches);
}

bool FileTreeView::removeBranch(FileTreeBranch *branch)
{
    if (m_branches.contains(branch)) {
        delete branch->root();
        m_branches.removeOne(branch);
        return (true);
    } else {
        return (false);
    }
}

void FileTreeView::setDirOnlyMode(FileTreeBranch *branch, bool bom)
{
    if (branch != nullptr) {
        branch->setDirOnlyMode(bom);
    }
}

void FileTreeView::slotNewTreeViewItems(FileTreeBranch *branch, const FileTreeViewItemList &items)
{
    if (branch == nullptr) {
        return;
    }
#ifdef DEBUG_LISTING
    qCDebug(LIBFILETREE_LOG) << "items" << items.count();
#endif // DEBUG_LISTING

    /* Sometimes it happens that new items should become selected, i.e. if the user
     * creates a new dir, he probably wants it to be selected. This can not be done
     * right after creating the directory or file, because it takes some time until
     * the item appears here in the treeview. Thus, the creation code sets the member
     * m_neUrlToSelect to the required url. If this url appears here, the item becomes
     * selected and the member nextUrlToSelect will be cleared.
     */
    if (!m_nextUrlToSelect.isEmpty())
    {
        for (FileTreeViewItem *it : std::as_const(items))
        {
            QUrl url = it->url();

            if (m_nextUrlToSelect.adjusted(QUrl::StripTrailingSlash|QUrl::NormalizePathSegments) ==
                url.adjusted(QUrl::StripTrailingSlash|QUrl::NormalizePathSegments)) {
                setCurrentItem(static_cast<QTreeWidgetItem *>(it));
                m_nextUrlToSelect = QUrl();
                break;
            }
        }
    }
}

QIcon FileTreeView::itemIcon(FileTreeViewItem *item) const
{
    QIcon pix;

    if (item != nullptr) {
        /* Check whether it is a branch root */
        FileTreeBranch *branch = item->branch();
        if (item == branch->root()) {
            pix = branch->pixmap();
            if (m_wantOpenFolderPixmaps && branch->root()->isExpanded()) {
                pix = branch->openPixmap();
            }
        } else {
            // TODO: different modes, user Pixmaps ?
            pix = QIcon::fromTheme(item->fileItem()->iconName());
            /* Only if it is a dir and the user wants open dir pixmap and it is open,
             * change the fileitem's pixmap to the open folder pixmap. */
            if (item->isDir() && m_wantOpenFolderPixmaps) {
                if (item->isExpanded()) {
                    pix = m_openFolderPixmap;
                }
            }
        }
    }

    return (pix);
}

void FileTreeView::slotStartAnimation(FileTreeViewItem *item)
{
    if (item == nullptr) {
        return;
    }
#ifdef DEBUG_LISTING
    qCDebug(LIBFILETREE_LOG) << "for" << item->text(0);
#endif // DEBUG_LISTING

    ++m_busyCount;
    setCursor(Qt::BusyCursor);
}

void FileTreeView::slotStopAnimation(FileTreeViewItem *item)
{
    if (item == nullptr) {
        return;
    }
#ifdef DEBUG_LISTING
    qCDebug(LIBFILETREE_LOG) << "for" << item->text(0);
#endif // DEBUG_LISTING
    if (m_busyCount <= 0) {
        return;
    }

    --m_busyCount;
    if (m_busyCount == 0) {
        unsetCursor();
    }
}

FileTreeViewItem *FileTreeView::selectedFileTreeViewItem() const
{
    QList<QTreeWidgetItem *> items = selectedItems();
    return (items.count() > 0 ? static_cast<FileTreeViewItem *>(items.first()) : nullptr);
}

const KFileItem *FileTreeView::selectedFileItem() const
{
    FileTreeViewItem *item = selectedFileTreeViewItem();
    return (item == nullptr ? nullptr : item->fileItem());
}

QUrl FileTreeView::selectedUrl() const
{
    FileTreeViewItem *item = selectedFileTreeViewItem();
    return (item != nullptr ? item->url() : QUrl());
}

FileTreeViewItem *FileTreeView::highlightedFileTreeViewItem() const
{
    QList<QTreeWidgetItem *> items = selectedItems();
    if (items.isEmpty()) return (nullptr);
    return (static_cast<FileTreeViewItem *>(items.first()));
}

const KFileItem *FileTreeView::highlightedFileItem() const
{
    FileTreeViewItem *item = highlightedFileTreeViewItem();
    return (item == nullptr ? nullptr : item->fileItem());
}

QUrl FileTreeView::highlightedUrl() const
{
    FileTreeViewItem *item = highlightedFileTreeViewItem();
    return (item != nullptr ? item->url() : QUrl());
}

void FileTreeView::slotOnItem(QTreeWidgetItem *item)
{
    FileTreeViewItem *i = static_cast<FileTreeViewItem *>(item);
    if (i != nullptr) emit onItem(i->url().url(QUrl::PreferLocalFile));
}

FileTreeViewItem *FileTreeView::findItemInBranch(const QString &branchName, const QString &relUrl) const
{
    const FileTreeBranch *br = branch(branchName);
    return (findItemInBranch(br, relUrl));
}

FileTreeViewItem *FileTreeView::findItemInBranch(const FileTreeBranch *branch, const QString &relPath) const
{
    if (branch==nullptr) return (nullptr);			// no branch to search

    FileTreeViewItem *ret;
    if (relPath.isEmpty() || relPath=="/") ret = branch->root();
    else ret = branch->findItemByPath(relPath);
    return (ret);
}

bool FileTreeView::showFolderOpenPixmap() const
{
    return (m_wantOpenFolderPixmaps);
}

void FileTreeView::setShowFolderOpenPixmap(bool showIt)
{
    m_wantOpenFolderPixmaps = showIt;
}

void FileTreeView::slotSetNextUrlToSelect(const QUrl &url)
{
    m_nextUrlToSelect = url;
}
