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

#include <kfiletreeview.h>
#include <ktrader.h>


/**
  *@author Klaas Freitag
  */

class QPopupMenu;
class QImage;
class QListViewItem;
class QSignalMapper;

class KookaImage;
class KookaImageMeta;

class KURL;
class KActionMenu;

typedef enum{ Dummy, NameSearch, UrlSearch } SearchType;


class ScanPackager : public KFileTreeView
{
    Q_OBJECT
public:
    ScanPackager( QWidget *parent);
    ~ScanPackager();
    virtual QString getImgName( QString name_on_disk );

    QString 	getCurrImageFileName( bool ) const;
    KookaImage* getCurrImage(bool loadOnDemand = false);

    KFileTreeBranch* openRoot( const KURL&, bool open=false );

   QPopupMenu *contextMenu() const { return m_contextMenu; }
   void         openRoots();

   void setAllowRename(bool on);
   void showOpenWithMenu(KActionMenu *menu);

   void 	addImage(const QImage *img,KookaImageMeta *meta = NULL);

public slots:
   void         slSelectImage( const KURL& );
   void         slShowContextMenu(QListViewItem *lvi,const QPoint &p);

   void         slotExportFile( );
    void        slotImportFile();
   void         slotCurrentImageChanged( QImage* );

   void         slotSelectDirectory(const QString &branchName,const QString &relPath);

protected:
   virtual void contentsDragMoveEvent( QDragMoveEvent *e );

protected slots:
   void         slClicked(QListViewItem *);
   void         slFileRename(QListViewItem *it,const QString &newName);
   // void         slFilenameChanged( KFileTreeViewItem*, const KURL & );
   void         slImageArrived( KFileTreeViewItem *item, KookaImage* image );
   void         slotCreateFolder( );
   void         slotDeleteItems( );
   void         slotRenameItems( );
   void         slotUnloadItems( );
   void         slotUnloadItem( KFileTreeViewItem *curr );
   void         slotDirCount( KFileTreeViewItem *item, int cnt );
   void         slotUrlsDropped(KFileTreeView *me,QDropEvent *ev,QListViewItem *parent,QListViewItem *after);
   void         slotDeleteFromBranch( KFileItem* );
   void         slotStartupFinished( KFileTreeViewItem * );
   void         slotItemExpanded(QListViewItem *item);
   void slotOpenWith(int idx);
   void slotItemProperties();

   void         slotDecorate( KFileTreeViewItem* );
   void         slotDecorate( KFileTreeBranch*, const KFileTreeViewItemList& );
   void         slotCanceled(KIO::Job*);

signals:
   void         showImage(const KookaImage *img,bool isDir);
   void         deleteImage( KookaImage* );
   void         unloadImage( KookaImage* );
   void         galleryPathChanged( KFileTreeBranch* branch, const QString& relativPath );
   void         galleryDirectoryRemoved( KFileTreeBranch *branch, const QString& relativPath );
   void         showThumbnails( KFileTreeViewItem* );

   void         aboutToShowImage( const KURL& ); /* starting to load image */

    void imageChanged(const KFileItem *kfi);
    void fileDeleted(const KFileItem *kfi);
    void fileChanged(const KFileItem *kfi);
    void fileRenamed(const KFileTreeViewItem *item,const QString &newName);
    void showItem(const KFileTreeViewItem *item);


private:
   QString     localFileName( KFileTreeViewItem* it ) const;
   void 	loadImageForItem( KFileTreeViewItem* item );
   QCString     getImgFormat( KFileTreeViewItem* item ) const;

    QString 	 buildNewFilename( QString cmplFilename, QString currFormat ) const;
   KFileTreeViewItem *spFindItem(SearchType type,const QString &name,const KFileTreeBranch *branch = NULL);
   QString       itemDirectory( const KFileTreeViewItem *item, bool relativ = false ) const;
   void updateParent(const KFileTreeViewItem *curr);

   // int 	        readDir( QListViewItem *parent, QString dir_to_read );
    void         showContextMenu( QPoint p, bool show_folder = true );

    QString      m_currImportDir;
    QString      m_currCopyDir;
    QString      currSelectedDir;
    int          img_counter;
    QPopupMenu    *m_contextMenu;

    KTrader::OfferList openWithOffers;
    QSignalMapper *openWithMapper;

    // like m_nextUrlToSelect in KFileTreeView but for our own purposes (showing the image)
    KURL         m_nextUrlToShow;

   QPixmap       m_floppyPixmap;
   QPixmap       m_grayPixmap;
   QPixmap       m_bwPixmap;
   QPixmap       m_colorPixmap;

   KFileTreeBranch *m_defaultBranch;
   bool          m_startup;
};

#endif
