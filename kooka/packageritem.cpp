/***************************************************************************
                          packageritem.cpp  -  description                              
                             -------------------                                         
    begin                : Mon Dec 27 1999                                           
    copyright            : (C) 1999 by Klaas Freitag                         
    email                : Klaas.freitag@suse.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   * 
 *                                                                         *
 ***************************************************************************/
#include <qdict.h>
#include <qpixmap.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <qfile.h>

#include <kdebug.h>
#include <klocale.h>

#include "resource.h"
#include "packageritem.h"


extern QDict<QPixmap> icons;


PackagerItem::PackagerItem( QListViewItem *parent, bool is_dir )
	: QListViewItem( parent )
{
   isdir = is_dir;
   image = 0;
   filename = "";
}


PackagerItem::PackagerItem( QListView *parent, bool is_dir )
	: QListViewItem( parent )
{
   isdir = is_dir;
   image = 0;
}


PackagerItem::~PackagerItem()
{
   delete( image );
}

QString PackagerItem::getFilename( bool withPath  ) const 
{
   QString ret = filename;
   
   if( ! withPath )
   {
      QFileInfo fi( ret );
      ret = fi.fileName();
   }
   return( ret );
}

QImage *PackagerItem::getImage( void )
{
   if( ! image )
   {
      image = new QImage( filename );
      decorateFile();
   }
		
   return( image );
}

QString PackagerItem::getImgFormat( void )
{
   if( filename.isNull() ) return( "" );
   QFileInfo fi( filename );
   return( fi.extension());
}


void PackagerItem::setFilename( QString fi )
{
   filename = fi;
   decorateFile();
}

void PackagerItem::setImage( QImage *img )
{
   if( image ) delete image;
 	
   image = new QImage( *img );
   decorateFile();
 	
}

bool PackagerItem::deleteFolder( void )
{
   QDir direc;
   direc.setFilter( QDir::Files | QDir::Hidden | QDir::NoSymLinks );
   direc.setPath(filename);
 	
   if( direc.exists() )
   {
      return( direc.rmdir(filename));
   }
   return( true );
}

bool PackagerItem::unload( void )
{
   if( image )
   {
      delete( image );
      image = 0;
      decorateFile();
   }

   return( true );
}

bool PackagerItem::deleteFile( void )
{
   if( isDir() ) return( false );
 	
   if( image ) unload();
 	
   QDir direc;
   direc.setFilter( QDir::Files | QDir::Hidden | QDir::NoSymLinks );
   direc.setPath(filename);

   return( direc.remove(filename));
}

bool PackagerItem::createFolder( void )
{
   if( ! isDir()) return( false );  /* can only create Dirs */
   if( filename.isEmpty()) return( false ); /* no empty files */

   QDir direc;
   direc.setFilter( QDir::Files | QDir::Hidden | QDir::NoSymLinks );
   direc.setPath(filename);

   if( !direc.exists() )
      return( direc.mkdir(filename) );
 		
   return( true );
}

void PackagerItem::decorateFile( void )
{

   QString s;
   if( image )
   {

      QSize size = image->size();	
      setText( 1, s.sprintf( "%dx%d", size.width(), size.height()));
			
      int num_colors = image->numColors();
      setText( 2, s.sprintf( "%d", num_colors ));
	
      if( image->depth() > 2 )
	 if( image->isGrayscale() )
	    setPixmap( 0, *icons[ "mini-gray" ] );
	 else
	    setPixmap( 0, *icons["mini-color"] );
      else
	 setPixmap( 0, *icons["mini-lineart"] );
   }
   else
   {
      QFileInfo fi( filename);
      setText( 0, fi.baseName() );
      if( isDir() )
      {
	 setPixmap( 0, *icons["mini-folder"] );

	 int cc = childCount();
         if (cc==1)
                s = i18n("1 item");
         else
                s = i18n("%1 items").arg(cc);
	 setText( 1, s );
      }
      else
	 setPixmap( 0, *icons["mini-floppy"] );

   }
}

FileOpStat  PackagerItem::copy_file( QString to )
{
   QFile from( filename );
   FileOpStat res = FILE_OP_OK;

   if( ! from.open( IO_ReadOnly ) )
   {
      kdDebug(28000) << "Cant open source-file!" << endl;
      return( FILE_OP_ERR );
   }
   QFile tofile( to );
   if( ! tofile.open( IO_WriteOnly ) )
   {
      kdDebug(28000) << "Cant open target-file" << endl;
      return( FILE_OP_ERR );
   }

   int bsize = 4*1024;
   char *data;
   data = new char[bsize];
   if( data )
   {
      int c = from.readBlock( data, bsize );
      while ( c  > 0 )
      {
	 tofile.writeBlock( data, c );
	 c = from.readBlock( data, bsize );
      }
      delete( data );
   } else { res = FILE_OP_ERR; }

   tofile.flush(); tofile.close();
   from.close();
   return( res );
}

