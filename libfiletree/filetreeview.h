/* This file is part of the KDE project
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

#ifndef FILETREEVIEW_H
#define FILETREEVIEW_H


#include <qmap.h>
#include <qpoint.h>
#include <qicon.h>
#include <qtreewidget.h>

//#include <kdirnotify.h>

//#include <kio/job.h>

#include "filetreebranch.h"


class QTimer;


/**
 * The filetreeview offers a treeview on the file system which behaves like
 * a QTreeView showing files and/or directories in the file system.
 *
 * FileTreeView is able to handle more than one URL, represented by
 * KFileTreeBranch.
 *
 * Typical usage:
 * 1. create a FileTreeView fitting in your layout and add columns to it
 * 2. call addBranch to create one or more branches
 * 3. retrieve the root item with KFileTreeBranch::root() and set it open
 *    if desired. That starts the listing.
 */

class FileTreeView : public QTreeWidget
{
    Q_OBJECT

public:
    FileTreeView(QWidget *parent);
    virtual ~FileTreeView();

    /**
     * @return the selected item.
     */
    FileTreeViewItem *selectedFileTreeViewItem() const;
    const KFileItem *selectedFileItem() const;
    KUrl selectedUrl() const;

    /**
     * @return the highlighted item.
     */
    FileTreeViewItem *highlightedFileTreeViewItem() const;
    const KFileItem *highlightedFileItem() const;
    KUrl highlightedUrl() const;

    /**
     *  Adds a branch to the treeview.
     *
     *  This high-level function creates the branch, adds it to the treeview and
     *  connects some signals. Note that directory listing does not start until
     *  a branch is expanded either by opening the root item by user or by setOpen
     *  on the root item.
     *
     *  @returns a pointer to the new branch or zero
     *  @param path is the base url of the branch
     *  @param name is the name of the branch, which will be the text for column 0
     *  @param showHidden says if hidden files and directories should be visible
     */
    FileTreeBranch *addBranch(const KUrl &path, const QString &name, bool showHidden = false);

    /**
     *  same as the function above but with a pixmap to set for the branch.
     */
    // TODO: why is this and next virtual while the above is not?
    virtual FileTreeBranch *addBranch(const KUrl &path, const QString &name,
                                      const QIcon &pix, bool showHidden = false);

    /**
     *  same as the function above but letting the user create the branch.
     */
    virtual FileTreeBranch *addBranch(FileTreeBranch *branch);

    /**
     *  removes the branch from the treeview.
     *  @param branch is a pointer to the branch
     *  @returns true on success.
     */
    virtual bool removeBranch(FileTreeBranch *branch);

    /**
     *  @returns a pointer to the named branch in the treeview or NULL on failure.
     *  @param searchName is the name of a branch
     */
    FileTreeBranch *branch(const QString &searchName);

    /**
     *  @returns a list of pointers to all existing branches in the treeview.
     **/
    FileTreeBranchList &branches();

    /**
     *  set the directory mode for branches. If true is passed, only directories will be loaded.
     *  @param branch is a pointer to a KFileTreeBranch
     */
    virtual void setDirOnlyMode(FileTreeBranch *branch, bool dirsOnly);

    /**
     * Searches a branch for an item identified by a relative URL.
     * The branch's base URL is added to the relative path.
     * @returns a pointer to the item or NULL if the item does not exist
     * @param branch the branch to search in
     * @param relUrl relative URL
     */
    FileTreeViewItem *findItemInBranch(FileTreeBranch *branch, const QString &relUrl);

    /**
     * As above, differs only in the first parameter. Searches in the named
     * branch.
     * @param branch the name of the branch to search in
     * @param relUrl relative URL
     */
    FileTreeViewItem *findItemInBranch(const QString &branchName, const QString &relUrl);

    /**
     * @returns a flag indicating if extended folder pixmaps are displayed or not.
     */
    bool showFolderOpenPixmap() const;


public slots:
    /**
     * set the flag to show 'extended' folder icons on or off. If switched on, folders will
     * have an open folder pixmap displayed if their children are visible, and the standard
     * closed folder pixmap (from mimetype folder) if they are closed.
     * If switched off, the plain mime pixmap is displayed.
     * @param showIt = false displays mime type pixmap only
     */
    virtual void setShowFolderOpenPixmap(bool showIt = true);

protected:
    // TODO: port D&D
    /**
     * @returns true if we can decode the drag and support the action
     */
    //virtual bool acceptDrag(QDropEvent *ev) const;
    //virtual void contentsDragEnterEvent(QDragEnterEvent *ev);
    //virtual void contentsDragMoveEvent(QDragMoveEvent *ev);
    //virtual void contentsDragLeaveEvent(QDragLeaveEvent *ev);
    //virtual void contentsDropEvent(QDropEvent *ev);
    virtual QMimeData *mimeData(const QList<QTreeWidgetItem *> items) const;

protected slots:
    // TODO: does anything here need to be virtual?
    virtual void slotNewTreeViewItems(FileTreeBranch *branch,
                                      const FileTreeViewItemList &newItems);

    virtual void slotSetNextUrlToSelect(const KUrl &url);
    virtual void slotDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);

private slots:
    void slotExecuted(QTreeWidgetItem *item);
    void slotExpanded(QTreeWidgetItem *item);
    void slotCollapsed(QTreeWidgetItem *item);
    void slotDoubleClicked(QTreeWidgetItem *item);

    void slotSelectionChanged();

    void slotAutoOpenFolder();

    void slotOnItem(QTreeWidgetItem *item);

    void slotStartAnimation(FileTreeViewItem *item);
    void slotStopAnimation(FileTreeViewItem *item);

signals:
    void onItem(const QString &path);

    /* New signals if you like it ? */
    void dropped(QWidget *widget, QDropEvent *ev);
    void dropped(QWidget *widget, QDropEvent *ev, KUrl::List &urlList);
    void dropped(KUrl::List &urlList, KUrl &url);
    // The drop event allows to differentiate between move and copy
    void dropped(QWidget *widget, QDropEvent *ev, KUrl::List &urllist, KUrl &url);

    void dropped(QDropEvent *ev, QTreeWidgetItem *item);
    void dropped(FileTreeView *view, QDropEvent *ev, QTreeWidgetItem *item);
    void dropped(QDropEvent *ev, QTreeWidgetItem *item, QTreeWidgetItem *onto);
    void dropped(FileTreeView *view, QDropEvent *ev, QTreeWidgetItem *item, QTreeWidgetItem *onto);

    void fileRenamed(FileTreeViewItem *item, const QString &newName);

protected:
    KUrl m_nextUrlToSelect;

private:
    // Returns whether item is still a valid item in the tree
    bool isValidItem(QTreeWidgetItem *item);
    void clearTree();
    QIcon itemIcon(FileTreeViewItem *item) const;

    /* List that holds the branches */
    FileTreeBranchList m_branches;

    int m_busyCount;

    QPoint m_dragPos;
    bool m_bDrag;

    // Flag whether the folder should have open-folder pixmaps
    bool m_wantOpenFolderPixmaps;

    // The item that was current before the drag-enter event happened
    QTreeWidgetItem *m_currentBeforeDropItem;
    // The item we are moving the mouse over (during a drag)
    QTreeWidgetItem *m_dropItem;

    QIcon m_openFolderPixmap;
    QTimer *m_autoOpenTimer;

    class FileTreeViewPrivate;
    FileTreeViewPrivate *d;
};


#endif							// FILETREEVIEW_H
