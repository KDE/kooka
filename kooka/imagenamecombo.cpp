/***************************************************************************
                          imgnamecombo.cpp - combobox for image names 
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

#include "imagenamecombo.h"
#include "imagenamecombo.moc"

#include <kdebug.h>
#include <klocale.h>
#include <kcombobox.h>
#include <k3filetreeview.h>
#include <kfiletreebranch.h>



#define GALLERY_PATH_SEP	" - "


ImageNameCombo::ImageNameCombo(QWidget *parent)
    : KComboBox(parent)
{
    connect(this,SIGNAL(activated(const QString &)),SLOT(slotActivated(const QString &)));
}


QString itemName(KFileTreeBranch *branch,const QString &relPath)
{
    K3FileTreeView *view = static_cast<K3FileTreeView *>(branch->root()->listView());
    if (view==NULL) return (relPath);

    if (relPath=="/") return (branch->name());		// root => gallery name
    if (view->branches().count()<=1) return (relPath);	// only one gallery => path only
    return (branch->name()+GALLERY_PATH_SEP+relPath);	// gallery and path within it
}


void ImageNameCombo::slotPathRemoved(KFileTreeBranch *branch,const QString &relPath)
{
    QString removedEntry = itemName(branch,relPath);
    kDebug() << "removing " << removedEntry;

    if (!items.contains(removedEntry)) return;		// nothing to do

    QString select = currentText();			// save current selection
    items.removeOne(removedEntry);
    setCurrentIndex(items.indexOf(select));		// restore current selection

    // Having done the above, the original code used to call rewriteList()
    // which did something like:
    //
    //   clear();
    //   for ( QStringList::Iterator it = items.begin(); it != items.end(); ++it )
    //   {
    //     insertItem( branch->pixmap(), *it );
    //   }
    //
    // I can't work out what that was supposed to do, if indeed it did anything!
    // After the clear(), 'items' would have been an empty list so the loop would
    // never have been executed...
}


void ImageNameCombo::slotPathChanged(KFileTreeBranch *branch,const QString &relPath)
{
    QString newEntry = itemName(branch,relPath);
    kDebug() << "inserting " << newEntry;

    setCurrentItem(newEntry,true,0);			// insert at top if missing
}


void ImageNameCombo::slotActivated(const QString &itemText)
{
    QString branchName = QString::null;
    QString relPath = itemText;

    int ix = relPath.indexOf(GALLERY_PATH_SEP);		// is the separator present?
    if (ix>0)						// (multiple gallery roots)
    {
        branchName = relPath.left(ix);			// split into root and path
        relPath = relPath.mid(ix+3);
    }

    if (!relPath.endsWith("/")) relPath = "/";		// it's the root
    emit pathSelected(branchName,relPath);
}
