/***************************************************************************
               thumbview.cpp  - Class to display thumbnailed images
                             -------------------
    begin                : Tue Apr 18 2002
    copyright            : (C) 2002 by Klaas Freitag
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

#include "thumbview.h"
#include "thumbview.moc"

#include <qsignalmapper.h>

#include <kfileitem.h>
#include <k3filetreeviewitem.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kaction.h>
#include <kactionmenu.h>
#include <kdiroperator.h>
#include <klocale.h>
#include <kdebug.h>
#include <kmenu.h>
#include <kconfiggroup.h>

#include "libkscan/previewer.h"

//#include "thumbviewdiroperator.h"


// TODO: does this need to be a KVBox containing the DirOperator?
// can it not just be the DirOperator itself?
ThumbView::ThumbView(QWidget *parent)
    : KVBox(parent)
{
    setObjectName("ThumbView");

    setMargin(3);
    m_thumbSize = KIconLoader::SizeHuge;
    m_bgImg = QString::null;
    m_firstMenu = true;					// first time menu used

    // TODO: does there need to be a special ThumbViewDirOperator?
    // The setupMenu() below should remove all of the standard actions
    // from the menu.
    //m_dirop = new ThumbViewDirOperator(KUrl(Previewer::galleryRoot()), this);
    m_dirop = new KDirOperator(KUrl(Previewer::galleryRoot()), this);
    m_dirop->setObjectName("ThumbViewDirOperator");
    m_dirop->setView(KFile::Default);
    m_dirop->setView(KFile::PreviewContents);
    m_dirop->setPreviewWidget(NULL);			// no preview at side
    m_dirop->setupMenu(0);				// no standard menu entries

    //m_fileview->setViewName(i18n("Thumbnails"));
    //m_fileview->setSelectionMode(KFile::Single);
    m_dirop->setMode(KFile::File);			// implies single selection mode
    // TODO: is there an equivalent?
    //m_fileview->setIgnoreMaximumSize(true);
    m_dirop->setInlinePreviewShown(true);

    connect(m_dirop, SIGNAL(fileHighlighted(const KFileItem &)),
            SLOT(slotFileSelected(const KFileItem &)));
    connect(m_dirop, SIGNAL(contextMenuAboutToShow(const KFileItem &, QMenu *)),
            SLOT(slotAboutToShowMenu(const KFileItem &, QMenu *)));

    connect(m_dirop,SIGNAL(finishedLoading()),SLOT(slotFinishedLoading()));

    m_lastSelected = m_dirop->url();
    m_toSelect = QString::null;

    readSettings();

    m_sizeMenu = new KActionMenu(i18n("Preview Size"),this);
    QSignalMapper *mapper = new QSignalMapper(this);
    KToggleAction *act;

    act = new KToggleAction(sizeName(KIconLoader::SizeEnormous), this);
    connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
    mapper->setMapping(act,KIconLoader::SizeEnormous);
    m_sizeMap[KIconLoader::SizeEnormous] = act;
    m_sizeMenu->addAction(act);

    act = new KToggleAction(sizeName(KIconLoader::SizeHuge), this);
    connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
    mapper->setMapping(act,KIconLoader::SizeHuge);
    m_sizeMap[KIconLoader::SizeHuge] = act;
    m_sizeMenu->addAction(act);

    act = new KToggleAction(sizeName(KIconLoader::SizeLarge), this);
    connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
    mapper->setMapping(act,KIconLoader::SizeLarge);
    m_sizeMap[KIconLoader::SizeLarge] = act;
    m_sizeMenu->addAction(act);

    act = new KToggleAction(sizeName(KIconLoader::SizeMedium), this);
    connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
    mapper->setMapping(act,KIconLoader::SizeMedium);
    m_sizeMap[KIconLoader::SizeMedium] = act;
    m_sizeMenu->addAction(act);

    act = new KToggleAction(sizeName(KIconLoader::SizeSmallMedium), this);
    connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
    mapper->setMapping(act,KIconLoader::SizeSmallMedium);
    m_sizeMap[KIconLoader::SizeSmallMedium] = act;
    m_sizeMenu->addAction(act);

    connect(mapper,SIGNAL(mapped(int)),SLOT(slotSetSize(int)));
//    connect(contextMenu(),SIGNAL(aboutToShow()),SLOT(slotAboutToShowMenu()));

    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}





ThumbView::~ThumbView()
{
    saveConfig();
}



QSize ThumbView::sizeHint() const
{
    return (QSize(64, 64));				// sensible minimum size
}



//KMenu *ThumbView::contextMenu() const
//{
//    return (m_dirop->contextMenu());
//}


void ThumbView::slotAboutToShowMenu(const KFileItem &kfi, QMenu *menu)
{
    // TODO: is this necessary?
    slotFileSelected(kfi);

    if (m_sizeMenu==NULL) return;
    kDebug();

    if (m_firstMenu)					// popup for the first time
    {
        //KMenu *menu = contextMenu();
        menu->addSeparator();
        menu->addAction(m_sizeMenu);			// append size menu at end
        m_firstMenu = false;				// don't do this again
    }

    for (QMap<KIconLoader::StdSizes, KToggleAction *>::const_iterator it = m_sizeMap.constBegin();
         it!=m_sizeMap.constEnd(); ++it)		// tick applicable size entry
    {
        (*it)->setChecked(m_thumbSize==it.key());
    }
}


void ThumbView::slotSetSize(int size)
{
    kDebug() << "size" << size;
    m_thumbSize = static_cast<KIconLoader::StdSizes>(size);
    // TODO: what's the equivalent, is it m_dirop->view()->seticonSize()
    //m_fileview->setPreviewSize(m_thumbSize);
}


void ThumbView::slotFinishedLoading()
{
    if (m_toSelect.isNull()) return;			// nothing to do

    m_dirop->blockSignals(true);			// avoid signal loop
    m_dirop->setCurrentItem(m_toSelect);
    m_dirop->blockSignals(false);
    m_toSelect = QString::null;				// have dealt with this now
}


void ThumbView::slotFileSelected(const KFileItem &kfi)
{
    KUrl url = (!kfi.isNull() ? kfi.url() : m_dirop->url());
    kDebug() << url.prettyUrl();

    if (url!=m_lastSelected)
    {
        m_lastSelected = url;
        emit selectFromThumbnail(url);
    }
}


void ThumbView::slotSelectImage(const K3FileTreeViewItem *item)
{
    kDebug() << "item url" << item->url().prettyUrl() << "isDir" << item->isDir();

    KUrl cur = m_dirop->url();				// directory currently showing

    KUrl urlToShow = item->url();			// new URL to show
    KUrl dirToShow = urlToShow;				// directory part of that
    if (!item->isDir()) dirToShow.setFileName(QString::null);

    if (cur.path(KUrl::AddTrailingSlash)!=dirToShow.path(KUrl::AddTrailingSlash))
    {							// see if changing path
        if (!item->isDir()) m_toSelect = urlToShow.fileName();
							// select that when loading finished
        m_dirop->setUrl(dirToShow,true);		// change path and reload
        return;
    }

    KFileItemList selItems = m_dirop->selectedItems();
    if (!selItems.isEmpty())				// the current selection
    {
        KFileItem curItem = selItems.first();
        if (curItem.url()==urlToShow) return;		// already selected
    }

    m_dirop->blockSignals(true);			// avoid signal loop
    m_dirop->setCurrentItem(item->isDir() ? QString::null : urlToShow.fileName());
    m_dirop->blockSignals(false);
}


bool ThumbView::readSettings()
{
    KConfigGroup grp = KGlobal::config()->group(THUMB_GROUP);
    bool changed = false;

    KIconLoader::StdSizes size = static_cast<KIconLoader::StdSizes>(grp.readEntry(THUMB_PREVIEW_SIZE,
                                                                          static_cast<int>(KIconLoader::SizeHuge)));
    if (size!=m_thumbSize)
    {
        slotSetSize(size);
        changed = true;
    }

    QString newBgImg = grp.readEntry(THUMB_BG_WALLPAPER, standardBackground());
    if (newBgImg!=m_bgImg )
    {
        m_bgImg = newBgImg;
        setBackground();
        changed = true;
    }

    return (changed);
}


void ThumbView::saveConfig()
{
    // Do nothing, preview size set by menu is for this session only.
    // Set the default size in Kooka Preferences.
}


void ThumbView::setBackground()
{
    QPixmap bgPix;

    if (!m_bgImg.isEmpty()) bgPix.load(m_bgImg);
    else
    {
        bgPix = QPixmap(16,16);
        bgPix.fill(Qt::blue);
    }

    QPalette pal = m_dirop->palette();
    pal.setBrush(m_dirop->backgroundRole(), QBrush(bgPix));
    m_dirop->setPalette(pal);
}


void ThumbView::slotImageChanged(const KFileItem *kfi)
{
    kDebug() << kfi->url().prettyUrl();
    // TODO: is there an equivalent?
    //m_fileview->updateView(kfi);			// update that view item
}


void ThumbView::slotImageRenamed(const K3FileTreeViewItem *item,const QString &newName)
{
    kDebug() << item->url().prettyUrl() << "->" << newName;

    if (item->isDir()) return;				// directory rename, nothing to do

    KUrl cur = m_dirop->url();				// check it's the one currently showing
    if (item->url().directory(KUrl::AppendTrailingSlash)!=cur.path(KUrl::AddTrailingSlash)) return;

    m_toSelect = newName;				// select new after update
    // TODO: is there an equivalent?
    //m_fileview->updateView(item->fileItem());		// update that view item
}


void ThumbView::slotImageDeleted(const KFileItem *kfi)
{
    // No need to do anything, KDirOperator/KFileView handles all of that
}


QString ThumbView::standardBackground()
{
    return (KGlobal::dirs()->findResource("data",THUMB_STD_TILE_IMG));
}


QString ThumbView::sizeName(KIconLoader::StdSizes size)
{
    switch (size)
    {
case KIconLoader::SizeEnormous:		return (i18n("Very Large"));
case KIconLoader::SizeHuge:		return (i18n("Large"));
case KIconLoader::SizeLarge:		return (i18n("Medium"));
case KIconLoader::SizeMedium:		return (i18n("Small"));
case KIconLoader::SizeSmallMedium:	return (i18n("Very Small"));
case KIconLoader::SizeSmall:		return (i18n("Tiny"));
default:				return ("?");
    }
}
