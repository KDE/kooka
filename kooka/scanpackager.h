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
#include <qdir.h>
#include <qdragobject.h>
#include <qmap.h>
#include <klistview.h>
#include <kio/job.h>
#include <kio/global.h>
#include <kio/file.h>


/**
  *@author Klaas Freitag
  */
#include "packageritem.h"
class KURL;
class KPopupMenu;
class PackagerItem;

class JobDescription
{
public:
   enum JobType { NoJob, ImportJob, RenameJob, ExportJob };
   JobDescription():jobType( NoJob ), kioJob(0L), pitem(0L) {}
   JobDescription( KIO::Job* kiojob, PackagerItem *new_item, JobType type ) :
      jobType(type), kioJob(kiojob), pitem(new_item) {}

   JobType type( void ) { return( jobType ); }
   PackagerItem *item( void ) { return( pitem ); }
   KIO::Job* job( void ){ return( kioJob ); }
private:
   JobType       jobType;
   KIO::Job*     kioJob;
   PackagerItem* pitem;
};

class ScanPackager : public KListView
{
    Q_OBJECT
public: 
    ScanPackager( QWidget *parent);
    ~ScanPackager();
    virtual QString getImgName( QString name_on_disk );
    virtual QString getSaveRoot(){ return( save_root ); }

    QString 	getCurrImageFileName( bool ) const;
   
public slots:
    void         slSelectImage( const QString );
    void 	 slAddImage( QImage *img );		
    void 	 slSelectionChanged( QListViewItem *);
    void         slShowContextMenue(QListViewItem *, const QPoint &, int );
    void 	 slHandlePopup( int item );

    void         exportFile( PackagerItem *);
    void         slotRename( PackagerItem* , const KURL& );
    void         slotImportFiles( PackagerItem*, const QStringList );
    
    void         slotCanceled(KIO::Job*);
    void         slotRenameResult( KIO::Job*);
    void         slotImportFinished( KIO::Job*);
    void         slotExportFinished( KIO::Job *job );
   
    void         slotImageChanged( QImage * );

protected slots:
    void         slCollapsed( QListViewItem *);
    void         slExpanded( QListViewItem *);
    void         slFileRename( QListViewItem*, const QString&, int );
    void         slFilenameChanged( PackagerItem*, const KURL & );
    void 	 slDropped(QDropEvent * e, QListViewItem *after);
    bool         acceptDrag(QDropEvent *ev) const;
    signals:
    void 	showImage( QImage* );
    void         deleteImage( QImage* );
    void         unloadImage( QImage* );
	
private:

    QString 	 buildNewFilename( QString cmplFilename, QString currFormat ) const;   
    PackagerItem *spFindItem( SearchType type, const QString name );
    void         storeJob( KIO::Job*, PackagerItem *, JobDescription::JobType );
   
    int 	        readDir( QListViewItem *parent, QString dir_to_read );
    void         showContextMenu( QPoint p, bool show_folder = true );
    void 			 createFolder( void );
    bool         deleteItem( PackagerItem*, bool );
    void         unloadItem( PackagerItem *curr );

    PackagerItem *root;
    QDir         curr_copy_dir;
    QString      save_root;
    KIO::Job     *copyjob;
    KPopupMenu   *popup;   
    int 		img_counter;

   QMap<KIO::Job*, JobDescription> jobMap;
};

#endif
