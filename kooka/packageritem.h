/***************************************************************************
                          packageritem.h  -  description                              
                             -------------------                                         
    begin                : Mon Dec 27 1999                                           
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


#ifndef PACKAGERITEM_H
#define PACKAGERITEM_H
#include <kurl.h>

#include <qlistview.h>
#include <qimage.h>



/**
  *@author Klaas Freitag
  */
// typedef enum{ FILE_OP_ERR, FILE_OP_OK, FILE_OP_NO_MEM,
//              FILE_OP_NO_TARGET, FILE_OP_PERM } FileOpStat;

typedef enum{ Dummy, NameSearch } SearchType;

class PackagerItem : public QListViewItem
{
   
public: 
   PackagerItem( QListViewItem *parent, bool is_dir = false );
   PackagerItem( QListView *parent, bool is_dir = false );
	
   ~PackagerItem();
	
   void    setImage( QImage *img, const QCString& );
   void    setFilename( QString fi );
   QImage  *getImage( void );

   /**
    *  test if the item is a directory.
    *
    * @result true, if it is a directory.
    */
   
   bool    isDir( void ) const { return ( isdir ); }

   /**
    *  queries if an image is loaded, e.g. needs memory.
    *
    * @return true if the image is in the memory.
    */
   bool    isLoaded( void ) { return( !( image == 0 ) ); }

   /**
    *  Returns the filename of the item as QString. If the parameter withPath
    *  is false, only the filename without path is returned, otherwise the complete
    *  absolute path including filename.
    *
    * @param withPath flag to indicate if the path is wanted
    *
    * @return the path- or filename.
    */
   QString getFilename( bool withPath=true ) const;
   QString getLocalFilename( void ) const;

   /**
    *  Returns the directory of the item.
    */
   QString getDirectory( void ) const;
   
   KURL    getFilenameURL( void ) const;
   void    changedParentsPath( KURL );
#if 0
   /**
    *  Retrieves the compete path where the item is stored.
    *
    * @return the directory name.
    */
   QString getStorageDir( void ) const;
#endif
   
   bool    createFolder( void );
   bool    unload();
   bool    deleteFile();
   bool    deleteFolder();

   void    decorateFile();
   QCString getImgFormat() const;
   
   // FileOpStat     duplicate( QString to ) { return( copy_file(KURL(to))); }
   // FileOpStat     move( QString to ){ return( copy_file(KURL(to))); };

private:
   QImage 		*image;
   KURL                 filename;
   bool    		isdir;
   QCString             format; // Image format 
};

#endif
