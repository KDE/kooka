/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 1999-2016 Klaas Freitag <Klaas.Freitag@gmx.de>	*
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

#ifndef SCANGALLERY_H
#define SCANGALLERY_H

#include "kookacore_export.h"

#include "filetreeview.h"

#include "imageformat.h"
#include "scanimage.h"

class QImage;
class QTreeWidgetItem;
class QMenu;
class QUrl;

class ImgSaver;


class KOOKACORE_EXPORT ScanGallery : public FileTreeView
{
    Q_OBJECT

public:
    explicit ScanGallery(QWidget *parent);
    ~ScanGallery() override;

    QString currentImageFileName() const;
    ScanImage::Ptr getCurrImage(bool loadOnDemand = false);

    void openRoots();

    void setAllowRename(bool on);

    bool prepareToSave(ScanImage::ImageType type);
    void addImage(ScanImage::Ptr img);

    void saveHeaderState(int forIndex) const;
    void restoreHeaderState(int forIndex);
    QUrl saveURL() const;

    QMenu *contextMenu() const				{ return (m_contextMenu); }
    ImgSaver *imageSaver() const			{ return (mSaver); }

public slots:
    void slotExportFile();
    void slotImportFile();
    void slotSelectImage(const QUrl &url);
    void slotSelectDirectory(const QString &branchName, const QString &relPath);
    void slotUnloadItems();

    void slotHighlightItem(const QUrl &url);
    void slotActivateItem(const QUrl &url);
    void slotUpdatedItem(const QUrl &url);

    void slotCreateFolder();
    void slotDeleteItems();
    void slotRenameItems();
    void slotItemProperties();

protected:
    // TODO: port D&D
    //virtual void contentsDragMoveEvent( QDragMoveEvent *ev);
    void contextMenuEvent(QContextMenuEvent *ev) override;

protected slots:
    void slotImageArrived(FileTreeViewItem *item, ScanImage::Ptr img);
    void slotUnloadItem(FileTreeViewItem *curr);
    void slotDirCount(FileTreeViewItem *item, int cnt);
    void slotStartupFinished(FileTreeViewItem *item);
    void slotItemExpanded(QTreeWidgetItem *item);

    void slotUrlsDropped(QDropEvent *ev, FileTreeViewItem *item);
    void slotJobResult(KJob *job);
    bool slotFileRenamed(FileTreeViewItem *item, const QString &newName);

    void slotDecorate(FileTreeViewItem *item);
    void slotDecorate(FileTreeBranch *branch, const FileTreeViewItemList &list);

    void slotItemHighlighted(QTreeWidgetItem *curr = nullptr);
    void slotItemActivated(QTreeWidgetItem *curr);

signals:
    void aboutToShowImage(const QUrl &url);
    void showImage(ScanImage::Ptr img, bool isDir);
    void deleteImage(ScanImage::Ptr img);
    void unloadImage(ScanImage::Ptr img);
    void galleryPathChanged(const FileTreeBranch *branch, const QString &relPath);
    void galleryDirectoryRemoved(const FileTreeBranch *branch, const QString &relPath);

    void imageChanged(const KFileItem *kfi);
    void fileChanged(const KFileItem *kfi);
    void fileRenamed(const KFileItem *item, const QString &newName);
    void showItem(const KFileItem *kfi);

    void itemHighlighted(const QUrl &url, bool isDir);

private:
    void loadImageForItem(FileTreeViewItem *item);
    FileTreeBranch *openRoot(const QUrl &root, const QString &title = QString());

    FileTreeViewItem *findItemByUrl(const QUrl &url, FileTreeBranch *branch = nullptr);
    QUrl itemDirectory(const FileTreeViewItem *item) const;
    QString itemDirectoryRelative(const FileTreeViewItem *item) const;
    void updateParent(const FileTreeViewItem *curr);

    QUrl m_currSelectedDir;
    QMenu *m_contextMenu;

    ImgSaver *mSaver;
    FileTreeViewItem *mSavedTo;

    // like m_nextUrlToSelect in KFileTreeView,
    // but for our own purposes (showing the image)
    QUrl m_nextUrlToShow;

    FileTreeBranch *m_defaultBranch;
    bool m_startup;
};

#endif							// SCANGALLERY_H
