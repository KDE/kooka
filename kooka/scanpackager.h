/***************************************************** -*- mode:c++; -*- ***
                          scanpackager.h  -  description
                             -------------------
    begin                : Fri Dec 17 1999
    copyright            : (C) 1999 by Klaas Freitag
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


#ifndef SCANPACKAGER_H
#define SCANPACKAGER_H

#include <k3filetreeview.h>
#include <kmimetypetrader.h>
#include <qmap.h>


/**
  *@author Klaas Freitag
  */

class QImage;
class Q3ListViewItem;
class QSignalMapper;

class KMenu;
class KUrl;
class KActionMenu;
//class KActionCollection;

class KookaImage;
class KookaImageMeta;

class PackagerInfo;


// TODO: into class
typedef enum{ Dummy, NameSearch, UrlSearch } SearchType;


class ScanPackager : public K3FileTreeView
{
    Q_OBJECT

public:
    ScanPackager( QWidget *parent);
    ~ScanPackager();
    virtual QString getImgName( QString name_on_disk );

    QString 	getCurrImageFileName( bool ) const;
    const KookaImage *getCurrImage(bool loadOnDemand = false);

    KFileTreeBranch* openRoot(const KUrl &root, bool open = false);

   KMenu *contextMenu() const { return m_contextMenu; }
   void         openRoots();

   void setAllowRename(bool on);
   void showOpenWithMenu(KActionMenu *menu);

   void 	addImage(const QImage *img,KookaImageMeta *meta = NULL);

public slots:
   void         slotSelectImage( const KUrl& );
   void         slotShowContextMenu(Q3ListViewItem *lvi,const QPoint &p);

   void         slotExportFile( );
    void        slotImportFile();
    // TODO: does this need to be a slot?
   void         slotCurrentImageChanged(const QImage *img);

   void         slotSelectDirectory(const QString &branchName,const QString &relPath);

protected:
   virtual void contentsDragMoveEvent( QDragMoveEvent *ev);

protected slots:
   void         slotClicked(Q3ListViewItem *);
   void         slotFileRename(Q3ListViewItem *it,const QString &newName);
   // void         slFilenameChanged( KFileTreeViewItem*, const KUrl & );
   void         slotImageArrived(K3FileTreeViewItem *item, KookaImage *image);
   void         slotCreateFolder();
   void         slotDeleteItems();
   void         slotRenameItems();
   void         slotUnloadItems();
   void         slotUnloadItem( K3FileTreeViewItem *curr );
   void         slotDirCount( K3FileTreeViewItem *item, int cnt );
   void         slotUrlsDropped(K3FileTreeView *me,QDropEvent *ev,Q3ListViewItem *parent,Q3ListViewItem *after);
   void         slotDeleteFromBranch( KFileItem* );
   void         slotStartupFinished( K3FileTreeViewItem * );
   void         slotItemExpanded(Q3ListViewItem *item);
   void slotOpenWith(int idx);
   void slotItemProperties();

   void         slotDecorate(K3FileTreeViewItem *item);
   void         slotDecorate(KFileTreeBranch *branch, const K3FileTreeViewItemList &list);
   void         slotCanceled(KIO::Job*);

signals:
   void         showImage(const KookaImage *img,bool isDir);
   void         deleteImage(const KookaImage *img);
   void         unloadImage(const KookaImage *img);
   void         galleryPathChanged( KFileTreeBranch* branch, const QString& relativPath );
   void         galleryDirectoryRemoved( KFileTreeBranch *branch, const QString& relativPath );
   void         showThumbnails( K3FileTreeViewItem* );

   void         aboutToShowImage( const KUrl& ); /* starting to load image */

    void imageChanged(const KFileItem *kfi);
    void fileDeleted(const KFileItem *kfi);
    void fileChanged(const KFileItem *kfi);
    void fileRenamed(const K3FileTreeViewItem *item,const QString &newName);
    void showItem(const K3FileTreeViewItem *item);


private:
   static QString localFileName(const K3FileTreeViewItem *item);
   static QByteArray getImgFormat(const K3FileTreeViewItem *item);

   void 	loadImageForItem( K3FileTreeViewItem* item );

    QString 	 buildNewFilename(const QString &cmplFilename,const QString &currFormat) const;
   K3FileTreeViewItem *spFindItem(SearchType type,const QString &name,const KFileTreeBranch *branch = NULL);
   QString       itemDirectory( const K3FileTreeViewItem *item, bool relativ = false ) const;
   void updateParent(const K3FileTreeViewItem *curr);

   // int 	        readDir( Q3ListViewItem *parent, QString dir_to_read );
    void         showContextMenu( QPoint p, bool show_folder = true );


    const PackagerInfo infoForUrl(const KUrl &url);
    KookaImage *imageForItem(const K3FileTreeViewItem *item);


    QString      currSelectedDir;
    int          img_counter;
    KMenu    *m_contextMenu;

    KService::List openWithOffers;
    QSignalMapper *openWithMapper;
//    KActionCollection *openWithActions;

    // like m_nextUrlToSelect in KFileTreeView but for our own purposes (showing the image)
    KUrl         m_nextUrlToShow;

   QPixmap       m_floppyPixmap;
   QPixmap       m_grayPixmap;
   QPixmap       m_bwPixmap;
   QPixmap       m_colorPixmap;

   KFileTreeBranch *m_defaultBranch;
   bool          m_startup;

    // PackagerInfo is fairly small, so this map stores values
    QMap<KUrl, PackagerInfo> mInfoMap;
};

#endif							// SCANPACKAGER_H
