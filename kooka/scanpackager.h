/***************************************************************************
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


/**
  *@author Klaas Freitag
  */

class QPopupMenu;
class QImage;
class QListViewItem;

class KookaImage;
class KookaImageMeta;
class KURL;

typedef enum{ Dummy, NameSearch, UrlSearch } SearchType;


class ScanPackager : public KFileTreeView
{
    Q_OBJECT
public:
    ScanPackager( QWidget *parent);
    ~ScanPackager();
    virtual QString getImgName( QString name_on_disk );

    QString 	getCurrImageFileName( bool ) const;
    KookaImage* getCurrImage() const;

    KFileTreeBranch* openRoot( const KURL&, bool open=false );

   QPopupMenu *contextMenu() const { return m_contextMenu; }
   void         openRoots();
   void setAllowRename(bool on);

public slots:
   void         slSelectImage( const KURL& );
   void 	slAddImage( QImage *img, KookaImageMeta* meta = 0 );
   void         slShowContextMenu(QListViewItem *lvi,const QPoint &p);

   void         slotExportFile( );
    void        slotImportFile();
   void         slotCanceled(KIO::Job*);
   void         slotCurrentImageChanged( QImage* );

   void         slotDecorate( KFileTreeViewItem* );
   void         slotDecorate( KFileTreeBranch*, const KFileTreeViewItemList& );

   void         slotSelectDirectory( const QString& );

protected:
   virtual void contentsDragMoveEvent( QDragMoveEvent *e );

protected slots:
   void         slClicked( QListViewItem * );
   void         slFileRename( QListViewItem*, const QString&, int );
   // void         slFilenameChanged( KFileTreeViewItem*, const KURL & );
   void         slImageArrived( KFileTreeViewItem *item, KookaImage* image );
   void         slotCreateFolder( );
   void         slotDeleteItems( );
   void         slotRenameItems( );
   void         slotUnloadItems( );
   void         slotUnloadItem( KFileTreeViewItem *curr );
   void         slotDirCount( KFileTreeViewItem *item, int cnt );
   void         slotUrlsDropped( QWidget*, QDropEvent*, KURL::List& urls, KURL& copyTo );
   void         slotDeleteFromBranch( KFileItem* );
   void         slotStartupFinished( KFileTreeViewItem * );
signals:
   void         showImage(KookaImage *img,bool isDir);
   void         deleteImage( KookaImage* );
   void         unloadImage( KookaImage* );
   void         galleryPathSelected( KFileTreeBranch* branch, const QString& relativPath );
   void         directoryToRemove( KFileTreeBranch *branch, const QString& relativPath );
   void         showThumbnails( KFileTreeViewItem* );

   void         aboutToShowImage( const KURL& ); /* starting to load image */
   void         imageChanged( KFileItem* );     /* the image has changed  */

    void         fileDeleted( KFileItem* );
    void         fileChanged( KFileItem* );
    void         fileRenamed( KFileItem*, const KURL& );

private:
   QString     localFileName( KFileTreeViewItem* it ) const;
   void 	loadImageForItem( KFileTreeViewItem* item );
   QCString     getImgFormat( KFileTreeViewItem* item ) const;

    QString 	 buildNewFilename( QString cmplFilename, QString currFormat ) const;
   KFileTreeViewItem *spFindItem( SearchType type, const QString name, const KFileTreeBranch* branch = 0 );
   QString       itemDirectory( const KFileTreeViewItem*, bool relativ = false ) const;

   // int 	        readDir( QListViewItem *parent, QString dir_to_read );
    void         showContextMenu( QPoint p, bool show_folder = true );

    QString      m_currImportDir;
    QString      m_currCopyDir;
    QString      currSelectedDir;
    KIO::Job     *copyjob;
    int          img_counter;
    QPopupMenu    *m_contextMenu;

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
