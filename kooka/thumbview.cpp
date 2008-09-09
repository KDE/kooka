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


/*
    Some most useful original comments retained here for reference,
    probably not relevant now that we are using kio/kfile to do all
    the hard work.

    From a mail from Waldo pointing out two probs in Thumbview:

    1) KDirLister is the owner of the KFileItems it emits, this means
       that you must watch it's deleteItem() signal vigourously,
       otherwise you may end up with KFileItems that are already
       deleted. This burden is propagated to classes that use
       KDirLister, such as KFileIconView.

       This has a tendency to go wrong in combination with PreviewJob,
       because it stores a list of KFileItems while running. This has
       the potential to crash if the fileitems are being deleted
       during this time. The remedy is to make sure to remove
       fileitems that get deleted from the PreviewJob with
       PreviewJob::removeItem.

    2) I think you may end up creating two PreviewJob's in parallel
       when the slNewFileItems() function is called two times in
       quick succession. The current code doesn't seem to expect
       that, given the comment in slPreviewResult(). In the light of
       (1) it might become fatal since you will not be able to call
       PreviewJob::removeItem on the proper job. I suggest to queue
       new items when a job is already running and start a new job
       once the first one is finished when there are any items left
       in the queue. Don't forget to delete items from the queue if
       they get deleted in the mean time.

       The strategy is as follows: In the global list m_pendingJobs
       the jobs to start are appended. Only if m_job is zero (no job
       is running) a job is started on the current m_pendingJobs list.
       The m_pendingJobs list is clear afterwords.
*/

#include <qsignalmapper.h>
#include <qpopupmenu.h>

#include <kfileitem.h>
#include <kfileiconview.h>
#include <kfiletreeviewitem.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kaction.h>
#include <kactionclasses.h>
#include <kdiroperator.h>
#include <kfileiconview.h>
#include <klocale.h>
#include <kpopupmenu.h>

#include "previewer.h"
#include "thumbviewdiroperator.h"

#include "thumbview.h"
#include "thumbview.moc"


ThumbView::ThumbView(QWidget *parent,const char *name)
    : QVBox(parent)
{
    setMargin(3);
    m_thumbSize = KIcon::SizeHuge;
    m_bgImg = QString::null;
    m_firstMenu = true;					// first time menu used

    m_dirop = new ThumbViewDirOperator(KURL(Previewer::galleryRoot()),this);

    m_fileview = new KFileIconView(this,"filepreview");
    m_dirop->setView(m_fileview);

    m_fileview->setViewName(i18n("Thumbnails"));
    m_fileview->setSelectionMode(KFile::Single);
    m_fileview->setViewMode(KFileView::Files);
    m_fileview->setIgnoreMaximumSize(true);
    m_fileview->showPreviews();

    connect(m_fileview->signaler(),SIGNAL(fileHighlighted(const KFileItem *)),
            SLOT(slotFileSelected(const KFileItem *)));
    connect(m_fileview->signaler(),SIGNAL(activatedMenu(const KFileItem *,const QPoint &)),
            SLOT(slotFileSelected(const KFileItem *)));

    connect(m_dirop,SIGNAL(finishedLoading()),SLOT(slotFinishedLoading()));

    m_lastSelected = m_dirop->url();
    m_toSelect = QString::null;

    readSettings();

    m_sizeMenu = new KActionMenu(i18n("Preview Size"),this);
    QSignalMapper *mapper = new QSignalMapper(this);
    KToggleAction *act;

    act = new KToggleAction(sizeName(KIcon::SizeEnormous),0,mapper,SLOT(map()),NULL);
    mapper->setMapping(act,KIcon::SizeEnormous);
    m_sizeMap[KIcon::SizeEnormous] = act;
    m_sizeMenu->insert(act);

    act = new KToggleAction(sizeName(KIcon::SizeHuge),0,mapper,SLOT(map()),NULL);
    mapper->setMapping(act,KIcon::SizeHuge);
    m_sizeMap[KIcon::SizeHuge] = act;
    m_sizeMenu->insert(act);

    act = new KToggleAction(sizeName(KIcon::SizeLarge),0,mapper,SLOT(map()),NULL);
    mapper->setMapping(act,KIcon::SizeLarge);
    m_sizeMap[KIcon::SizeLarge] = act;
    m_sizeMenu->insert(act);

    act = new KToggleAction(sizeName(KIcon::SizeMedium),0,mapper,SLOT(map()),NULL);
    mapper->setMapping(act,KIcon::SizeMedium);
    m_sizeMap[KIcon::SizeMedium] = act;
    m_sizeMenu->insert(act);

    act = new KToggleAction(sizeName(KIcon::SizeSmallMedium),0,mapper,SLOT(map()),NULL);
    mapper->setMapping(act,KIcon::SizeSmallMedium);
    m_sizeMap[KIcon::SizeSmallMedium] = act;
    m_sizeMenu->insert(act);

    connect(mapper,SIGNAL(mapped(int)),SLOT(slotSetSize(int)));
    connect(contextMenu(),SIGNAL(aboutToShow()),SLOT(slotAboutToShowMenu()));
}


ThumbView::~ThumbView()
{
    saveConfig();
}


QPopupMenu *ThumbView::contextMenu() const
{
    return (m_dirop->contextMenu());
}


void ThumbView::slotAboutToShowMenu()
{
    if (m_sizeMenu==NULL) return;
    kdDebug(28000) << k_funcinfo << endl;

    if (m_firstMenu)					// popup for the first time
    {
        QPopupMenu *menu = contextMenu();
        menu->insertSeparator();
        m_sizeMenu->plug(menu);				// append size menu at end
        m_firstMenu = false;				// don't do this again
    }

    for (QMap<KIcon::StdSizes,KToggleAction *>::const_iterator it = m_sizeMap.constBegin();
         it!=m_sizeMap.constEnd(); ++it)		// tick applicable size entry
    {
        it.data()->setChecked(m_thumbSize==it.key());
    }
}


void ThumbView::slotSetSize(int size)
{
    kdDebug() << k_funcinfo << "size " << size << endl;
    m_thumbSize = static_cast<KIcon::StdSizes>(size);
    m_fileview->setPreviewSize(m_thumbSize);
}


void ThumbView::slotFinishedLoading()
{
    if (m_toSelect.isNull()) return;			// nothing to do

    m_fileview->signaler()->blockSignals(true);		// avoid signal loop
    m_dirop->setCurrentItem(m_toSelect);
    m_fileview->signaler()->blockSignals(false);
    m_toSelect = QString::null;				// have dealt with this now
}


void ThumbView::slotFileSelected(const KFileItem *kfi)
{
    KURL url = (kfi!=NULL ? kfi->url() : m_dirop->url());
    kdDebug(28000) << k_funcinfo << url.prettyURL() << endl;

    if (url!=m_lastSelected)
    {
        m_lastSelected = url;
        emit selectFromThumbnail(url);
    }
}


void ThumbView::slotSelectImage(const KFileTreeViewItem *item)
{
    kdDebug(28000) << k_funcinfo << "item url=" << item->url().prettyURL() << " isDir=" << item->isDir() << endl;

    KURL cur = m_dirop->url();				// directory currently showing

    KURL urlToShow = item->url();			// new URL to show
    KURL dirToShow = urlToShow;				// directory part of that
    if (!item->isDir()) dirToShow.setFileName(QString::null);

    if (cur.path(+1)!=dirToShow.path(+1))		// see if changing path
    {
        if (!item->isDir()) m_toSelect = urlToShow.fileName();
							// select that when loading finished
        m_dirop->setURL(dirToShow,true);		// change path and reload
        return;
    }

    const KFileItem *curItem = m_dirop->selectedItems()->getFirst();
    if (curItem!=NULL)					// the current selection
    {
        if (curItem->url()==urlToShow) return;		// already selected
    }

    m_fileview->signaler()->blockSignals(true);		// avoid signal loop
    m_dirop->setCurrentItem(item->isDir() ? QString::null : urlToShow.fileName());
    m_fileview->signaler()->blockSignals(false);
}


bool ThumbView::readSettings()
{
    KConfig *cfg = KGlobal::config();
    cfg->setGroup(THUMB_GROUP);
    bool changed = false;

    KIcon::StdSizes size = static_cast<KIcon::StdSizes>(cfg->readNumEntry(THUMB_PREVIEW_SIZE,
                                                                          KIcon::SizeHuge));
    if (size!=m_thumbSize)
    {
        slotSetSize(size);
        changed = true;
    }

    QString newBgImg = cfg->readEntry(THUMB_BG_WALLPAPER,
                                      standardBackground());
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
        bgPix.resize(QSize(16,16));
        bgPix.fill(QPixmap::blue);
    }

    m_fileview->widget()->setPaletteBackgroundPixmap(bgPix);
}


void ThumbView::slotImageChanged(const KFileItem *kfi)
{
    kdDebug(28000) << k_funcinfo << kfi->url().prettyURL() << endl;
    m_fileview->updateView(kfi);			// update that view item
}


void ThumbView::slotImageRenamed(const KFileTreeViewItem *item,const QString &newName)
{
    kdDebug(28000) << k_funcinfo << item->url().prettyURL() << " -> " << newName << endl;

    if (item->isDir()) return;				// directory rename, nothing to do

    KURL cur = m_dirop->url();				// check it's the one currently showing
    if (item->url().directory(false)!=cur.path(+1)) return;

    m_toSelect = newName;				// select new after update
    m_fileview->updateView(item->fileItem());		// update that view item
}


void ThumbView::slotImageDeleted(const KFileItem *kfi)
{
    // No need to do anything, KDirOperator/KFileView handles all of that
}


QString ThumbView::standardBackground()
{
    return (KGlobal::dirs()->findResource("data",THUMB_STD_TILE_IMG));
}


QString ThumbView::sizeName(KIcon::StdSizes size)
{
    switch (size)
    {
case KIcon::SizeEnormous:	return (i18n("Very Large"));
case KIcon::SizeHuge:		return (i18n("Large"));
case KIcon::SizeLarge:		return (i18n("Medium"));
case KIcon::SizeMedium:		return (i18n("Small"));
case KIcon::SizeSmallMedium:	return (i18n("Very Small"));
case KIcon::SizeSmall:		return (i18n("Tiny"));
default:			return ("?");
    }
}
