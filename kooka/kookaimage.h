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

#include <qimage.h>
#include <qvector.h>
#include <qrect.h>

#include <kurl.h>

#include <kfilemetainfo.h>

class KFileItem;
class KUrl;


/**
  * @author Klaas Freitag
  *
  * class that represents an image, very much as QImage. But this one can contain
  * multiple pages.
  */

// TODO: into class
typedef enum { MaxCut, MediumCut } TileMode;

class KookaImage : public QImage
{
public:
    KookaImage();
    /**
     * creating a subimage for a parent image.
     * @param subNo contains the sequence number of subimages to create.
     * @param p is the parent image.
     */
    KookaImage(int subNo, KookaImage *p);
    KookaImage(const QImage &img);
    ~KookaImage();

    KookaImage& operator=(const KookaImage &src);
    KookaImage& operator=(const QImage &src);

    /**
     * load an image from a KURL. This method reads the entire file and sets
     * the values for subimage count.  Returns a null string if succeeded,
     * or an error message string if failed.
     */
    QString loadFromUrl(const KUrl &url);

    /**
     * the number of subimages. This is 0 if there are no subimages.
     */
    int subImagesCount() const;

    /**
     * the parent image.
     */
    KookaImage *parentImage() const;

    /**
     * returns true if this is a subimage.
     */
    bool isSubImage() const;

    /**
     * extracts the correct subimage according to the number given in the constructor.
     */
    void extractNow();

    QString localFileName() const;

    /**
     *  Set and get the KFileItem of the image. Note that the KFileItem pointer returned
     *  may be NULL.
     */
    const KFileItem *fileItem() const;
    void setFileItem(const KFileItem *item);

    /**
     * @return the KFileMetaInfo
     **/
    const KFileMetaInfo fileMetaInfo() const;

    /**
     * set the url of the kooka image. Note that loadFromUrl sets this
     * url automatically.
     */
    void setUrl(const KUrl &url) { m_url = url; }
    KUrl url() const { return m_url; }

    /**
     * checks if the image is file bound ie. was loaded from file. If this
     * method returns false, fileMetaInfo and FileItem are undefined.
     */
    bool isFileBound() const { return m_fileBound; }

    /**
     * Create tiles on the given image. That is just cut the image in parts
     * while non of the parts is larger than maxSize and store the rect list.
     * The parameters rows and cols contain the number of rows and cols after
     * tiling. If both are one, the image is smaller than maxSize, thus the
     * left-top tile is index 1,1.
     * Use getTile() to read the QRect list.
     */
    int cutToTiles(const QSize maxSize, int &rows, int &cols, TileMode mode = MaxCut);

    /**
     * read tiles from the tile list. The image needs to be tiled by method
     * cutToTiles before.
     */
    QRect getTileRect(int rowPos, int colPos) const;

    /**
     * retrieve the sub number of this image.
     */
    int subNumber() const { return m_subNo; }


private:
    int 		m_subImages;
    bool                loadTiffDir( const QString&, int );

    /* if subNo is 0, the image is the one and only. If it is larger than 0, the
     * parent contains the filename */
    int                 m_subNo;

    /* In case being a subimage */
    KookaImage          *m_parent;
    KUrl                m_url;
    /* Fileitem if available */
    const KFileItem           *m_fileItem;
    bool                m_fileBound;

    QVector<QRect> m_tileVector;
    int                 m_tileCols;  /* number of tile columns  */
};


#endif							// KOOKAIMAGE_H
