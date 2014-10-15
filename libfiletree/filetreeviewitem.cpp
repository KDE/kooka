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

#include "filetreeviewitem.h"

#include <QDebug>
#include <kfileitem.h>
#include <kiconloader.h>

#include "filetreeview.h"

FileTreeViewItem::FileTreeViewItem(FileTreeViewItem *parent,
                                   const KFileItem &fi,
                                   FileTreeBranch *branch)
    : QTreeWidgetItem(parent)
{
    init(fi, branch);
    setFlags(flags() | Qt::ItemIsEditable);     // non-top level items are editable
}

FileTreeViewItem::FileTreeViewItem(FileTreeView *parent,
                                   const KFileItem &fi,
                                   FileTreeBranch *branch)
    : QTreeWidgetItem(parent)
{
    init(fi, branch);
}                           // top level item not editable

void FileTreeViewItem::init(const KFileItem &fi, FileTreeBranch *branch)
{
    m_kfileitem = fi;
    m_branch = branch;
    m_wasListed = false;
    m_clientData = NULL;

    //QT5 setIcon(0, fi.pixmap(KIconLoader::SizeSmall));
    setText(0, fi.text());
}

FileTreeViewItem::~FileTreeViewItem()
{
}

bool FileTreeViewItem::alreadyListed() const
{
    return (m_wasListed);
}

void FileTreeViewItem::setListed(bool wasListed)
{
    m_wasListed = wasListed;
}

KUrl FileTreeViewItem::url() const
{
    return (!m_kfileitem.isNull() ? m_kfileitem.url() : KUrl());
}

void FileTreeViewItem::setUrl(const KUrl &url)
{
    if (!m_kfileitem.isNull()) {
        m_kfileitem.setUrl(url);
    }
}

QString FileTreeViewItem::path() const
{
    return (!m_kfileitem.isNull() ? m_kfileitem.url().path() : QString());
}

bool FileTreeViewItem::isDir() const
{
    return (!m_kfileitem.isNull() ? m_kfileitem.isDir() : false);
}

bool FileTreeViewItem::isRoot() const
{
    return (this == m_branch->root());
}

void FileTreeViewItem::setClientData(void *data)
{
    m_clientData = data;
}

void *FileTreeViewItem::clientData() const
{
    return (m_clientData);
}
