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
#include "filetreeview.moc"

#include <stdlib.h>

#include <qevent.h>
#include <qdir.h>
#include <qapplication.h>

#include <kdebug.h>
#include <kglobalsettings.h>
#include <kfileitem.h>
#include <kmimetype.h>
#include <kstandarddirs.h>
#include <kiconloader.h>

#include <kio/job.h>
#include <kio/global.h>

#include "filetreeviewitem.h"
#include "filetreebranch.h"


FileTreeView::FileTreeView(QWidget *parent)
    : QTreeWidget(parent)
{
    setObjectName("FileTreeView");
    kDebug();

    setDragEnabled(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setExpandsOnDoubleClick(false);			// we'll handle this ourselves
    setEditTriggers(QAbstractItemView::NoEditTriggers);	// maybe changed later

    m_wantOpenFolderPixmaps = true;
    m_currentBeforeDropItem = NULL;
    m_dropItem = NULL;
    m_busyCount = 0;

    m_autoOpenTimer = new QTimer( this );
    connect( m_autoOpenTimer, SIGNAL( timeout() ),
             this, SLOT( slotAutoOpenFolder() ) );

    /* The executed-Slot only opens  a path, while the expanded-Slot populates it */
    connect(this, SIGNAL(itemActivated( QTreeWidgetItem *, int)),
            SLOT(slotExecuted(QTreeWidgetItem *)));
    connect(this, SIGNAL(itemExpanded(QTreeWidgetItem *)),
            SLOT(slotExpanded(QTreeWidgetItem *)));
    connect(this, SIGNAL(itemCollapsed(QTreeWidgetItem *)),
            SLOT(slotCollapsed(QTreeWidgetItem *)));
    connect(this, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)),
            SLOT(slotDoubleClicked(QTreeWidgetItem *)));

    connect(model(), SIGNAL(dataChanged(const QModelIndex &,const QModelIndex &)),
            SLOT(slotDataChanged(const QModelIndex &,const QModelIndex &)));

    /* connections from the konqtree widget */
    connect(this, SIGNAL(itemSelectionChanged()),
            SLOT(slotSelectionChanged()));
    connect(this, SIGNAL(itemEntered(QTreeWidgetItem *, int)),
            SLOT(slotOnItem(QTreeWidgetItem *)));

    m_bDrag = false;

    m_openFolderPixmap = KIconLoader::global()->loadIcon("folder-open",
                                                         KIconLoader::Desktop,
                                                         KIconLoader::SizeSmall,
                                                         KIconLoader::ActiveState);
    kDebug() << "done";
}


FileTreeView::~FileTreeView()
{
    // we must make sure that the FileTreeTreeViewItems are deleted _before_ the
    // branches are deleted. Otherwise, the KFileItems would be destroyed
    // and the FileTreeViewItems had dangling pointers to them.
    hide();
    clear();

    qDeleteAll(m_branches);
    m_branches.clear();
}


bool FileTreeView::isValidItem(QTreeWidgetItem *item)
{
    if (item==NULL) return (false);
    return (indexOfTopLevelItem(item)!=-1);
}


// This is used when dragging and dropping out of the view to somewhere else.
QMimeData *FileTreeView::mimeData(const QList<QTreeWidgetItem *> items) const
{
    QMimeData *mimeData = new QMimeData();
    QList<QUrl> urlList;

    for (QList<QTreeWidgetItem*>::const_iterator it = items.constBegin();
         it!=items.constEnd(); ++it)
    {
        FileTreeViewItem *item = static_cast<FileTreeViewItem *>(*it);
        kDebug() << item->url();
        urlList.append(item->url());
    }

    mimeData->setUrls(urlList);
    return (mimeData);
}


// TODO: port drag and drop
#if 0
void FileTreeView::contentsDragEnterEvent(QDragEnterEvent *ev)
{
   if ( ! acceptDrag( ev ) )
   {
      ev->ignore();
      return;
   }
   ev->acceptProposedAction();
   m_currentBeforeDropItem = selectedItem();

   QTreeWidgetItem *item = itemAt( contentsToViewport( ev->pos() ) );
   if( item )
   {
      m_dropItem = item;
      m_autoOpenTimer->start( (QApplication::startDragTime() * 3) / 2 );
   }
   else
   {
       m_dropItem = NULL;
   }
}


void FileTreeView::contentsDragMoveEvent( QDragMoveEvent *ev)
{
   if( ! acceptDrag( ev) )
   {
      ev->ignore();
      return;
   }
   ev->acceptProposedAction();

   QTreeWidgetItem *afterme;
   QTreeWidgetItem *parent;
   findDrop( e->pos(), parent, afterme );

   // "afterme" is 0 when aiming at a directory itself
   QTreeWidgetItem *item = afterme ? afterme : parent;

   if( item!=NULL && item->isSelectable() )
   {
      setSelected( item, true );
      if( item != m_dropItem ) {
         m_autoOpenTimer->stop();
         m_dropItem = item;
         m_autoOpenTimer->start( (QApplication::startDragTime() * 3) / 2 );
      }
   }
   else
   {
      m_autoOpenTimer->stop();
      m_dropItem = NULL;
   }
}


void FileTreeView::contentsDragLeaveEvent(QDragLeaveEvent *ev)
{
   // Restore the current item to what it was before the dragging (#17070)
   if ( isValidItem(m_currentBeforeDropItem) )
   {
      setSelected( m_currentBeforeDropItem, true );
      ensureItemVisible( m_currentBeforeDropItem );
   }
   else if ( isValidItem(m_dropItem) )
      setSelected( m_dropItem, false ); // no item selected
   m_currentBeforeDropItem = NULL;
   m_dropItem = NULL;

}


void FileTreeView::contentsDropEvent(QDropEvent *ev)
{

    m_autoOpenTimer->stop();
    m_dropItem = NULL;

    kDebug();
    if( ! acceptDrag( ev) ) {
       ev->ignore();
       return;
    }

    ev->acceptProposedAction();

    QTreeWidgetItem *afterme;
    QTreeWidgetItem *parent;
    findDrop(ev->pos(), parent, afterme);

    //kDebug(250) << " parent=" << (parent?parent->text(0):QString())
    //             << " afterme=" << (afterme?afterme->text(0):QString()) << endl;

    if (ev->source() == viewport() && itemsMovable())
        movableDropEvent(parent, afterme);
    else
    {
       emit dropped(ev, afterme);
       emit dropped(this, ev, afterme);
       emit dropped(ev, parent, afterme);
       emit dropped(this, ev, parent, afterme);

       KUrl::List urls = KUrl::List::fromMimeData( ev->mimeData() );
       if ( urls.isEmpty() )
           return;
       emit dropped(this, ev, urls );

       KUrl parentURL;
       if( parent )
           parentURL = static_cast<FileTreeViewItem*>(parent)->url();
       else
           // can happen when dropping above the root item
           // Should we choose the first branch in such a case ??
           return;

       emit dropped(urls, parentURL );
       emit dropped(this , ev, urls, parentURL );
    }
}


bool FileTreeView::acceptDrag(QDropEvent *ev) const
{

   bool ancestOK= acceptDrops();
   // kDebug(250) << "Do accept drops: " << ancestOK;
   ancestOK = ancestOK && itemsMovable();
   // kDebug(250) << "acceptDrag: " << ancestOK;
   // kDebug(250) << "canDecode: " << KUrl::List::canDecode(e->mimeData());
   // kDebug(250) << "action: " << e->action();

   /*  K3ListView::acceptDrag(e);  */
   /* this is what K3ListView does:
    * acceptDrops() && itemsMovable() && (e->source()==viewport());
    * ask acceptDrops and itemsMovable, but not the third
    */
   return ancestOK && KUrl::List::canDecode( ev->mimeData() ) &&
       // Why this test? All DnDs are one of those AFAIK (DF)
      ( ev->dropAction() == Qt::CopyAction
     || ev->dropAction() == Qt::MoveAction
     || ev->dropAction() == Qt::LinkAction );
}
#endif


void FileTreeView::slotCollapsed(QTreeWidgetItem *tvi)
{
    FileTreeViewItem *item = static_cast<FileTreeViewItem *>(tvi);
    if (item!=NULL && item->isDir()) item->setIcon(0, itemIcon(item));
}


void FileTreeView::slotExpanded(QTreeWidgetItem *tvi)
{
    FileTreeViewItem *item = static_cast<FileTreeViewItem *>(tvi);
    if (item==NULL) return;

    kDebug() << item->text(0);

    FileTreeBranch *branch = item->branch();

    // Check if the branch needs to be populated now
    if (item->isDir() && branch!=NULL && item->childCount()==0)
    {
        kDebug() << "need to populate" << item->url();
        if (!branch->populate(item->url(), item))
        {
            kDebug() << "Branch populate failed!";
        }
    }

    // set pixmap for open folders
    if (item->isDir() && item->isExpanded()) item->setIcon(0, itemIcon(item));
}


// Called when an item is single- or double-clicked, according to the
// configured selection model.
//
// If the item is a branch root, we don't want to expand/collapse it on
// a single click, but just to select it.  An explicit double click will
// do the expand/collapse.

void FileTreeView::slotExecuted(QTreeWidgetItem *item)
{
    if (item==NULL) return;

    FileTreeViewItem *ftvi = static_cast<FileTreeViewItem *>(item);
    if (ftvi!=NULL && ftvi->isDir() && !ftvi->isRoot()) item->setExpanded(!item->isExpanded());
}


void FileTreeView::slotDoubleClicked(QTreeWidgetItem *item)
{
    if (item==NULL) return;

    FileTreeViewItem *ftvi = static_cast<FileTreeViewItem *>(item);
    if (ftvi!=NULL && ftvi->isRoot()) item->setExpanded(!item->isExpanded());
}


// TODO: needed? QTreeWidget does this automatically
void FileTreeView::slotAutoOpenFolder()
{
    m_autoOpenTimer->stop();

    if (!isValidItem(m_dropItem) || m_dropItem->isExpanded()) return;

    m_dropItem->setExpanded(true);
    //m_dropItem->repaint();
}


void FileTreeView::slotSelectionChanged()
{
    if (m_dropItem!=NULL)				// don't do this during the dragmove
    {
    }
}


void FileTreeView::slotDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    if (topLeft.column()!=0) return;			// not the file name
    if (topLeft.row()!=bottomRight.row()) return;	// not a single row
    if (topLeft.column()!=bottomRight.column()) return;	// not a single column

    QTreeWidgetItem *twi = itemFromIndex(topLeft);
    if (twi==NULL) return;
    FileTreeViewItem *item = static_cast<FileTreeViewItem *>(twi);

    QString oldName = item->url().fileName();
    QString newName = item->text(0);
    if (oldName==newName) return;			// no change
    if (newName.isEmpty()) return;			// no new name

    emit fileRenamed(item, newName);
    item->branch()->itemRenamed(item);			// update branch's item map
}


FileTreeBranch *FileTreeView::addBranch(const KUrl &path, const QString &name,
                              bool showHidden)
{
    const QIcon &folderPix = KIconLoader::global()->loadMimeTypeIcon( KMimeType::mimeType("inode/directory")->iconName(),
                                                                        KIconLoader::Desktop, KIconLoader::SizeSmall );
    return (addBranch(path, name, folderPix, showHidden));
}


FileTreeBranch *FileTreeView::addBranch(const KUrl &path, const QString &name,
                              const QIcon &pix, bool showHidden )
{
    kDebug() << path;

    /* Open a new branch */
    FileTreeBranch *newBranch = new FileTreeBranch( this, path, name, pix,
                                                     showHidden );
    return (addBranch(newBranch));
}


FileTreeBranch *FileTreeView::addBranch(FileTreeBranch *newBranch)
{
    connect(newBranch, SIGNAL(populateStarted(FileTreeViewItem *)),
            SLOT(slotStartAnimation(FileTreeViewItem *)));
    connect(newBranch, SIGNAL(populateFinished(FileTreeViewItem *)),
            SLOT(slotStopAnimation(FileTreeViewItem *)));

    connect(newBranch, SIGNAL(newTreeViewItems(FileTreeBranch *,const FileTreeViewItemList &)),
            SLOT(slotNewTreeViewItems(FileTreeBranch *,const FileTreeViewItemList &)));

    m_branches.append(newBranch);
    return (newBranch);
}


FileTreeBranch *FileTreeView::branch(const QString &searchName)
{
    for (FileTreeBranchList::const_iterator it = m_branches.constBegin();
             it!=m_branches.constEnd(); ++it)
    {
        FileTreeBranch *branch = (*it);
        QString bname = branch->name();
        kDebug() << "branch" << bname;
        if (bname==searchName)
        {
            kDebug() << "Found requested branch";
            return (branch);
        }
    }

    return (NULL);
}


FileTreeBranchList &FileTreeView::branches()
{
    return (m_branches);
}


bool FileTreeView::removeBranch(FileTreeBranch *branch)
{
    if (m_branches.contains(branch))
    {
        delete branch->root();
        m_branches.removeOne(branch);
        return (true);
    }
    else
    {
        return (false);
    }
}


void FileTreeView::setDirOnlyMode(FileTreeBranch *branch, bool bom)
{
    if (branch!=NULL) branch->setDirOnlyMode(bom);
}


void FileTreeView::slotNewTreeViewItems(FileTreeBranch *branch, const FileTreeViewItemList &items)
{
    if (branch==NULL) return;
    kDebug();

    /* Sometimes it happens that new items should become selected, i.e. if the user
     * creates a new dir, he probably wants it to be selected. This can not be done
     * right after creating the directory or file, because it takes some time until
     * the item appears here in the treeview. Thus, the creation code sets the member
     * m_neUrlToSelect to the required url. If this url appears here, the item becomes
     * selected and the member nextUrlToSelect will be cleared.
     */
    if (!m_nextUrlToSelect.isEmpty())
    {
        for (FileTreeViewItemList::const_iterator it = items.constBegin();
             it!=items.constEnd(); ++it)
        {
            KUrl url = (*it)->url();

            if (m_nextUrlToSelect.equals(url, KUrl::CompareWithoutTrailingSlash))
            {
                setCurrentItem(static_cast<QTreeWidgetItem *>(*it));
                m_nextUrlToSelect = KUrl();
                break;
            }
        }
    }
}


QIcon FileTreeView::itemIcon(FileTreeViewItem *item) const
{
    QIcon pix;

    if (item!=NULL)
    {
        /* Check whether it is a branch root */
        FileTreeBranch *branch = item->branch();
        if (item==branch->root())
        {
            pix = branch->pixmap();
            if (m_wantOpenFolderPixmaps && branch->root()->isExpanded())
            {
                pix = branch->openPixmap();
            }
        }
        else
        {
            // TODO: different modes, user Pixmaps ?
            pix = item->fileItem()->pixmap(KIconLoader::SizeSmall);

            /* Only if it is a dir and the user wants open dir pixmap and it is open,
             * change the fileitem's pixmap to the open folder pixmap. */
            if (item->isDir() && m_wantOpenFolderPixmaps)
            {
                if (item->isExpanded()) pix = m_openFolderPixmap;
            }
        }
    }

    return (pix);
}


void FileTreeView::slotStartAnimation(FileTreeViewItem *item)
{
    if (item==NULL) return;
    kDebug() << "for" << item->text(0);

    ++m_busyCount;
    setCursor(Qt::BusyCursor);
}


void FileTreeView::slotStopAnimation(FileTreeViewItem *item)
{
    if (item==NULL) return;
    kDebug() << "for" << item->text(0);
    if (m_busyCount<=0) return;

    --m_busyCount;
    if (m_busyCount==0) unsetCursor();
}


FileTreeViewItem *FileTreeView::selectedFileTreeViewItem() const
{
    QList<QTreeWidgetItem *> items = selectedItems();
    return (items.count()>0 ? static_cast<FileTreeViewItem *>(items.first()) : NULL);
}


const KFileItem *FileTreeView::selectedFileItem() const
{
    FileTreeViewItem *item = selectedFileTreeViewItem();
    return (item==NULL ? NULL : item->fileItem());
}


KUrl FileTreeView::selectedUrl() const
{
    FileTreeViewItem *item = selectedFileTreeViewItem();
    return (item!=NULL ? item->url() : KUrl());
}


FileTreeViewItem *FileTreeView::highlightedFileTreeViewItem() const
{
    return (static_cast<FileTreeViewItem *>(currentItem()));
}


const KFileItem *FileTreeView::highlightedFileItem() const
{
    FileTreeViewItem *item = highlightedFileTreeViewItem();
    return (item==NULL ? NULL : item->fileItem());
}


KUrl FileTreeView::highlightedUrl() const
{
    FileTreeViewItem *item = highlightedFileTreeViewItem();
    return (item!=NULL ? item->url() : KUrl());
}


void FileTreeView::slotOnItem(QTreeWidgetItem *item)
{
    FileTreeViewItem *i = static_cast<FileTreeViewItem *>( item );
    if (i!=NULL)
    {
        const KUrl url = i->url();
        if ( url.isLocalFile() )
            emit onItem( url.toLocalFile() );
        else
            emit onItem( url.prettyUrl() );
    }
}


FileTreeViewItem *FileTreeView::findItemInBranch(const QString &branchName, const QString &relUrl)
{
    FileTreeBranch *br = branch(branchName);
    return (findItemInBranch(br, relUrl));
}


FileTreeViewItem *FileTreeView::findItemInBranch(FileTreeBranch *branch, const QString &relUrl)
{
    FileTreeViewItem *ret = NULL;
    if (branch!=NULL)
    {
        if (relUrl.isEmpty() || relUrl=="/") ret = branch->root();
        else
        {
            QString partUrl(relUrl);
            if (partUrl.endsWith('/')) partUrl.chop(1);

            KUrl url = branch->rootUrl();
            if (QDir::isRelativePath(relUrl)) url.addPath(partUrl);
            else url.setPath(partUrl);
            kDebug() << "searching for" << url;
            ret = branch->findItemByUrl(url);
        }
    }

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


void FileTreeView::slotSetNextUrlToSelect(const KUrl &url)
{
    m_nextUrlToSelect = url;
}
