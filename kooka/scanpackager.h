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
#include <klistview.h>
#include <kio/job.h>
#include <kio/global.h>
#include <kio/file.h>


#include "img_saver.h"
#include "packageritem.h"
/**
  *@author Klaas Freitag
  */

class ScanPackager : public KListView
{
	Q_OBJECT
public: 
   ScanPackager( QWidget *parent);
   ~ScanPackager();
   virtual QString getImgName( QString name_on_disk );
   virtual QString getSaveRoot(){ return( save_root ); }
	
public slots:
   void 	slAddImage( QImage *img );		
   void 	slSelectionChanged( QListViewItem *);
   void         slShowContextMenue(QListViewItem *, const QPoint &, int );
   void 	slHandlePopup( int item );

   void         exportFile( PackagerItem *curr );	

   void         slotCanceled(KIO::Job*);
   void         slotResult( KIO::Job*);
signals:
   void 	showImage( QImage* );
   void         deleteImage( QImage* );
   void         unloadImage( QImage* );
	
private:
   int 	        readDir( QListViewItem *parent, const char *dir_to_read );
   void         showContextMenu( QPoint p, bool show_folder = true );
   void 			 createFolder( void );
   bool         deleteItem( PackagerItem*, bool );
   void         unloadItem( PackagerItem *curr );

   PackagerItem *root;
   QDir         curr_copy_dir;
   QString      save_root;
   KIO::Job     *copyjob;
   
   int 		img_counter;
};

#endif
