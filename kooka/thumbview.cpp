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
#include <qabstractitemview.h>
#include <qlistview.h>

#include <kfileitem.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kaction.h>
#include <kactionmenu.h>
#include <kdiroperator.h>
#include <klocale.h>
#include <kdebug.h>
#include <kmenu.h>
#include <kconfiggroup.h>

#include <kdirmodel.h>

//#include "libkscan/previewer.h"

#include "thumbviewdiroperator.h"

#include "kookapref.h"


#define PREVIEW_MAX_FILESIZE	(20*1024*1024LL)	// 20Mb, standard default is 5Mb


// TODO: does this need to be a KVBox containing the DirOperator?
// can it not just be the DirOperator itself?
ThumbView::ThumbView(QWidget *parent)
    : KVBox(parent)
{
    setObjectName("ThumbView");

    setMargin(0);
    m_thumbSize = KIconLoader::SizeHuge;
    m_bgImg = QString::null;
    m_firstMenu = true;

    // No way to set the maximum file size or to ignore it directly,
    // the preview job with that setting is private to KFilePreviewGenerator.
    // But we can set the size limit in the application config - this also has
    // a useful side effect that the user can change the setting there if they
    // so wish.
    // See PreviewJob::maximumFileSize() in kdelibs/kio/kio/previewjob.cpp
    const KSharedConfigPtr conf = KGlobal::config();
    KConfig conf2(conf->name(), KConfig::NoGlobals);	// ensure setting goes to app config
    KConfigGroup grp = conf2.group("PreviewSettings");
    if (!grp.hasKey("MaximumSize"))
    {
        kDebug() << "Setting maximum preview file size to" << PREVIEW_MAX_FILESIZE;
        grp.writeEntry("MaximumSize", PREVIEW_MAX_FILESIZE);
        grp.sync();
    }
    else
    {
        kDebug() << "Using maximum preview file size" << grp.readEntry("MaximumSize", 0);
    }

    m_dirop = new ThumbViewDirOperator(KUrl(KookaPref::galleryRoot()), this);
    //m_dirop = new KDirOperator(KUrl(KookaPref::galleryRoot()), this);
    m_dirop->setObjectName("ThumbViewDirOperator");

    m_dirop->setPreviewWidget(NULL);			// no preview at side
    m_dirop->setMode(KFile::File);			// implies single selection mode
    m_dirop->setInlinePreviewShown(true);		// show file previews
    m_dirop->setView(KFile::Simple);			// simple icon view
    m_dirop->dirLister()->setMimeExcludeFilter(QStringList("inode/directory"));
							// only files, not directories

//    connect(m_dirop, SIGNAL(fileHighlighted(const KFileItem &)),
    connect(m_dirop, SIGNAL(fileSelected(const KFileItem &)),
            SLOT(slotFileSelected(const KFileItem &)));
    connect(m_dirop, SIGNAL(contextMenuAboutToShow(const KFileItem &,QMenu *)),
            SLOT(slotAboutToShowMenu(const KFileItem &,QMenu *)));
    connect(m_dirop, SIGNAL(finishedLoading()),
            SLOT(slotFinishedLoading()));

    // Scrolling to the selected item only works after the KDirOperator's
    // KDirModel has received this signal.
    connect(m_dirop->dirLister(), SIGNAL(refreshItems(const QList<QPair<KFileItem,KFileItem> > &)),
            SLOT(slotEnsureVisible()));

    m_lastSelected = m_dirop->url();
    m_toSelect = KUrl();
    m_toChangeTo = KUrl();

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


KMenu *ThumbView::contextMenu() const
{
    return (m_dirop->contextMenu());
}


void ThumbView::slotAboutToShowMenu(const KFileItem &kfi, QMenu *menu)
{
    kDebug();

    // TODO: is this necessary?
    //slotFileSelected(kfi);

    if (m_firstMenu)					// first time menu activated
    {
        menu->addSeparator();
        menu->addAction(m_sizeMenu);			// append size menu at end
        m_firstMenu = false;				// note this has been done
    }

    for (QMap<KIconLoader::StdSizes, KToggleAction *>::const_iterator it = m_sizeMap.constBegin();
         it!=m_sizeMap.constEnd(); ++it)		// tick applicable size entry
    {
        (*it)->setChecked(m_thumbSize==it.key());
    }
}


void ThumbView::slotSetSize(int size)
{
    m_thumbSize = static_cast<KIconLoader::StdSizes>(size);

    // see KDirOperator::setIconsZoom() in kdelibs/kfile/kdiroperator.cpp
    int val = ((size-KIconLoader::SizeSmall)*100)/(KIconLoader::SizeEnormous-KIconLoader::SizeSmall);

    kDebug() << "size" << size << "-> val" << val;
    m_dirop->setIconsZoom(val);
}


void ThumbView::slotFinishedLoading()
{
    if (m_toChangeTo.isValid())				// see if change deferred
    {
        kDebug() << "setting dirop url to" << m_toChangeTo;
        m_dirop->setUrl(m_toChangeTo, true);		// change path and reload
        m_toChangeTo = KUrl();				// have dealt with this now
        return;
    }

    if (m_toSelect.isValid())				// see if something to select
    {
        kDebug() << "selecting" << m_toSelect;
        bool blk = m_dirop->blockSignals(true);		// avoid signal loop
        m_dirop->setCurrentItem(m_toSelect.url());
        m_dirop->blockSignals(blk);
        m_toSelect = KUrl();				// have dealt with this now
    }
}


void ThumbView::slotEnsureVisible()
{
    QListView *v = qobject_cast<QListView *>(m_dirop->view());
    if (v==NULL) return;

    // Ensure that the currently selected item is visible,
    // from KDirOperator::Private::_k_assureVisibleSelection()
    // in kdelibs/kfile/kdiroperator.cpp
    QItemSelectionModel *selModel = v->selectionModel();
    if (selModel->hasSelection())
    {
        const QModelIndex index = selModel->currentIndex();
        kDebug() << "ensuring visible" << index;
        v->scrollTo(index, QAbstractItemView::EnsureVisible);
    }
}


void ThumbView::slotFileSelected(const KFileItem &kfi)
{
    KUrl url = (!kfi.isNull() ? kfi.url() : m_dirop->url());
    kDebug() << url;

    if (url!=m_lastSelected)
    {
        m_lastSelected = url;
        emit selectFromThumbnail(url);
    }
}


void ThumbView::slotSelectImage(const KFileItem *item)
{
    kDebug() << "item url" << item->url() << "isDir" << item->isDir();

    KUrl cur = m_dirop->url();				// directory currently showing

    KUrl urlToShow = item->url();			// new URL to show
    KUrl dirToShow = urlToShow;				// directory part of that
    if (!item->isDir()) dirToShow.setFileName(QString::null);

    if (cur.path(KUrl::AddTrailingSlash)!=dirToShow.path(KUrl::AddTrailingSlash))
    {							// see if changing path
        if (!item->isDir()) m_toSelect = urlToShow;
							// select that when loading finished

        // Bug 216928: Need to check whether the KDirOperator's KDirLister is
        // currently busy.  If it is, then trying to set the KDirOperator to a
        // new directory at this point is accepted but fails soon afterwards
        // with an assertion such as:
        //
        //    kooka(7283)/kio (KDirModel): Items emitted in directory
        //    KUrl("file:///home/jjm4/Documents/KookaGallery/a")
        //    but that directory isn't in KDirModel!
        //    Root directory: KUrl("file:///home/jjm4/Documents/KookaGallery/a/a")
        //    ASSERT: "result" in file /ws/trunk/kdelibs/kio/kio/kdirmodel.cpp, line 372
        //
        // To fix this, if the KDirLister is busy we delay changing to the new
        // directory until the it has finished, the finishedLoading() signal
        // will then call our slotFinishedLoading() and do the setUrl() there.
        //
        // There are two possible (but extremely unlikely) race conditions here.

        if (m_dirop->dirLister()->isFinished())		// idle, can do this now
        {
            kDebug() << "lister idle, changing dir to" << dirToShow;
            m_dirop->setUrl(dirToShow, true);		// change path and reload
        }
        else
        {
            kDebug() << "lister busy, deferring change to" << dirToShow;
            m_toChangeTo = dirToShow;
        }
        return;
    }

    KFileItemList selItems = m_dirop->selectedItems();
    if (!selItems.isEmpty())				// the current selection
    {
        KFileItem curItem = selItems.first();
        if (curItem.url()==urlToShow) return;		// already selected
    }

    m_dirop->blockSignals(true);			// avoid signal loop
    m_dirop->setCurrentItem(item->isDir() ? QString::null : urlToShow.url());
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
    kDebug() << kfi->url();
    // TODO: is there an equivalent?
    //m_fileview->updateView(kfi);			// update that view item
}


void ThumbView::slotImageRenamed(const KFileItem *item, const QString &newName)
{
    kDebug() << item->url() << "->" << newName;

    // Nothing to do here.
    // KDirLister::refreshItems signal -> slotEnsureVisible()
    // will scroll to the selected item.
}


void ThumbView::slotImageDeleted(const KFileItem *kfi)
{
    // No need to do anything here.
}


QString ThumbView::standardBackground()
{
    return (KGlobal::dirs()->findResource("data", THUMB_STD_TILE_IMG));
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
