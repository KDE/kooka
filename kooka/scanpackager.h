/***************************************************************************
                          scanpackager.h  -  description
                             -------------------
    begin                : Fri Dec 17 1999
    copyright            : (C) 1999 by Klaas Freitag
    email                : freitag@suse.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef SCANPACKAGER_H
#define SCANPACKAGER_H

#include <qlistview.h>
#include <qimage.h>
#include <qpixmap.h>
#include <qdragobject.h>
#include <qmap.h>
#include <klistview.h>
#include <kio/job.h>
#include <kio/global.h>
#include <kio/file.h>
#include <kfiletreeview.h>


/**
  *@author Klaas Freitag
  */

class KURL;
class QPopupMenu;
class KFileTreeViewItem;
class KookaImage;
class KookaImageMeta;
class KFileTreeBranch;


typedef enum{ Dummy, NameSearch, UrlSearch } SearchType;

class JobDescription
{
public:
   enum JobType { NoJob, ImportJob, RenameJob, ExportJob };
   JobDescription():jobType( NoJob ), kioJob(0L), pitem(0L) {}
   JobDescription( KIO::Job* kiojob, KFileTreeViewItem *new_item, JobType type ) :
      jobType(type), kioJob(kiojob), pitem(new_item) {}

   JobType type( void ) { return( jobType ); }
   KFileTreeViewItem *item( void ) { return( pitem ); }
   KIO::Job* job( void ){ return( kioJob ); }
private:
   JobType       jobType;
   KIO::Job*     kioJob;
   KFileTreeViewItem* pitem;
};

class ScanPackager : public KFileTreeView
{
    Q_OBJECT
public:
    ScanPackager( QWidget *parent);
    ~ScanPackager();
    virtual QString getImgName( QString name_on_disk );

    QString 	getCurrImageFileName( bool ) const;
    KookaImage* getCurrImage();

    KFileTreeBranch* openRoot( const KURL&, bool open=false );

   QPopupMenu *contextMenu() { return m_contextMenu; }
   void         openRoots();

public slots:
   void         slSelectImage( const KURL& );
   void 	slAddImage( QImage *img, KookaImageMeta* meta = 0 );
   void 	slSelectionChanged( QListViewItem *);
   void         slShowContextMenue(QListViewItem *, const QPoint &, int );

   void         slotExportFile( );

   void         slotCanceled(KIO::Job*);
   void         slotCurrentImageChanged( QImage* );

   void         slotDecorate( KFileTreeViewItem* );
   void         slotDecorate( KFileTreeBranch*, const KFileTreeViewItemList& );

   void         slotSelectDirectory( const QString& );

protected:
   virtual void contentsDragMoveEvent( QDragMoveEvent *e );

protected slots:
   void         slFileRename( QListViewItem*, const QString&, int );
   // void         slFilenameChanged( KFileTreeViewItem*, const KURL & );
   // void 	 slDropped(QDropEvent * e, QListViewItem *after);
   void         slImageArrived( KFileTreeViewItem *item, KookaImage* image );
   void         slotExportFinished( KIO::Job *job );
   void 	slotCreateFolder( );
   void 	slotDeleteItems( );
   void         slotUnloadItems( );
   void         slotUnloadItem( KFileTreeViewItem *curr );
   void         slotDirCount( KFileTreeViewItem *item, int cnt );
   void 	slotUrlsDropped( KURL::List& urls, KURL& copyTo );
   void         slotDeleteFromBranch( KFileItem* );
   void         slotStartupFinished( KFileTreeViewItem * );
signals:
   void 	showImage  ( KookaImage* );
   void         deleteImage( KookaImage* );
   void         unloadImage( KookaImage* );
   void         galleryPathSelected( KFileTreeBranch* branch, const QString& relativPath );
   void         directoryToRemove( KFileTreeBranch *branch, const QString& relativPath );
   void         showThumbnails( KFileTreeViewItem* );

   void         aboutToShowImage( const KURL& ); /* starting to load image */
   void         imageChanged( KFileItem* );     /* the image has changed  */

   void         fileDeleted( KFileItem* );
   void         fileChanged( KFileItem* );

private:
   QString     localFileName( KFileTreeViewItem* it ) const;
   void 	loadImageForItem( KFileTreeViewItem* item );
   QCString     getImgFormat( KFileTreeViewItem* item ) const;

    QString 	 buildNewFilename( QString cmplFilename, QString currFormat ) const;
   KFileTreeViewItem *spFindItem( SearchType type, const QString name, const KFileTreeBranch* branch = 0 );
    void         storeJob( KIO::Job*, KFileTreeViewItem *, JobDescription::JobType );
   QString       itemDirectory( const KFileTreeViewItem*, bool relativ = false ) const;

   // int 	        readDir( QListViewItem *parent, QString dir_to_read );
    void         showContextMenu( QPoint p, bool show_folder = true );

    QString      m_currCopyDir;
    QString      currSelectedDir;
    KIO::Job     *copyjob;
    int          img_counter;
    QPopupMenu    *m_contextMenu;

    QMap<KIO::Job*, JobDescription> jobMap;

   QPixmap       m_floppyPixmap;
   QPixmap       m_grayPixmap;
   QPixmap       m_bwPixmap;
   QPixmap       m_colorPixmap;

   KFileTreeBranch *m_defaultBranch;
   bool          m_startup;
};

#endif
