/***************************************************************************
                          kookaimage.h  - Kooka's Image
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



#ifndef KOOKAIMAGE_H
#define KOOKAIMAGE_H
#include <kurl.h>
#include <qimage.h>
#include <kfilemetainfo.h>

class KFileItem;

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

    /**
     *  Set and get the KFileItem of the image. Note that the KFileItem pointer returned
     *  may be zero.
     */
    KFileItem*   fileItem() const;
    void         setFileItem( KFileItem* );

    /**
     * @return the KFileMetaInfo
     **/
    const KFileMetaInfo fileMetaInfo( );

    /**
     * set the url of the kooka image. Note that loadFromUrl sets this
     * url automatically.
     */
    void setUrl( const KURL& url )
        { m_url = url; }

    /**
     * checks if the image is file bound ie. was loaded from file. If this
     * method returns false, fileMetaInfo and FileItem are undefined.
     */
    bool isFileBound() { return m_fileBound; }

private:
    int 		m_subImages;
    bool         loadTiffDir( const QString&, int );

    /* if subNo is 0, the image is the one and only. If it is larger than 0, the
     * parent contains the filename */
    int          m_subNo;

    /* In case beeing a subimage */
    KookaImage   *m_parent;
    KURL          m_url;
    /* Fileitem if available */
    KFileItem    *m_fileItem;
    bool          m_fileBound;
};

#endif
