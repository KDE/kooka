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
#include <kio/jobclasses.h>
#include <kio/file.h>
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kio/netaccess.h>

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
   image = 0L;
}


PackagerItem::~PackagerItem()
{
   if( image ) delete( image );
}

QString PackagerItem::getLocalFilename( void ) const
{
   QString f = getFilename( true );

   /* Now the string contains file:/bla.
      The local filename does not need the 'file:' stuff.
   */
   if( f.left(5) == "file:" )
   {
      return( f.remove( 0,5 ));
   }	
   else
   {
      return( QString::null );
   }
}


/* Should return directory relative to save root. */
QString PackagerItem::getDirectory( void ) const
{
   QString str = filename.path(0);
   
   kdDebug(28000) << "GetDirectory returns " << str << endl;

   return(str );
}

QString PackagerItem::getFilename( bool withPath  ) const 
{
   QString ret = filename.url();

   if( isDir() && ret.right(1) != "/" )
       ret += "/";
       
   if( ! withPath )
   {
      ret = filename.fileName();
   }
   kdDebug( 28000) << "Packageritem getFilename return " << ret << endl;
   return( ret );
}

KURL PackagerItem::getFilenameURL( void ) const
{
   return( filename );
}

QImage *PackagerItem::getImage( void )
{
   
   
   if( filename.isLocalFile() )
   {
      if( image ) delete image;

      /* Bummer ! This is terrible. If the PackagerItem is a KURL and may
	 reside on a remote server, how to open this image here ? Seems not
	 to be a good concept at all ;(
      */
      QString str = filename.path();
      kdDebug(28000) << "getImage: Loading <" << str << ">" << endl;

      image = new QImage( );
      if( image->load( str ) ) {
	 format = QImage::imageFormat(str);
	 kdDebug(28000) << "getImage: Loaded image successfull" << endl;
      } else {
	 kdDebug(28000) << "getImage: Could not load image " << str << endl;
      }
      decorateFile();
   }
   else
   {
      kdDebug( 28000) << "getImage: is not a local file -> Cant load image !!" << endl;
   }
		
   return( image );
}

QCString PackagerItem::getImgFormat( void ) const
{
#if 0
   QString str = getFilename( );
   
   if( str.isNull() ) return( "" );
   QFileInfo fi(str);
   QString exten = fi.extension();
   
   kdDebug( 28000) << "Returning extension: " << exten << endl;
#endif
   return( format );
}


void PackagerItem::setFilename( QString fi )
{

   filename = fi;
   
   // filename.setFileName( fi );
   decorateFile();
}

void PackagerItem::setImage( QImage *img, const QCString& newFormat )
{
   if( image ) delete image;
   
   image = new QImage( *img );
   format = newFormat;
   
   decorateFile();
 	
}

bool PackagerItem::deleteFolder( void )
{
   QDir direc;
   direc.setFilter( QDir::Files | QDir::Hidden | QDir::NoSymLinks );
   direc.setPath( getFilename());
 	
   if( direc.exists() )
   {
      return( direc.rmdir(getFilename()));
   }
   return( true );
}

bool PackagerItem::unload( void )
{
   if( image )
   {
      delete( image );
      image = 0L;
      format = "";
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
   direc.setPath(getFilename());

   return( direc.remove(getLocalFilename()));
}

bool PackagerItem::createFolder( void )
{
   if( ! isDir()) return( false );  /* can only create Dirs */
   if( filename.isEmpty()) return( false ); /* no empty files */

   if( ! KIO::NetAccess::exists( getFilename())) {
      kdDebug(28000) << "WRN Creating directory <" << getFilename() << ">" << endl;
      KIO::mkdir( getFilename());
   }
 		
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

      /* image format */
      setText( 3, format );
   }
   else
   {
      QString bName = getFilename(false ); // get without path
      kdDebug(28000) << "Stating Basename <"<< bName << ">" << endl;
      
      setText( 0, bName );
      if( isDir() )
      {
	 if( isOpen() ) {
	    setPixmap( 0, *icons["mini-folder-open"] );
	 }
	 else {
	    setPixmap( 0, *icons["mini-folder"] );
	 }
	 int cc = childCount();
         if (cc==1)
	    s = i18n("1 item");
         else
	    s = i18n("%1 items").arg(cc);
	 setText( 1, s );
      }
      else
      {
	 setPixmap( 0, *icons["mini-floppy"] );
      }

   }
}


void PackagerItem::changedParentsPath( KURL newParentPath )
{

   if( (newParentPath.url()).right(1) != "/" )
   {
      newParentPath.addPath( "/" );
   }
   QString me = filename.filename(true);
   kdDebug(28000) << "Got parentpath: " << newParentPath.url() << endl;
   filename = newParentPath;

   if( isDir() )
   {
      filename.addPath( me );

      PackagerItem *child = (PackagerItem*) firstChild();
      while( child )
      {
	    child->changedParentsPath( filename );
	    child = (PackagerItem*) child->nextSibling();
      }
   }
   else
   {
      filename.setFileName( me );
   }
   kdDebug(28000) << "refreshPath created new filename " << filename.url() << endl;
}
