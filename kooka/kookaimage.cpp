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

#include "kookaimage.h"

#include <kdebug.h>
#include <kurl.h>
#include <kfileitem.h>
#include <kimageio.h>

#ifdef HAVE_TIFF
extern "C"
{
#include <tiffio.h>
#include <tiff.h>
}
#endif


KookaImage::KookaImage()
    : QImage(),
      m_subImages(-1),
      m_subNo(0),
      m_parent(NULL),
      m_fileBound(false),
      m_tileCols(0)
{
    kDebug();
}

/* constructor for subimages */
KookaImage::KookaImage(int subNo, KookaImage *p)
    : QImage(),
      m_subImages(-1),
      m_subNo(subNo),
      m_parent(p),
      m_fileItem(NULL),
      m_fileBound(false),
      m_tileCols(0)
{
    kDebug() << "subimageNo" << subNo;
}


KookaImage::KookaImage(const QImage &img)
    : QImage(img)
    // m_subImages( 1 )
{
    kDebug();

    /* Load one QImage, can not be Tiff yet. */
    m_subImages = 0;
}


KookaImage &KookaImage::operator=(const KookaImage &img)
{
    QImage::operator=(img);

    m_subImages = img.subImagesCount();
    m_subNo     = img.m_subNo;
    m_parent    = img.m_parent;
    m_url       = img.m_url;
    m_fileItem  = img.m_fileItem;
    
    return (*this);
}


KookaImage &KookaImage::operator=(const QImage &img)
{
    QImage::operator=(img);
    // TODO: does this initialise the rest?
    return (*this);
}


KFileItem *KookaImage::fileItem() const
{
    return (m_fileItem);
}


void KookaImage::setFileItem(KFileItem* fi)
{
    m_fileItem = fi;
}


// Only used/displayed by OCR.  Get the "TechnicalInfo".
const KFileMetaInfo KookaImage::fileMetaInfo() const
{
    QString filename = localFileName();
    if (filename.isEmpty()) return (KFileMetaInfo());

    kDebug() << "Fetching metainfo for" << filename;
    return (KFileMetaInfo(filename, QString::null, KFileMetaInfo::TechnicalInfo));
}


QString KookaImage::localFileName( ) const
{

    if( ! m_url.isEmpty() )
        return( m_url.directory() + "/" + m_url.fileName());
    else
        return QString();
}


bool KookaImage::loadFromUrl(const KUrl &url)
{
    bool ret = true;					// return status
    bool isTiff = false;				// do we have a TIFF file?
    bool haveTiff = false;				// can it be read via TIFF lib?

    m_url = url;
    if (!m_url.isLocalFile())
    {
        kDebug() << "Error: Cannot load non-local images, not yet implemented!";
        return (false);
    }

    QString filename = localFileName();
    QString format = KookaImage::formatForUrl(url);

    KMimeType::Ptr mime = KMimeType::findByUrl(url, 0, true);
    if (mime->is("image/tiff"))				// check MIME for TIFF file
    {
        isTiff = true;					// note that for later
        if (format.isEmpty())				// if format wasn't recognised,
        {
            format = "TIF";				// set it now
            kDebug() << "Setting format to TIFF by MIME type";
        }
    }

    kDebug() << "Image format to load:" << format << "from file" << filename;

#ifdef HAVE_TIFF
    if (isTiff)						// if it is TIFF, check
    {							// for multiple images
        m_subImages = 0;				// no subimages found yet
        kDebug() << "Trying to load TIFF...";
        TIFF *tif = TIFFOpen(filename.toLatin1(), "r");
        if (tif!=NULL)
        {
            do { ++m_subImages; } while (TIFFReadDirectory(tif));
            kDebug() << "found" << m_subImages << "TIFF-directories";
            haveTiff = true;
        }
    }
#endif
    if (!haveTiff)
    {
        ret = QImage::load(filename);			// Qt can only read one image
        if (ret)					// image loaded OK
        {
            m_subImages = 0;
            m_subNo = 0;
        }
    }
#ifdef HAVE_TIFF
    else
    {
        loadTiffDir(filename, 0);			// read by TIFFlib directly
    }
#endif

    m_fileBound = ret;
    return (ret);
}


/* loads the number stored in m_subNo */
void KookaImage::extractNow()
{
    kDebug() << "extracting subimage number" << m_subNo;

    KookaImage *parent = parentImage();
    if (parent!=NULL) loadTiffDir(parent->localFileName(), m_subNo);
    else kDebug() << "Error: No parent, cannot load subimage";
}


bool KookaImage::loadTiffDir(const QString &filename, int no)
{
#ifdef HAVE_TIFF
    int imgWidth,imgHeight;

    // if it is TIFF, check with TIFFlib if it is multiple images
    kDebug() << "Trying to load TIFF, subimage number" << no;
    TIFF *tif = TIFFOpen(filename.toLatin1(), "r");
    if (tif==NULL) return (false);

    if (!TIFFSetDirectory(tif, no))
    {
        kDebug() << "Error: TIFFSetDirectory failed";
        TIFFClose(tif);
        return (false);
    }
   
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH,  &imgWidth);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &imgHeight);

    // TODO: load bw-image correctly only 2 bit
    //
    // The Qt3 version of this did:
    //
    //   create( imgWidth, imgHeight, 32 );
    //
    // and then read the TIFF image into the image data as returned by bits().
    //
    // This is not possible on Qt4, where the size/depth of a QImage is set in
    // its constructor and cannot be subsequently changed.  Insteat we read
    // the TIFF file into a temporary image and then use QImage::operator=
    // to shallow copy that temporary image into ours.  After that temporary
    // image has been destroyed, we have the only copy of the TIFF read image.
    //
    QImage tmpImg(imgWidth, imgHeight, QImage::Format_RGB32);
    uint32 *data = (uint32 *) (tmpImg.bits());
    if (TIFFReadRGBAImage(tif, imgWidth, imgHeight, data, 0))
    {							// Successfully read, now convert
        tmpImg = tmpImg.rgbSwapped();			// swap red and blue
        tmpImg = tmpImg.mirrored();			// reverse (it's upside down)
    }
    else
    {
        kDebug() << "Error: TIFFReadRGBAImage failed";
        TIFFClose(tif);
        return (false);
    }

    // Fetch the x- and y-resolutions to adjust images
    float xReso,yReso;
    bool resosFound = TIFFGetField(tif, TIFFTAG_XRESOLUTION, &xReso) &&
                      TIFFGetField(tif, TIFFTAG_YRESOLUTION, &yReso);
    kDebug() << "TIFF image: X-Res" << xReso << "Y-Res" << yReso;

    TIFFClose(tif);					// finished with TIFF file

    // Check now if resolution in x- and y-directions differ.
    // If so, stretch the image accordingly.
    if (resosFound && xReso!=yReso)
    {
        if (xReso>yReso)
        {
            float yScalefactor = xReso/yReso;
            kDebug() << "Different resolution X/Y, rescaling Y with factor" << yScalefactor;
            /* rescale the image */
            tmpImg = tmpImg.scaled(imgWidth, int(imgHeight*yScalefactor),
                                   Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        }
        else
        {
            float xScalefactor = yReso/xReso;
            kDebug() << "Different resolution X/Y, rescaling X with factor" << xScalefactor;
            tmpImg = tmpImg.scaled(int(imgWidth*xScalefactor), imgHeight,
                                   Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        }
    }

    QImage::operator=(tmpImg);				// copy as our image
#endif							// HAVE_TIFF

   return (true);
}


int KookaImage::subImagesCount() const
{
    return (m_subImages);
}


KookaImage::~KookaImage()
{
}


KookaImage* KookaImage::parentImage() const
{
    return (m_parent);
}


bool KookaImage::isSubImage() const
{
    return (subImagesCount()>0);
}


/*
 * tiling
 */
int KookaImage::cutToTiles( const QSize maxSize, int& rows, int& cols, TileMode  )
{
    QSize imgSize = size();

    int w = imgSize.width();
    if( w > maxSize.width() )
    {
        // image is wider than paper
        w = maxSize.width();
    }
    int h = imgSize.height();
    if( h > maxSize.height() )
    {
        // image is wider than paper
        h = maxSize.height();
    }

    int absX = 0;  // absolute x position from where to start print
    int absY = 0;  // on the image, left top corner of the part to print
    rows = 0;

    while( h )  // Loop over height, cut in vertical direction
    {
        rows++;
        cols = 0;
        while( w ) // Loop over width, cut in horizontal direction
        {
            cols++;
            m_tileVector.append( QRect( absX, absY, w, h ));

            absX += w+1;
            w = imgSize.width() - absX;

            // if w < 0, this was the last loop, set w to zero to stop loop
            if( w < 0 ) w = 0;

            // if > 0 here, a new page is required
            if( w > 0 )
            {
                if( w > maxSize.width() ) w = maxSize.width();
            }
        }
        // Reset the X-values to start on the left border again
        absX = 0;
        // start with full width again
        w = imgSize.width();
        if( w > maxSize.width() )
            w = maxSize.width();

        absY += h+1;
        h = imgSize.height() - absY;

        if( h < 0 ) h = 0;  // be sure to meet the break condition
        if( h > maxSize.height()) h = maxSize.height();  // limit to page height
    }
    m_tileCols = cols;

    return m_tileVector.count();
}



QRect KookaImage::getTileRect(int rowPos, int colPos) const
{
    int indx = rowPos*m_tileCols+colPos;
    kDebug() << "Tile index" << indx;
    return (m_tileVector[(rowPos)*m_tileCols + colPos]);
}










// KImageIO::suffix(format) seems to be no longer present, with no
// equivalent available.  The lookup here is to get the preferred
// file extension for the Qt image format.
//
// This assumes that the KDE MIME type system will always recognise
// a file with the Qt image format key as extension.
//
// The dot is not included in the result, unlike KMimeType::mainExtension()

QString KookaImage::extensionForFormat(const QString &format)
{
    QString suf = format.toLower();
    KMimeType::Ptr mime = KMimeType::findByPath("/tmp/x."+suf, 0, true);
    if (!mime->isDefault())				// not application/octet-stream
    {
        QString ext = mime->mainExtension();
        if (!ext.isEmpty()) suf = ext;
    }
    else kDebug() << "no MIME type for image format" << format;

    if (suf.startsWith('.')) suf = suf.mid(1);
    kDebug() << "for" << format << "returning" << suf;
    return (suf);
}


// KImageIO::typeForMime returns a list of possible formats.  Normally
// we assume that there will only be one, but possibly there may be
// more.  This lookup just takes the first one.

QString KookaImage::formatForUrl(const KUrl &url)
{
    KMimeType::Ptr mime = KMimeType::findByUrl(url, 0, true);
    QStringList formats = KImageIO::typeForMime(mime->name());
    int fcount = formats.count();			// get format from file name
    if (fcount==0)					// no MIME/Qt type found
    {
        kDebug() << "no format found for MIME type" << mime->name();
        return (QString::null);
    }
    if (fcount>1)					// more than one type found
    {
        kDebug() << "found" << fcount << "formats for MIME type" << mime->name();
    }

    return (formats.first());
}
