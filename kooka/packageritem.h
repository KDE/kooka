/***************************************************************************
                          packageritem.h  -  description                              
                             -------------------                                         
    begin                : Mon Dec 27 1999                                           
    copyright            : (C) 1999 by Klaas Freitag                         
    email                :                                      
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   * 
 *                                                                         *
 ***************************************************************************/


#ifndef PACKAGERITEM_H
#define PACKAGERITEM_H

#include <qlistview.h>
#include <qimage.h>

/**
  *@author Klaas Freitag
  */
typedef enum{ FILE_OP_ERR, FILE_OP_OK, FILE_OP_NO_MEM,
              FILE_OP_NO_TARGET, FILE_OP_PERM } FileOpStat;

class PackagerItem : public QListViewItem  {

public: 
    PackagerItem( QListViewItem *parent, bool is_dir = false );
    PackagerItem( QListView *parent, bool is_dir = false );
	
    ~PackagerItem();
	
    void setImage( QImage *img );
    void setFilename( QString fi );
    QImage *getImage( void );
    bool    isDir( void ) { return ( isdir ); }
    bool    isLoaded( void ) { return( !( image == 0 ) ); }
    QString getFilename( bool withPath=true );
    bool    createFolder( void );
    bool    unload();
    bool    deleteFile();
    bool    deleteFolder();

    void    decorateFile();
    QString getImgFormat();
    FileOpStat     duplicate( QString to ) { return( copy_file(to)); }
    FileOpStat     move( QString to ){ return( copy_file(to)); };
    	
private:
    QImage *image;
    QString filename;

    FileOpStat    copy_file( QString );
    bool    isdir;


};

#endif
