
/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
                 2000 Carsten Pfeiffer <pfeiffer@kde.org>
                 2001 Klaas Freitag <freitag@suse.de>

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

#ifndef FILETREEBRANCH_H
#define FILETREEBRANCH_H

#include <qicon.h>
#include <qhash.h>
#include <qurl.h>

#include <kfileitem.h>
#include <kdirlister.h>

#include <kio/global.h>
#include <kio/job.h>

#include "filetreeviewitem.h"

class FileTreeView;

/**
 * This is the branch class of the FileTreeView, which represents one
 * branch in the treeview. Every branch has a root which is an url. The branch
 * lists the files under the root. Every branch uses its own dirlister and can
 * have its own filter etc.
 *
 * @short Branch object for FileTreeView object.
 *
 */

class FileTreeBranch : public KDirLister
{
    Q_OBJECT

public:
    /**
     * constructs a branch for FileTreeView. Does not yet start to list it.
     * @param url start url of the branch.
     * @param name the name of the branch, which is displayed in the first column of the treeview.
     * @param pix is a pixmap to display as an icon of the branch.
     * @param showHidden flag to make hidden files visible or not.
     * @param branchRoot is the FileTreeViewItem to use as the root of the
     *        branch, with the default NULL meaning to let FileTreeBranch create
     *        it for you.
     */
    FileTreeBranch(FileTreeView *parent, const QUrl &url, const QString &name,
                   const QIcon &pix, bool showHidden = false,
                   FileTreeViewItem *branchRoot = NULL);

    /**
     * @returns the root url of the branch.
     */
    QUrl rootUrl() const;

    /**
     * sets a FileTreeViewItem as root widget for the branch.
     * That must be created outside of the branch. All FileTreeViewItems
     * the branch is allocating will become children of that object.
     * @param r the FileTreeViewItem to become the root item.
     */
    virtual void setRoot(FileTreeViewItem *r);

    /**
     * @returns the root item.
     */
    FileTreeViewItem *root() const;

    /**
     * returns and sets the name of the branch.
     */
    QString name() const;
    virtual void setName(const QString &newName);

    /*
     * returns the current root item pixmap set in the constructor. The root
     * item pixmap defaults to the icon for directories.
     * @see openPixmap()
     */
    QIcon pixmap() const;

    /*
     * returns the current root item pixmap set by setOpenPixmap()
     * which is displayed if the branch is expanded.
     * The root item pixmap defaults to the icon for directories.
     * @see pixmap()
     * Note that it depends on FileTreeView::showFolderOpenPximap weather
     * open pixmap are displayed or not.
     */
    QIcon openPixmap() const;
    void setOpenPixmap(const QIcon &pix);

    /**
     * @returns whether the items in the branch show their file extensions in the
     * tree or not. See setShowExtensions for more information.
     * sets printing of the file extensions on or off. If you pass false to this
     * slot, all items of this branch will not show their file extensions in the
     * tree.
     * @param visible flags if the extensions should be visible or not.
     */
    bool showExtensions() const;
    virtual void setShowExtensions(bool visible = true);

    /**
     * sets the root of the branch open or closed.
     */
    void setOpen(bool open = true);

    /**
     * sets if children recursion is wanted or not. If this is switched off, the
     * child directories of a just opened directory are not listed internally.
     * That means that it can not be determined if the sub directories are
     * expandable or not. If this is switched off there will be no call to
     * setExpandable.
     * @param t set to true to switch on child recursion
     */
    bool childRecurse();
    void setChildRecurse(bool t = true);

    /**
     * find the according FileTreeViewItem for a url
     */
    virtual FileTreeViewItem *findItemByUrl(const QUrl &url);

    virtual void itemRenamed(FileTreeViewItem *item);

public slots:
    /**
     * populates a branch. This method must be called after a branch was added
     * to  a FileTreeView using method addBranch.
     * @param url is the url of the root item where the branch starts.
     * @param currItem is the current parent.
     */
    virtual bool populate(const QUrl &url, FileTreeViewItem *currItem);

protected:
    /**
     * allocates a FileTreeViewItem for a new item in the branch.
     */
    virtual FileTreeViewItem *createTreeViewItem(FileTreeViewItem *parent,
            const KFileItem &fileItem);

signals:
    /**
     * indicates start/finish lister activity
     */
    void populateStarted(FileTreeViewItem *item);
    void populateFinished(FileTreeViewItem *item);

    /**
     * emitted with a list of new or updated FileTreeViewItem's which were
     * found in a branch. Note that this signal is emitted very often and
     * may slow down the performance of the treeview!
     */
    void newTreeViewItems(FileTreeBranch *branch, const FileTreeViewItemList &items);
    void changedTreeViewItems(FileTreeBranch *branch, const FileTreeViewItemList &items);

    /**
     * emitted with the exact count of children for a directory.
     */
    void directoryChildCount(FileTreeViewItem *item, int count);

private slots:
    void slotRefreshItems(const QList<QPair<KFileItem, KFileItem> > &items);
    void slotListerCompleted(const QUrl &url);
    void slotListerCanceled(const QUrl &url);
    void slotListerStarted(const QUrl &url);
    void slotListerClear();
    void slotListerClearUrl(const QUrl &url);
    void slotRedirect(const QUrl &oldUrl, const QUrl &newUrl);
    void slotItemsDeleted(const KFileItemList &items);
    void slotItemsAdded(const QUrl &parent, const KFileItemList &items);

private:
    void itemDeleted(const KFileItem *fi);

    static void deleteChildrenOf(QTreeWidgetItem *parent);

    FileTreeViewItem *m_root;
    QUrl m_startURL;
    QString m_name;
    QIcon m_rootIcon;
    QIcon m_openRootIcon;

    /* this list holds the url's which children are opened. */
    QList<QUrl> m_openChildrenURLs;

    /* Used for caching purposes in findItemByURL() */
    QUrl m_lastFoundUrl;
    FileTreeViewItem *m_lastFoundItem;

    bool m_recurseChildren;
    bool m_showExtensions;

    QHash<QUrl, FileTreeViewItem *> m_itemMap;
};

/**
 * List of KFileTreeBranches
 */
typedef QList<FileTreeBranch *> FileTreeBranchList;

#endif                          // FILETREEBRANCH_H
