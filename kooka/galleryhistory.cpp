/***************************************************************************
                          galleryhistory.cpp - combobox for gallery folder history
                          -------------------
    begin                : Tue Nov 13 2001
    copyright            : (C) 2001 by Klaas Freitag
    email                : freitag@suse.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This file may be distributed and/or modified under the terms of the    *
 *  GNU General Public License version 2 as published by the Free Software *
 *  Foundation and appearing in the file COPYING included in the           *
 *  packaging of this file.                                                *
 *
 *  As a special exception, permission is given to link this program       *
 *  with any version of the KADMOS ocr/icr engine of reRecognition GmbH,   *
 *  Kreuzlingen and distribute the resulting executable without            *
 *  including the source code for KADMOS in the source distribution.       *
 *
 *  As a special exception, permission is given to link this program       *
 *  with any edition of Qt, and distribute the resulting executable,       *
 *  without including the source code for Qt in the source distribution.   *
 *                                                                         *
 ***************************************************************************/

#include "galleryhistory.h"

#include <QDebug>
#include <klocale.h>
#include <kcombobox.h>

#include "libfiletree/filetreeview.h"
#include "libfiletree/filetreebranch.h"

#define GALLERY_PATH_SEP    " - "

GalleryHistory::GalleryHistory(QWidget *parent)
    : KComboBox(parent)
{
    connect(this, SIGNAL(activated(int)), SLOT(slotActivated(int)));
    setEnabled(false);                  // no items added yet
}

// Data used for retrieving branch name and path - fixed format
static QString entryData(const FileTreeBranch *branch, const QString &relPath)
{
    return (branch->name() + GALLERY_PATH_SEP + relPath);
}

// Data for display - what the user sees
static QString entryName(const FileTreeBranch *branch, const QString &relPath)
{
    QString name = QString::null;

    FileTreeView *view = static_cast<FileTreeView *>(branch->root()->treeWidget());
    if (view == NULL) {
        return (relPath);    // get view that this belongs to
    }

    if (view->branches().count() > 1 || relPath == "/") { // multiple galleries, or
        // at the branch root
        name = branch->name();              // start with branch name
        if (relPath != "/") {
            name += GALLERY_PATH_SEP;    // add separator if path
        }
    }

    if (relPath != "/") {
        name += relPath;                // not root, add relative name
        if (name.endsWith("/")) {
            name.chop(1);    // without trailing slash
        }
    }
    return (name);
}

void GalleryHistory::slotPathRemoved(const FileTreeBranch *branch, const QString &relPath)
{
    QString removeEntry = entryData(branch, relPath);
    //qDebug() << "removing" << removeEntry;

    int idx = findData(removeEntry);
    if (idx == -1) {
        return;    // not present, nothing to do
    }

    QString select = currentText();         // save current selection
    removeItem(idx);                    // remove that item
    setCurrentIndex(findText(select));          // restore current selection
    setEnabled(count() > 0);            // was the last removed?
}

void GalleryHistory::slotPathChanged(const FileTreeBranch *branch, const QString &relPath)
{
    QString newData = entryData(branch, relPath);
    //qDebug() << "inserting" << newData;

    int idx = findData(newData);            // see if exists already
    if (idx != -1) {
        setCurrentIndex(idx);    // if so, just select that
    } else {                    // if not already present,
        // insert at top
        insertItem(0, entryName(branch, relPath), newData);
        setCurrentIndex(0);
    }

    setEnabled(true);                   // now have at least 1 item
}

void GalleryHistory::slotActivated(int idx)
{
    QString branchName = QString::null;

    QString relPath = itemData(idx).toString();
    int ix = relPath.indexOf(GALLERY_PATH_SEP);     // is the separator present?
    if (ix > 0) {                   // (multiple gallery roots)
        branchName = relPath.left(ix);          // split into root and path
        relPath = relPath.mid(ix + 3);
    }

    if (!relPath.endsWith("/")) {
        relPath = "/";    // it's the root
    }
    emit pathSelected(branchName, relPath);
}
