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

#include <qsignalmapper.h>
#include <qabstractitemview.h>
#include <qlistview.h>
#include <qaction.h>
#include <qdebug.h>
#include <qmenu.h>
#include <qstandardpaths.h>

#include <kfileitem.h>
#include <kactionmenu.h>
#include <kdiroperator.h>
#include <klocalizedstring.h>
#include <kcolorscheme.h>

#include "kookapref.h"
#include "kookasettings.h"


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
    qDebug() << "Maximum preview file size is" << KookaSettings::previewMaximumSize();

    setUrl(QUrl::fromUserInput(KookaPref::galleryRoot()), true);
							// initial location
    setPreviewWidget(nullptr);				// no preview at side
    setMode(KFile::File);				// implies single selection mode
    setInlinePreviewShown(true);			// show file previews
    setView(KFile::Simple);				// simple icon view
    dirLister()->setMimeExcludeFilter(QStringList("inode/directory"));
							// only files, not directories

    connect(this, SIGNAL(fileSelected(KFileItem)),
            SLOT(slotFileSelected(KFileItem)));
    connect(this, SIGNAL(fileHighlighted(KFileItem)),
            SLOT(slotFileHighlighted(KFileItem)));
    connect(this, SIGNAL(finishedLoading()),
            SLOT(slotFinishedLoading()));

    // We want to provide our own context menu, not the one that
    // KDirOperator has built in.
    disconnect(view(), SIGNAL(customContextMenuRequested(QPoint)), nullptr, nullptr);
    connect(view(), SIGNAL(customContextMenuRequested(QPoint)),
            SLOT(slotContextMenu(QPoint)));

    mContextMenu = new QMenu(this);
    mContextMenu->addSection(i18n("Thumbnails"));

    // Scrolling to the selected item only works after the KDirOperator's
    // KDirModel has received this signal.
    connect(dirLister(), SIGNAL(refreshItems(QList<QPair<KFileItem,KFileItem> >)),
            SLOT(slotEnsureVisible()));

    m_lastSelected = url();
    m_toSelect = QUrl();
    m_toChangeTo = QUrl();

    readSettings();

    m_sizeMenu = new KActionMenu(i18n("Preview Size"), this);
    QSignalMapper *mapper = new QSignalMapper(this);
    KToggleAction *act;

    act = new KToggleAction(sizeName(KIconLoader::SizeEnormous), this);
    connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
    mapper->setMapping(act, KIconLoader::SizeEnormous);
    m_sizeMap[KIconLoader::SizeEnormous] = act;
    m_sizeMenu->addAction(act);

    act = new KToggleAction(sizeName(KIconLoader::SizeHuge), this);
    connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
    mapper->setMapping(act, KIconLoader::SizeHuge);
    m_sizeMap[KIconLoader::SizeHuge] = act;
    m_sizeMenu->addAction(act);

    act = new KToggleAction(sizeName(KIconLoader::SizeLarge), this);
    connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
    mapper->setMapping(act, KIconLoader::SizeLarge);
    m_sizeMap[KIconLoader::SizeLarge] = act;
    m_sizeMenu->addAction(act);

    act = new KToggleAction(sizeName(KIconLoader::SizeMedium), this);
    connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
    mapper->setMapping(act, KIconLoader::SizeMedium);
    m_sizeMap[KIconLoader::SizeMedium] = act;
    m_sizeMenu->addAction(act);

    act = new KToggleAction(sizeName(KIconLoader::SizeSmallMedium), this);
    connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
    mapper->setMapping(act, KIconLoader::SizeSmallMedium);
    m_sizeMap[KIconLoader::SizeSmallMedium] = act;
    m_sizeMenu->addAction(act);

    connect(mapper, SIGNAL(mapped(int)), SLOT(slotSetSize(int)));

    setMinimumSize(64, 64);             // sensible minimum size
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

#ifdef WORKAROUND_216928
        if (dirLister()->isFinished()) {        // idle, can do this now
            //qDebug() << "lister idle, changing dir to" << dirToShow;
            setUrl(dirToShow, true);            // change path and reload
        } else {
            //qDebug() << "lister busy, deferring change to" << dirToShow;
            m_toChangeTo = dirToShow;           // note to do later
        }
#else
        //qDebug() << "changing dir to" << dirToShow;
        setUrl(dirToShow, true);            // change path and reload
#endif
        return;
    }

    KFileItemList selItems = selectedItems();
    if (!selItems.isEmpty()) {              // the current selection
        KFileItem curItem = selItems.first();
        if (curItem.url() == urlToShow) {
            return;    // already selected
        }
    }

    // TODO: use QSignalBlocker
    bool b = blockSignals(true);            // avoid signal loop
    setCurrentItem(isDir ? QUrl() : urlToShow);
    blockSignals(b);
}

void ThumbView::slotContextMenu(const QPoint &pos)
{
    if (m_firstMenu) {                  // first time menu activated
        mContextMenu->addSeparator();
        mContextMenu->addAction(m_sizeMenu);        // append size menu at end
        m_firstMenu = false;                // note this has been done
    }

    for (QMap<KIconLoader::StdSizes, KToggleAction *>::const_iterator it = m_sizeMap.constBegin();
            it != m_sizeMap.constEnd(); ++it) {    // tick applicable size entry
        (*it)->setChecked(m_thumbSize == it.key());
    }

    mContextMenu->exec(QCursor::pos());
}

void ThumbView::slotSetSize(int size)
{
    m_thumbSize = size;

    // see KDirOperator::setIconsZoom() in kio/src/kfilewidgets/kdiroperator.cpp
    int val = ((size - KIconLoader::SizeSmall) * 100) / (KIconLoader::SizeEnormous - KIconLoader::SizeSmall);

    //qDebug() << "size" << size << "-> val" << val;
    setIconsZoom(val);
}

void ThumbView::slotFinishedLoading()
{
    if (m_toChangeTo.isValid()) {           // see if change deferred
        //qDebug() << "setting dirop url to" << m_toChangeTo;
        setUrl(m_toChangeTo, true);         // change path and reload
        m_toChangeTo = QUrl();              // have dealt with this now
        return;
    }

    if (m_toSelect.isValid()) {             // see if something to select
        //qDebug() << "selecting" << m_toSelect;
        // TODO: use QSignalBlocker
        bool blk = blockSignals(true);          // avoid signal loop
        setCurrentItem(m_toSelect);
        blockSignals(blk);
        m_toSelect = QUrl();                // have dealt with this now
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
        //qDebug() << "ensuring visible" << index;
        v->scrollTo(index, QAbstractItemView::EnsureVisible);
    }
}

void ThumbView::slotFileSelected(const KFileItem &kfi)
{
    QUrl u = (!kfi.isNull() ? kfi.url() : url());
    //qDebug() << u;

    if (u != m_lastSelected) {
        m_lastSelected = u;
        emit itemActivated(u);
    }
}

void ThumbView::slotFileHighlighted(const KFileItem &kfi)
{
    QUrl u = (!kfi.isNull() ? kfi.url() : url());
    //qDebug() << u;
    emit itemHighlighted(u);
}

void ThumbView::readSettings()
{
    slotSetSize(KookaSettings::thumbnailPreviewSize());
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
    //qDebug() << "custom" << newCustomBg << "img" << newBgImg;

    if (newCustomBg && !newBgImg.isEmpty()) {		// can set custom background
        if (bgPix.load(newBgImg)) {
            pal.setBrush(iv->backgroundRole(), QBrush(bgPix));
        } else {
            qWarning() << "Failed to load background image" << newBgImg;
        }
    } else {						// reset to default
        KColorScheme sch(QPalette::Normal);
        pal.setBrush(iv->backgroundRole(), sch.background());
    }

    iv->setPalette(pal);
}

void ThumbView::slotImageChanged(const KFileItem *kfi)
{
    //qDebug() << kfi->url();
    // TODO: is there an equivalent?
    //m_fileview->updateView(kfi);          // update that view item
}

void ThumbView::slotImageRenamed(const KFileItem *item, const QString &newName)
{
    //qDebug() << item->url() << "->" << newName;

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