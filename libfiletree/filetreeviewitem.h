/* This file is part of the KDE project
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

#ifndef FILETREEVIEWITEM_H
#define FILETREEVIEWITEM_H

#include <qtreewidget.h>

#include <kfileitem.h>
#include <kdirlister.h>

#include <kio/global.h>
#include <kio/job.h>

class KUrl;

class FileTreeView;
class FileTreeBranch;

/**
 * An item for a FileTreeView that knows about its own KFileItem.
 */
class FileTreeViewItem : public QTreeWidgetItem
{
public:
    FileTreeViewItem(FileTreeViewItem *parent, const KFileItem &fi, FileTreeBranch *branch);
    FileTreeViewItem(FileTreeView *parent, const KFileItem &fi, FileTreeBranch *branch);
    ~FileTreeViewItem();

    /**
     * @return the KFileTreeBranch the item is sorted in.
     */
    FileTreeBranch *branch() const
    {
        return (m_branch);
    }

    /**
     * @return the KFileItem the viewitem is representing.
     * A copy of the original (provided by the KDirLister), so not
     * much point in trying to modify it.
     */
    const KFileItem *fileItem() const
    {
        return (&m_kfileitem);
    }

    /**
     * @return the path of the item.
     */
    // TODO: is this simply equivalent to 'url().path()'?
    QString path() const;

    /**
     * @return the items KUrl
     */
    KUrl url() const;
    void setUrl(const KUrl &url);

    /**
     * @return if the item represents a directory
     */
    bool isDir() const;

    /**
     * @return if the item represents the root of its branch
     */
    bool isRoot() const;

    /**
     * @return if this directory was already seen by a KDirLister.
     */
    bool alreadyListed() const;

    /**
     * set the flag if the directory was already listed.
     */
    void setListed(bool wasListed);

    /**
     * substitute for the KFileItem's extra data, but only one of these
     */
    void setClientData(void *data);
    void *clientData() const;

private:
    void init(const KFileItem &fi, FileTreeBranch *branch);

    KFileItem m_kfileitem;
    FileTreeBranch *m_branch;
    bool m_wasListed;
    void *m_clientData;

    class FileTreeViewItemPrivate;
    FileTreeViewItemPrivate *d;
};

/**
 * List of KFileTreeViewItems
 */
typedef QList<FileTreeViewItem *> FileTreeViewItemList;

#endif                          // FILETREEVIEWITEM_H
