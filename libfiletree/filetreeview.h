/* This file is part of the KDE Project			-*- mode:c++; -*-
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


#include <qtreewidget.h>

#include "filetreebranch.h"


class QTimer;


/**
 * @short A tree view on the file system.
 *
 * The view is able to handle more than one base URL, each represented
 * by a FileTreeBranch.  The branches and subdirectories are automatically
 * listed and populated when they are expanded.
 *
 * The typical usage is:
 *
 * 1. Create and layout a FileTreeView.
 *
 * 2. Call addBranch() to create one or more branches.
 *
 * 3. Retrieve the root item with KFileTreeBranch::root(), and expand it
 *    if desired.  That starts the listing.
 *
 * @author David Faure
 * @author Carsten Pfeiffer
 * @author Klaas Freitag
 * @author Jonathan Marten
 **/

class FileTreeView : public QTreeWidget
{
    Q_OBJECT

public:
    /**
     * Create a new view widget.
     *
     * @param parent Parent widget
     *
     * @note The default is for the view to not accept either drags or
     * drops.  The caller must use @c setAcceptDrops(true) for drops to be
     * accepted, and @c setDragEnabled(true) for drags to be accepted.
     **/
    FileTreeView(QWidget *parent = NULL);

    /**
     * Destructor.  All of the branches are automatically deleted.
     **/
    virtual ~FileTreeView();

    /**
     * Get the currently selected item.
     *
     * @return the selected item,
     * or @c NULL if there is no selected item.
     **/
    FileTreeViewItem *selectedFileTreeViewItem() const;

    /**
     * Get the KFileItem (file information) for the currently selected item.
     *
     * @return the @c KFileItem for the selected item,
     * or @c NULL if there is no selected item.
     **/
    const KFileItem *selectedFileItem() const;

    /**
     * Get the URL of for the currently selected item.
     *
     * @return the URL of the selected item,
     * or an invalid URL if there is no selected item.
     **/
    KUrl selectedUrl() const;

    /**
     * Get the currently highlighted item.
     *
     * @return the highlighted item,
     * or @c NULL if there is no highlighted item.
     **/
    FileTreeViewItem *highlightedFileTreeViewItem() const;

    /**
     * Get the KFileItem (file information) for the currently highlighted item.
     *
     * @return the @c KFileItem for the highlighted item,
     * or @c NULL if there is no highlighted item.
     **/
    const KFileItem *highlightedFileItem() const;

    /**
     * Get the URL of for the currently highlighted item.
     *
     * @return the URL of the highlighted item,
     * or an invalid URL if there is no highlighted item.
     **/
    KUrl highlightedUrl() const;

    /**
     * Add a branch to the view.
     *
     * This creates a branch for the specified @p path and
     * adds it to the tree view.
     *
     * @param path base URL of the branch
     * @param name name of the branch (the text for column 0)
     * @param showHidden if @c true, hidden files and directories will be visible
     * @return the new branch, or @c NULL if the branch could not be created
     *
     * @note No directory listing starts until the branch is expanded,
     * either by the user opening the root item or by calling @c setExpanded()
     * on the root item.
     **/
    virtual FileTreeBranch *addBranch(const KUrl &path, const QString &name, bool showHidden = false);

    /**
     * Add a branch to the view, with a specified pixmap.
     *
     * This creates a branch for the specified @p path and
     * adds it to the tree view.
     *
     * @param path base URL of the branch
     * @param name name of the branch (the text for column 0)
     * @param pix pixmap for the branch
     * @param showHidden if @c true, hidden files and directories will be visible
     * @return the new branch, or @c NULL if the branch could not be created
     **/
    virtual FileTreeBranch *addBranch(const KUrl &path, const QString &name,
                                      const QIcon &pix, bool showHidden = false);

    /**
     * Add a user created branch to the view.
     *
     * @param branch the branch.  The tree view takes ownership of this branch,
     * and will delete it when the tree view is destroyed.
     *
     * @return the specified branch
     **/
    virtual FileTreeBranch *addBranch(FileTreeBranch *branch);

    /**
     * Remove a branch from the view.
     *
     * @param branch the branch to remove
     * @return @c true if the branch was successfully removed
     *
     * @note The @p branch is not automatically deleted.
     **/
    virtual bool removeBranch(FileTreeBranch *branch);

    /**
     * Search for a branch by name.
     *
     * @param searchName the name of a branch to search for.
     * @return the named branch, or @c NULL if the branch was not found.
     **/
    FileTreeBranch *branch(const QString &searchName) const;

    /**
     * Get a list of all existing branches.
     *
     * @return a list of all existing branches in the view
     **/
    const FileTreeBranchList &branches() const;

    /**
     * Set the directory-only mode for a branch.
     *
     * @param branch the branch to set
     * @param dirsOnly if @c true, only directories will be shown.
     *
     * @see KDirLister::setDirOnlyMode
     **/
    virtual void setDirOnlyMode(FileTreeBranch *branch, bool dirsOnly);

    /**
     * Search a specified branch for an item identified by a relative URL.
     *
     * @param branch the branch to search in
     * @param relUrl the URL path to search for, relative to the root URL of the branch
     * @return the found item, or @c NULL if nothing was found
     **/
    FileTreeViewItem *findItemInBranch(FileTreeBranch *branch, const QString &relUrl) const;

    /**
     * Search a named branch for an item identified by a relative URL.
     *
     * @param branchName the name of the branch to search in
     * @param relUrl the URL path to search for, relative to the root URL of the branch
     * @return the found item, or @c NULL if nothing was found
     **/
    FileTreeViewItem *findItemInBranch(const QString &branchName, const QString &relUrl) const;

    /**
     * Get the flag indicating whether extended folder pixmaps are displayed.
     *
     * @return the extended folder pixmap option
     * @see setShowFolderOpenPixmap()
     **/
    bool showFolderOpenPixmap() const;

public slots:
    /**
     * Set the flag to show 'extended' folder icons.
     *
     * If set on, folders will have an "open folder" pixmap displayed
     * if their children are visible, and the standard "closed folder" pixmap
     * if they are closed. If set off, the plain "folder" pixmap is displayed.
     *
     * The default is @c true.
     *
     * @param showIt the new option setting
     * @see showFolderOpenPixmap()
     **/
    virtual void setShowFolderOpenPixmap(bool showIt = true);

protected:
    /**
     * @reimp
     * @see QTreeWidget::mimeData()
     **/
    virtual QMimeData *mimeData(const QList<QTreeWidgetItem *> items) const;

    /**
     * @reimp
     * @see QAbstractItemView::dragEnterEvent()
     **/
    virtual void dragEnterEvent(QDragEnterEvent *ev);

    /**
     * @reimp
     * @see QTreeView::dragMoveEvent()
     **/
    virtual void dragMoveEvent(QDragMoveEvent *ev);

    /**
     * @reimp
     * @see QAbstractItemView::dragLeaveEvent()
     **/
    virtual void dragLeaveEvent(QDragLeaveEvent *ev);

    /**
     * @reimp
     * @see QTreeWidget::dropEvent()
     **/
    virtual void dropEvent(QDropEvent *ev);

protected slots:
    /**
     * New items have been added to or have appeared in a branch.
     *
     * @param branch the branch containing the new items
     * @param newItems a list of the new items
     **/
    void slotNewTreeViewItems(FileTreeBranch *branch,
                              const FileTreeViewItemList &newItems);

    /**
     * Set the URL of the item that will be selected after new
     * items have been added.
     *
     * @param url the URL of the item to select
     **/
    void slotSetNextUrlToSelect(const KUrl &url);

    /**
     * Model data has changed.  Used to detect the renaming of an item.
     *
     * @see QAbstractItemModel::dataChanged()
     **/
    void slotDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);

signals:
    /**
     * The mouse has been moved over an item.
     *
     * @param path The path of the item.  If the branch root is local, then
     * the file pathname of the item is sent.  Otherwise, the pretty URL
     * is sent.
     *
     * @see QTreeWidget::itemEntered()
     **/
    void onItem(const QString &path);

    /**
     * Something has been dropped onto the view.
     *
     * A move or copy drag can be distinguished by examining the
     * event's drop action.
     *
     * @param ev the drop event data
     * @param item the item over which the drop was released
     *
     * @note Only drops of URLs are accepted (@c ev->mimeData()->hasUrls()
     * must be @c true).
     **/
    void dropped(QDropEvent *ev, FileTreeViewItem *item);

    /**
     * An item has been renamed.
     *
     * @param item the renamed item
     * @param newName the new name of the item
     **/
    void fileRenamed(FileTreeViewItem *item, const QString &newName);

private slots:
    void slotExecuted(QTreeWidgetItem *item);
    void slotExpanded(QTreeWidgetItem *item);
    void slotCollapsed(QTreeWidgetItem *item);
    void slotDoubleClicked(QTreeWidgetItem *item);

    void slotSelectionChanged();

    void slotOnItem(QTreeWidgetItem *item);

    void slotAutoOpenFolder();
    void slotStartAnimation(FileTreeViewItem *item);
    void slotStopAnimation(FileTreeViewItem *item);

private:
    QIcon itemIcon(FileTreeViewItem *item) const;
    void setDropItem(QTreeWidgetItem *item);

    FileTreeBranchList m_branches;			// list of the branches
    int m_busyCount;					// branches currently listing
    KUrl m_nextUrlToSelect;				// next item to select
    bool m_wantOpenFolderPixmaps;			// want open-folder pixmaps?

    QTreeWidgetItem *m_currentBeforeDropItem;		// current item before drag
    QTreeWidgetItem *m_dropItem;			// item mouse is over

    QIcon m_openFolderPixmap;				// pixmap for open folders
    QTimer *m_autoOpenTimer;				// timer for auto open
};


#endif							// FILETREEVIEW_H
