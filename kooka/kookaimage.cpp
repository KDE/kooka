/***************************************************************************
                          kookaimage.cpp  - Kooka's Image
                             -------------------
    begin                : Thu Nov 20 2001
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

#include <kdebug.h>
#include <kurl.h>
#include <kfileitem.h>

#include "kookaimage.h"
#include <tiffio.h>
#include <tiff.h>
#include "config.h"
/**
  *@author Klaas Freitag
  */


KookaImage::KookaImage( )
   : QImage(),
     m_subImages(-1),
     m_subNo(0),
     m_parent(0),
     m_fileBound(false)
{

}

/* constructor for subimages */
KookaImage::KookaImage( int subNo, KookaImage *p )
   : QImage(),
     m_subImages(-1),
     m_subNo(subNo),
     m_parent( p ),
     m_fileItem(0L)
{
   kdDebug(28000) << "Setting subimageNo to " << subNo << endl;
}

KFileItem* KookaImage::fileItem() const
{
    return m_fileItem;
}

void KookaImage::setFileItem( KFileItem* it )
{
    m_fileItem = it;
}

const KFileMetaInfo KookaImage::fileMetaInfo( )
{
    QString filename = localFileName( );
    if( ! filename.isEmpty() )
    {
        kdDebug(28000) << "Fetching metainfo for " << filename << endl;
        const KFileMetaInfo info( filename );
        return info;
    }
    else
        return KFileMetaInfo();
}

QString KookaImage::localFileName( ) const
{

    if( ! m_url.isEmpty() )
        return( m_url.directory() + "/" + m_url.fileName());
    else
        return QString();
}

bool KookaImage::loadFromUrl( const KURL& url )
{
   bool ret = true;
   m_url = url;
   QString filename = localFileName( );
   QString format ( imageFormat( filename ));

   /* if the format was not recogniseable, check the extension, if it is tif, try to read it by
    * tifflib */
   if( format.isNull() )
   {
      if( filename.endsWith( "tif" ) || filename.endsWith( "tiff" ) ||
	  filename.endsWith( "TIF" ) || filename.endsWith( "TIFF" ) )
      {
	 format = "tif";
	 kdDebug(28000) << "Setting format to tif by extension" << endl;
      }
   }

   kdDebug(28000) << "Image format to load: <" << format << "> from file <" << filename << ">" << endl;
   bool haveTiff = false;

   if( !m_url.isLocalFile() )
   {
      kdDebug(28000)<<"ERROR: Can not laod non-local images -> not yet implemented!" << endl;
      return false;
   }

#ifdef HAVE_LIBTIFF
   TIFF* tif = 0;
   m_subImages = 0;

   if( format == "tif" ||
       format == "TIF" ||
       format == "TIFF" ||
       format == "tiff" )
   {
      /* if it is tiff, check with Tifflib if it is multiple sided */
      kdDebug(28000) << "Trying to load TIFF!" << endl;
      tif = TIFFOpen(filename.latin1(), "r");
      if (tif)
      {
	 do {
	    m_subImages++;
	 } while (TIFFReadDirectory(tif));
	 kdDebug(28000) << m_subImages << " TIFF-directories found" << endl;

	 haveTiff = true;
      }
   }
#endif
   if( !haveTiff )
   {
      /* Qt can only read one image */
      ret = load(filename);
      if( ret )
      {
	 m_subImages = 0;
	 m_subNo = 0;
      }
   }
#ifdef HAVE_LIBTIFF
   else
   {
      loadTiffDir( filename, 0);
      /* its a tiff, read by tifflib directly */
      // Find the width and height of the image
   }
#endif

   m_fileBound = ret;
   return( ret );
}


KookaImage::KookaImage( const QImage& img )
   : QImage( img )
   /* m_subImages( 1 ) */
{
   m_subImages = 0;

   /* Load one QImage, can not be Tiff yet. */
   kdDebug(28000) << "constructor from other image here " << endl;
}


/* loads the number stored in m_subNo */
void KookaImage::extractNow()
{
   kdDebug(28000) << "extracting a subimage number " << m_subNo << endl;

   KookaImage *parent = parentImage();

   if( parent )
   {
      loadTiffDir( parent->localFileName(), m_subNo );
   }
   else
   {
      kdDebug(28000) << "ERR: No parent defined - can not laod subimage" << endl;
   }
}

KURL KookaImage::url() const
{
   return m_url;
}

bool KookaImage::loadTiffDir( const QString& filename, int no )
{
#ifdef HAVE_LIBTIFF
   int width, height;
   TIFF* tif = 0;
   /* if it is tiff, check with Tifflib if it is multiple sided */
   kdDebug(28000) << "Trying to load TIFF, subimage number "<< no  << endl;
   tif = TIFFOpen(filename.latin1(), "r");
   if (!tif)
      return false;

   if( ! TIFFSetDirectory( tif, no ) )
   {
      kdDebug(28000) << "ERR: could not set Directory " << no << endl;
      TIFFClose(tif);
      return false;
   }

   TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
   TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);

   /* TODO: load bw-image correctly only 2 bit */
   KookaImage tmpImg;
   create( width, height, 32 );
   if (TIFFReadRGBAImage(tif, width, height, (uint32*) bits(),0))
   {
      // successfully read. now convert.
      // reverse red and blue
      uint32 *data;
      data = (uint32 *)bits();
      for( unsigned i = 0; i < unsigned(width * height); ++i )
      {
	 uint32 red = ( 0x00FF0000 & data[i] ) >> 16;
	 uint32 blue = ( 0x000000FF & data[i] ) << 16;
	 data[i] &= 0xFF00FF00;
	 data[i] += red + blue;
      }

      // reverse image (it's upside down)
      unsigned h = unsigned(height);
      for( unsigned ctr = 0; ctr < h>>1; )
      {
	 unsigned *line1 = (unsigned *)scanLine( ctr );
	 unsigned *line2 = (unsigned *)scanLine( height
						       - ( ++ctr ) );

	 unsigned w = unsigned(width);
	 for( unsigned x = 0; x < w; x++ )
	 {
	    int temp = *line1;
	    *line1 = *line2;
	    *line2 = temp;
	    line1++;
	    line2++;
	 }
      }
   }
   TIFFClose(tif);
   return true;
#endif
}


int KookaImage::subImagesCount() const
{
   return( m_subImages );
}

KookaImage::~KookaImage()
{

}

KookaImage* KookaImage::parentImage() const
{
   return( m_parent );
}

bool KookaImage::isSubImage() const
{
   return( subImagesCount() );
}



		       
