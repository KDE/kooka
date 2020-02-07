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

class QUrl;

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
    ~FileTreeViewItem() = default;

    /**
     * @return the KFileTreeBranch the item is sorted in.
     */
    FileTreeBranch *branch() const
    {
        return (m_branch);
    }

    /**
     * @return the KFileItem the viewitem is representing.
     * A copy of the original (provided by the KDirLister), so there is not
     * much point in trying to modify it.
     */
    const KFileItem *fileItem() const
    {
        return (&m_kfileitem);
    }

    /**
     * @return the path of the item
     */
    // TODO: is this simply equivalent to 'url().path()'?
    QString path() const;

    /**
     * @return the item's URL
     */
    QUrl url() const;
    void setUrl(const QUrl &url);

    /**
     * @return if the item represents a directory
     */
    bool isDir() const;

    /**
     * @return if the item represents the root of its branch
     */
    bool isRoot() const;

    /**
     * @return if this directory has already been seen by a KDirLister
     */
    bool alreadyListed() const;

    /**
     * set the flag to indicate if the directory has already been listed
     */
    void setListed(bool wasListed);

private:
    void init(const KFileItem &fi, FileTreeBranch *branch);

    KFileItem m_kfileitem;
    FileTreeBranch *m_branch;
    bool m_wasListed;
};

/**
 * List of KFileTreeViewItems
 */
typedef QList<FileTreeViewItem *> FileTreeViewItemList;

#endif							// FILETREEVIEWITEM_H
