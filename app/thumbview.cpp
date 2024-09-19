/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2002-2016 Klaas Freitag <freitag@suse.de>		*
 *                          Jonathan Marten <jjm@keelhaul.me.uk>	*
 *									*
 *  Kooka is free software; you can redistribute it and/or modify it	*
 *  under the terms of the GNU Library General Public License as	*
 *  published by the Free Software Foundation and appearing in the	*
 *  file COPYING included in the packaging of this file;  either	*
 *  version 2 of the License, or (at your option) any later version.	*
 *									*
 *  As a special exception, permission is given to link this program	*
 *  with any version of the KADMOS OCR/ICR engine (a product of		*
 *  reRecognition GmbH, Kreuzlingen), and distribute the resulting	*
 *  executable without including the source code for KADMOS in the	*
 *  source distribution.						*
 *									*
 *  This program is distributed in the hope that it will be useful,	*
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of	*
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*
 *  GNU General Public License for more details.			*
 *									*
 *  You should have received a copy of the GNU General Public		*
 *  License along with this program;  see the file COPYING.  If		*
 *  not, see <http://www.gnu.org/licenses/>.				*
 *									*
 ************************************************************************/

#include "thumbview.h"

#include <qabstractitemview.h>
#include <qlistview.h>
#include <qaction.h>
#include <qmenu.h>
#include <qstandardpaths.h>
#include <qtimer.h>
#include <QSignalBlocker>

#include <kfileitem.h>
#include <kactionmenu.h>
#include <kdiroperator.h>
#include <kdirlister.h>
#include <klocalizedstring.h>
#include <kcolorscheme.h>
#include <ktoggleaction.h>
#include <kio_version.h>

#include "galleryroot.h"
#include "kookasettings.h"
#include "kooka_logging.h"


void ThumbView::createActionForSize(KIconLoader::StdSizes size)
{
    KToggleAction *act = new KToggleAction(sizeName(size), this);
    connect(act, &QAction::triggered, this, [=]() { slotSetSize(size); });
    m_sizeMap[size] = act;
    m_sizeMenu->addAction(act);
}


ThumbView::ThumbView(QWidget *parent)
    : KDirOperator(QUrl(), parent)
{
    setObjectName("ThumbView");

    m_thumbSize = KIconLoader::SizeHuge;
    m_firstMenu = true;

    // There seems to be no way to set the maximum preview file size or to
    // ignore it directly, the preview job with that setting is private to
    // KFilePreviewGenerator.  But we can set the size limit in our application's
    // configuration - this also has a useful side effect that the user can
    // change the setting there if they so wish.
    // See PreviewJob::startPreview() in kio/src/widgets/previewjob.cpp
    qCDebug(KOOKA_LOG) << "Maximum preview file size is" << KookaSettings::previewMaximumSize();

    setUrl(GalleryRoot::root(), true);			// initial location
    setPreviewWidget(nullptr);				// no preview at side
    setMode(KFile::File);				// implies single selection mode
    setInlinePreviewShown(true);			// show file previews
    setViewMode(KFile::Simple);				// simple icon view
    dirLister()->setMimeExcludeFilter(QStringList("inode/directory"));
							// only files, not directories
    connect(this, &KDirOperator::fileSelected, this, &ThumbView::slotFileSelected);
    connect(this, &KDirOperator::fileHighlighted, this, &ThumbView::slotFileHighlighted);
    connect(this, &KDirOperator::finishedLoading, this, &ThumbView::slotFinishedLoading);

    // We want to provide our own context menu, not the one that
    // KDirOperator has built in.
    disconnect(view(), &QWidget::customContextMenuRequested, nullptr, nullptr);
    connect(view(), &QWidget::customContextMenuRequested, this, &ThumbView::slotContextMenu);

    mContextMenu = new QMenu(this);
    mContextMenu->addSection(i18n("Thumbnails"));

    // Scrolling to the selected item only works after the KDirOperator's
    // KDirModel has received this signal.
    connect(dirLister(), &KCoreDirLister::refreshItems, this, &ThumbView::slotEnsureVisible);

    m_lastSelected = url();
    m_toSelect = QUrl();

    readSettings();

    m_sizeMenu = new KActionMenu(i18n("Preview Size"), this);
    createActionForSize(KIconLoader::SizeEnormous);
    createActionForSize(KIconLoader::SizeHuge);
    createActionForSize(KIconLoader::SizeLarge);
    createActionForSize(KIconLoader::SizeMedium);
    createActionForSize(KIconLoader::SizeSmallMedium);

    setMinimumSize(64, 64);				// sensible minimum size
}

ThumbView::~ThumbView()
{
    saveConfig();
}

void ThumbView::slotHighlightItem(const QUrl &url, bool isDir)
{
    QUrl cur = this->url();				// directory currently showing
    QUrl urlToShow = url;				// new URL to show
    QUrl dirToShow = urlToShow;				// directory part of that
    if (!isDir) dirToShow = dirToShow.adjusted(QUrl::RemoveFilename);

    if (cur.adjusted(QUrl::StripTrailingSlash).path() != dirToShow.adjusted(QUrl::StripTrailingSlash).path())
    {							// see if changing path
        if (!isDir) m_toSelect = urlToShow;		// select that when loading finished

        // A workaround was formerly needed here:
        //
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
        //
        // It is not clear whether this is the same fundamental problem as tried to
        // work around above, but it seems to be possible to reliably trigger a
        // segfault in KDirModel when a new directory is created for the second time
        // at the same level (e.g. the top) of the gallery tree:
        //
        //    Application: Kooka (kooka), signal: Segmentation fault
        //    KFileItem::isDir (this=this@entry=0x8) at kio/src/core/kfileitem.cpp:1284
        //    KDirModelPrivate::isDir (node=0x0) at kio/src/widgets/kdirmodel.cpp:222
        //    KDirModelPrivate::isDir (node=0x0) at kio/src/widgets/kdirmodel.cpp:220
        //    KDirModelPrivate::_k_slotCompleted() at kio/src/widgets/kdirmodel.cpp:572
        //    [signal despatch]
        //    KCoreDirLister::listingDirCompleted() at moc_kcoredirlister.cpp:497
        //    KCoreDirListerCache::slotUpdateResult() at kio/src/core/kcoredirlister.cpp:1842
        //    KJob::result() at moc_kjob.cpp:635
        //
        // The twofold solution seems to be first to stop the lister if it appears
        // to be currently busy, and then to defer setting the new URL until after
        // a hopefully imperceptible delay.

        if (!dirLister()->isFinished())			// not idle, ensure stopped first
        {
            qCDebug(KOOKA_LOG) << "lister busy, stopping";
            dirLister()->stop();
        }

        qCDebug(KOOKA_LOG) << "deferring change to" << dirToShow;
        QTimer::singleShot(100, this, [this,dirToShow]()
        {
            setUrl(dirToShow, true);			// change path and reload
        });
        return;
    }

    KFileItemList selItems = selectedItems();
    if (!selItems.isEmpty())				// the current selection
    {
        KFileItem curItem = selItems.first();
        if (curItem.url() == urlToShow) return;		// already selected
    }

    QSignalBlocker block(this);
    setCurrentItem(isDir ? QUrl() : urlToShow);
}

void ThumbView::slotContextMenu(const QPoint &pos)
{
    if (m_firstMenu) {					// first time menu activated
        mContextMenu->addSeparator();
        mContextMenu->addAction(m_sizeMenu);		// append size menu at end
        m_firstMenu = false;				// note this has been done
    }

    for (QMap<KIconLoader::StdSizes, KToggleAction *>::const_iterator it = m_sizeMap.constBegin();
            it != m_sizeMap.constEnd(); ++it) {		// tick applicable size entry
        (*it)->setChecked(m_thumbSize == it.key());
    }

    mContextMenu->exec(QCursor::pos());
}

void ThumbView::slotSetSize(KIconLoader::StdSizes size)
{
    m_thumbSize = size;

#if KIO_VERSION>=QT_VERSION_CHECK(5, 76, 0)
    setIconSize(size);
#else
    // see KDirOperator::setIconsZoom() in kio/src/kfilewidgets/kdiroperator.cpp
    const int val = ((size-KIconLoader::SizeSmall)*100) / (KIconLoader::SizeEnormous-KIconLoader::SizeSmall);
    qCDebug(KOOKA_LOG) << "size" << size << "-> val" << val;
    setIconsZoom(val);
#endif
}

void ThumbView::slotFinishedLoading()
{
    if (m_toSelect.isValid()) {				// see if something to select
        qCDebug(KOOKA_LOG) << "selecting" << m_toSelect;
        QSignalBlocker block(this);
        setCurrentItem(m_toSelect);
        m_toSelect = QUrl();				// have dealt with this now
    }
}

void ThumbView::slotEnsureVisible()
{
    QListView *v = qobject_cast<QListView *>(view());
    if (v == nullptr) {
        return;
    }

    // Ensure that the currently selected item is visible,
    // from KDirOperator::Private::_k_assureVisibleSelection()
    // in kdelibs/kfile/kdiroperator.cpp
    QItemSelectionModel *selModel = v->selectionModel();
    if (selModel->hasSelection()) {
        const QModelIndex index = selModel->currentIndex();
        qCDebug(KOOKA_LOG) << "ensuring visible" << index;
        v->scrollTo(index, QAbstractItemView::EnsureVisible);
    }
}

void ThumbView::slotFileSelected(const KFileItem &kfi)
{
    QUrl u = (!kfi.isNull() ? kfi.url() : url());
    //qCDebug(KOOKA_LOG) << u;

    if (u != m_lastSelected) {
        m_lastSelected = u;
        emit itemActivated(u);
    }
}

void ThumbView::slotFileHighlighted(const KFileItem &kfi)
{
    QUrl u = (!kfi.isNull() ? kfi.url() : url());
    //qCDebug(KOOKA_LOG) << u;
    emit itemHighlighted(u);
}

void ThumbView::readSettings()
{
    slotSetSize(static_cast<KIconLoader::StdSizes>(KookaSettings::thumbnailPreviewSize()));
    setBackground();
}

void ThumbView::saveConfig()
{
    // Do nothing, preview size set by menu is for this session only.
    // Set the default size in Kooka Preferences.
}

void ThumbView::setBackground()
{
    QPixmap bgPix;
    QWidget *iv = view()->viewport();			// go down to the icon view
    QPalette pal = iv->palette();

    QString newBgImg = KookaSettings::thumbnailBackgroundPath();
    bool newCustomBg = KookaSettings::thumbnailCustomBackground();
    qCDebug(KOOKA_LOG) << "custom" << newCustomBg << "img" << newBgImg;

    if (newCustomBg && !newBgImg.isEmpty()) {		// can set custom background
        if (bgPix.load(newBgImg)) {
            pal.setBrush(iv->backgroundRole(), QBrush(bgPix));
        } else {
            qCWarning(KOOKA_LOG) << "Failed to load background image" << newBgImg;
        }
    } else {						// reset to default
        KColorScheme sch(QPalette::Normal);
        pal.setBrush(iv->backgroundRole(), sch.background());
    }

    iv->setPalette(pal);
}

void ThumbView::slotImageChanged(const KFileItem *kfi)
{
    //qCDebug(KOOKA_LOG) << kfi->url();
    // TODO: is there an equivalent?
    //m_fileview->updateView(kfi);			// update that view item
}

void ThumbView::slotImageRenamed(const KFileItem *item, const QString &newName)
{
    //qCDebug(KOOKA_LOG) << item->url() << "->" << newName;

    // Nothing to do here.
    // KDirLister::refreshItems signal -> slotEnsureVisible()
    // will scroll to the selected item.
}

void ThumbView::slotImageDeleted(const KFileItem *kfi)
{
    // No need to do anything here.
}

// TODO: code directly in settings
QString ThumbView::standardBackground()
{
    return (QStandardPaths::locate(QStandardPaths::AppDataLocation, "pics/thumbviewtile.png"));
}

QString ThumbView::sizeName(KIconLoader::StdSizes size)
{
    switch (size) {
    case KIconLoader::SizeEnormous:	return (i18n("Very Large"));
    case KIconLoader::SizeHuge:		return (i18n("Large"));
    case KIconLoader::SizeLarge:	return (i18n("Medium"));
    case KIconLoader::SizeMedium:	return (i18n("Small"));
    case KIconLoader::SizeSmallMedium:	return (i18n("Very Small"));
    case KIconLoader::SizeSmall:	return (i18n("Tiny"));
    default:				return ("?");
    }
}
