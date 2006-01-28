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

#include <q3listview.h>
#include <qimage.h>
#include <qpixmap.h>
#include <q3dragobject.h>
#include <qmap.h>
//Added by qt3to4:
#include <QDragMoveEvent>
#include <Q3PopupMenu>
#include <Q3CString>
#include <QDropEvent>
#include <klistview.h>
#include <kio/job.h>
#include <kio/global.h>
#include <kio/file.h>
#include <kfiletreeview.h>


/**
  *@author Klaas Freitag
  */

class KUrl;
class Q3PopupMenu;
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
    KookaImage* getCurrImage() const;

    KFileTreeBranch* openRoot( const KUrl&, bool open=false );

   Q3PopupMenu *contextMenu() const { return m_contextMenu; }
   void         openRoots();

public slots:
   void         slSelectImage( const KUrl& );
   void 	slAddImage( QImage *img, KookaImageMeta* meta = 0 );
   void         slShowContextMenue(Q3ListViewItem *, const QPoint &, int );

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
   void         slClicked( Q3ListViewItem * );
   void         slFileRename( Q3ListViewItem*, const QString&, int );
   // void         slFilenameChanged( KFileTreeViewItem*, const KUrl & );
   void         slImageArrived( KFileTreeViewItem *item, KookaImage* image );
   void         slotCreateFolder( );
   void         slotDeleteItems( );
   void         slotUnloadItems( );
   void         slotUnloadItem( KFileTreeViewItem *curr );
   void         slotDirCount( KFileTreeViewItem *item, int cnt );
   void         slotUrlsDropped( QWidget*, QDropEvent*, KUrl::List& urls, KUrl& copyTo );
   void         slotDeleteFromBranch( KFileItem* );
   void         slotStartupFinished( KFileTreeViewItem * );
signals:
   void         showImage  ( KookaImage* );
   void         deleteImage( KookaImage* );
   void         unloadImage( KookaImage* );
   void         galleryPathSelected( KFileTreeBranch* branch, const QString& relativPath );
   void         directoryToRemove( KFileTreeBranch *branch, const QString& relativPath );
   void         showThumbnails( KFileTreeViewItem* );

   void         aboutToShowImage( const KUrl& ); /* starting to load image */
   void         imageChanged( KFileItem* );     /* the image has changed  */

    void         fileDeleted( KFileItem* );
    void         fileChanged( KFileItem* );
    void         fileRenamed( KFileItem*, const KUrl& );

private:
   QString     localFileName( KFileTreeViewItem* it ) const;
   void 	loadImageForItem( KFileTreeViewItem* item );
   Q3CString     getImgFormat( KFileTreeViewItem* item ) const;

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
    Q3PopupMenu    *m_contextMenu;

    // like m_nextUrlToSelect in KFileTreeView but for our own purposes (showing the image)
    KUrl         m_nextUrlToShow;

   QPixmap       m_floppyPixmap;
   QPixmap       m_grayPixmap;
   QPixmap       m_bwPixmap;
   QPixmap       m_colorPixmap;

   KFileTreeBranch *m_defaultBranch;
   bool          m_startup;
};

#endif
