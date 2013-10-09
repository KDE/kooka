
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


#ifndef SCANGALLERY_H
#define SCANGALLERY_H

#include <qmap.h>

#include <kmimetypetrader.h>

#include "libfiletree/filetreeview.h"
#include "imageformat.h"


class QImage;
class QTreeWidgetItem;

class KMenu;
class KUrl;

class ImgSaver;
class ImageMetaInfo;
class KookaImage;


class ScanGallery : public FileTreeView
{
    Q_OBJECT

public:
    ScanGallery(QWidget *parent);
    ~ScanGallery();

    QString getCurrImageFileName(bool withPath = true) const;
    const KookaImage *getCurrImage(bool loadOnDemand = false);

    KMenu *contextMenu() const { return m_contextMenu; }
    void openRoots();

    void setAllowRename(bool on);

    bool prepareToSave(const ImageMetaInfo *info);
    void addImage(const QImage *img, const ImageMetaInfo *info = NULL);

    void saveHeaderState(const QString &key) const;
    void restoreHeaderState(const QString &key);

public slots:
    void slotExportFile();
    void slotImportFile();
    void slotSelectImage(const KUrl &url);
    void slotSelectDirectory(const QString &branchName, const QString &relPath);
    void slotUnloadItems();

protected:
    // TODO: port D&D
    //virtual void contentsDragMoveEvent( QDragMoveEvent *ev);

    void contextMenuEvent(QContextMenuEvent *ev);

protected slots:
    void slotImageArrived(FileTreeViewItem *item, KookaImage *image);
    void slotCreateFolder();
    void slotDeleteItems();
    void slotRenameItems();
    void slotUnloadItem(FileTreeViewItem *curr);
    void slotDirCount(FileTreeViewItem *item, int cnt);
    void slotStartupFinished(FileTreeViewItem *item);
    void slotItemExpanded(QTreeWidgetItem *item);
    void slotItemProperties();

    void slotUrlsDropped(QDropEvent *ev,FileTreeViewItem *item);
    void slotJobResult(KJob *job);
    bool slotFileRenamed(FileTreeViewItem *item, const QString &newName);

    void slotDecorate(FileTreeViewItem *item);
    void slotDecorate(FileTreeBranch *branch, const FileTreeViewItemList &list);

    void slotItemHighlighted(QTreeWidgetItem *curr);
    void slotItemActivated(QTreeWidgetItem *curr);
    void slotHighlightItem(const KUrl &url);
    void slotActivateItem(const KUrl &url);
    void slotUpdatedItem(const KUrl &url);

signals:
    void aboutToShowImage(const KUrl &url);
    void showImage(const KookaImage *img, bool isDir);
    void deleteImage(const KookaImage *img);
    void unloadImage(const KookaImage *img);
    void galleryPathChanged(const FileTreeBranch *branch, const QString &relPath);
    void galleryDirectoryRemoved(const FileTreeBranch *branch, const QString &relPath);

    void imageChanged(const KFileItem *kfi);
    void fileChanged(const KFileItem *kfi);
    void fileRenamed(const KFileItem *item, const QString &newName);
    void showItem(const KFileItem *kfi);

    void itemHighlighted(const KUrl &url, bool isDir);

private:
    void loadImageForItem(FileTreeViewItem *item);
    FileTreeBranch *openRoot(const KUrl &root, const QString &title = QString::null);

    FileTreeViewItem *findItemByUrl(const KUrl &url, FileTreeBranch *branch = NULL);
    KUrl itemDirectory(const FileTreeViewItem *item) const;
    QString itemDirectoryRelative(const FileTreeViewItem *item) const;
    void updateParent(const FileTreeViewItem *curr);

    KUrl m_currSelectedDir;
    KMenu *m_contextMenu;

    ImgSaver *mSaver;
    FileTreeViewItem *mSavedTo;

    // like m_nextUrlToSelect in KFileTreeView,
    // but for our own purposes (showing the image)
    KUrl m_nextUrlToShow;

    QPixmap mPixFloppy;
    QPixmap mPixGray;
    QPixmap mPixBw;
    QPixmap mPixColor;

    FileTreeBranch *m_defaultBranch;
    bool m_startup;
};


#endif							// SCANGALLERY_H
