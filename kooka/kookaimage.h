/***************************************************************************
                          kookaimage.h  - Kooka's Image 
                             -------------------                                         
    begin                : Thu Nov 20 2001
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


#ifndef KOOKAIMAGE_H
#define KOOKAIMAGE_H
#include <kurl.h>
#include <qimage.h>


/**
  * @author Klaas Freitag
  *
  * class that represents an image, very much as QImage. But this one can contain
  * multiple pages.
  */


class KookaImage: public QImage
{
public:
    
   KookaImage( );
   /**
    * creating a subimage for a parent image.
    * @param subNo contains the sequence number of subimages to create.
    * @param p is the parent image.
    */
   KookaImage(  int subNo, KookaImage *p ); 
   KookaImage( 	const QImage& img );

   /**
    * load an image from a KURL. This method reads the entire file and sets
    * the values for subimage count.
    */
   bool         loadFromUrl( const KURL& );
   
   ~KookaImage();

   /**
    * the amount of subimages. This is 0 if there are no subimages.
    */
   int         	subImagesCount() const;

   /**
    * the parent image.
    */
   KookaImage*  parentImage() const;

   /**
    * returns true if this is a subimage.
    */
   bool         isSubImage() const;

   /**
    * extracts the correct subimage according to the number given in the constructor.
    */
   void         extractNow();
   
   KURL         url() const;
   QString      localFileName( ) const;

private:
   int 		m_subImages;
   bool         loadTiffDir( const QString&, int );

   /* if subNo is 0, the image is the one and only. If it is larger than 0, the
    * parent contains the filename */
   int          m_subNo;

   /* In case beeing a subimage */
   KookaImage   *m_parent;
   KURL          m_url;
};

#endif
