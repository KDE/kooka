/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 1999-2016 Klaas Freitag <freitag@suse.de>		*
 *                          Jonathan Marten <jjm@keelhaul.me.uk>	*
 *									*
 *  Kooka is free software; you can redistribute it and/or modify it	*
 *  under the terms of the GNU Library General Public License as	*
 *  published by the Free Software Foundation and appearing in the	*
 *  file COPYING included in the packaging of this file;  either	*
 *  version 2 of the License, or (at your option) any later version.	*
 *									*
 *  As a special exception, permission is given to link this program	*
 *  with any version of the KADMOS OCR/ICR engine (a product of		*
 *  reRecognition GmbH, Kreuzlingen), and distribute the resulting	*
 *  executable without including the source code for KADMOS in the	*
 *  source distribution.						*
 *									*
 *  This program is distributed in the hope that it will be useful,	*
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of	*
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*
 *  GNU General Public License for more details.			*
 *									*
 *  You should have received a copy of the GNU General Public		*
 *  License along with this program;  see the file COPYING.  If		*
 *  not, see <http://www.gnu.org/licenses/>.				*
 *									*
 ************************************************************************/

#include "scanimage.h"

#include <qdebug.h>
#include <qfile.h>

#include <klocalizedstring.h>

#include "imageformat.h"

#ifdef HAVE_TIFF
extern "C"
{
#include <tiffio.h>
#include <tiff.h>
}
#endif


ScanImage::ScanImage()
    : QImage()
{
    init();
}


ScanImage::ScanImage(int width, int height, QImage::Format format)
    : QImage(width, height, format)
{
    init();
}


ScanImage::ScanImage(const QImage &img)
    : QImage(img)
{
    init();
}


ScanImage::~ScanImage()
{
}


void ScanImage::init()
{
    m_subImages = 0;
}


ScanImage &ScanImage::operator=(const ScanImage &img)
{
    if (this != &img) {					// ignore self-assignment
        QImage::operator=(img);

        m_subImages = img.m_subImages;
        m_url       = img.m_url;
    }

    return (*this);
}


QUrl ScanImage::url() const
{
    return (m_url);
}


bool ScanImage::isFileBound() const
{
    return (m_url.isValid());
}


// TODO: only used immediately after construction in ScanGallery,
// implement constructor taking a QUrl
QString ScanImage::loadFromUrl(const QUrl &url)
{
    bool ret = true;					// return status
    bool isTiff = false;				// do we have a TIFF file?
    bool haveTiff = false;				// can it be read via TIFF lib?

    if (!url.isLocalFile()) return (i18n("Loading non-local images is not yet implemented"));

    if (url.hasFragment())				// is this a subimage?
    {
        int subno = url.fragment().toInt();		// get subimage number
        if (subno>0)					// valid number from fragment
        {						// get local file without fragment
            const QString fileName = url.adjusted(QUrl::RemoveFragment).toLocalFile();
            qDebug() << "subimage" << subno << "from" << fileName;
            loadTiffDir(fileName, subno);		// load TIFF subimage
            return (QString());
        }

    }

    const QString filename = url.toLocalFile();		// local path of image
    ImageFormat format = ImageFormat::formatForUrl(url);

    // If the file is TIFF and QImageReader supports TIFF files, 'format' as
    // returned by ImageFormat::formatForUrl() will be "TIF".  If TIFF files
    // are not supported but the MIME type of the file says that it is TIFF,
    // then 'format' will be "TIFFLIB".  So either of these format names means
    // that a TIFF file is being loaded.
    isTiff = format.isTiff();
    //qDebug() << "Image format to load:" << format << "from file" << filename;

#ifdef HAVE_TIFF
    if (isTiff)						// if it is TIFF, check
    {							// for multiple images
        qDebug() << "Checking for multi-page TIFF";
        TIFF *tif = TIFFOpen(QFile::encodeName(filename).constData(), "r");
        if (tif!=nullptr)
        {
            do
            {
                ++m_subImages;
            } while (TIFFReadDirectory(tif));
            qDebug() << "found" << m_subImages << "TIFF directories";
            if (m_subImages>1) haveTiff = true;
            // This format will have been specially detected
            // in ImageFormat::formatForMime(), if the file is TIFF
            // but QImageReader does not have TIFF support.
            // The TIFF file can still be read by loadTiffDir() below.
            else if (m_subImages==1 && format==ImageFormat("TIFFLIB")) haveTiff = true;
            TIFFClose(tif);
        }
    }
#endif
    if (!haveTiff)					// not TIFF, or not multi-page
    {
        ret = QImage::load(filename);			// Qt can only read one image
        if (!ret) return (i18n("Image loading failed"));
        m_subImages = 0;
    }
#ifdef HAVE_TIFF
    else						// a TIFF file, multi-page or not
    {
        loadTiffDir(filename, 0);			// read by TIFFlib directly
    }
#endif

    m_url = url;					// record image source as from file
    return (QString());					// loaded OK
}


QString  ScanImage::loadTiffDir(const QString &filename, int subno)
{
#ifdef HAVE_TIFF
    // if it is TIFF, check with TIFFlib if it is multiple images
    qDebug() << "Trying to load TIFF, subimage" << subno;
    TIFF *tif = TIFFOpen(QFile::encodeName(filename).constData(), "r");
    if (tif==nullptr) return (i18n("Failed to open TIFF file"));

    if (subno>0)						// want a subimage
    {
        if (!TIFFSetDirectory(tif, subno-1))
        {
            TIFFClose(tif);
            return (i18n("Failed to read TIFF directory"));
        }
    }

    int imgWidth, imgHeight;
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
    // its constructor and cannot be subsequently changed.  Instead we read
    // the TIFF file into a temporary image and then use QImage::operator=
    // to shallow copy that temporary image into ours.  After that temporary
    // image has been destroyed, we have the only copy of the TIFF read image.
    //
    QImage tmpImg(imgWidth, imgHeight, QImage::Format_RGB32);
    uint32 *data = (uint32 *)(tmpImg.bits());
    if (TIFFReadRGBAImage(tif, imgWidth, imgHeight, data, 0)) {
							// Successfully read, now convert
        tmpImg = tmpImg.rgbSwapped();			// swap red and blue
        tmpImg = tmpImg.mirrored();			// reverse (it's upside down)
    } else {
        TIFFClose(tif);
        return (i18n("Failed to read TIFF image"));
    }

    // Fetch the x- and y-resolutions to adjust images
    float xReso, yReso;
    bool resosFound = TIFFGetField(tif, TIFFTAG_XRESOLUTION, &xReso) &&
                      TIFFGetField(tif, TIFFTAG_YRESOLUTION, &yReso);
    //qDebug() << "TIFF image: X-Res" << xReso << "Y-Res" << yReso;

    TIFFClose(tif);					// finished with TIFF file

    // Check now if resolution in x- and y-directions differ.
    // If so, stretch the image accordingly.
    if (resosFound && xReso != yReso) {
        if (xReso > yReso) {
            float yScalefactor = xReso / yReso;
            //qDebug() << "Different resolution X/Y, rescaling Y with factor" << yScalefactor;
            /* rescale the image */
            tmpImg = tmpImg.scaled(imgWidth, int(imgHeight * yScalefactor),
                                   Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        } else {
            float xScalefactor = yReso / xReso;
            //qDebug() << "Different resolution X/Y, rescaling X with factor" << xScalefactor;
            tmpImg = tmpImg.scaled(int(imgWidth * xScalefactor), imgHeight,
                                   Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        }
    }

    QImage::operator=(tmpImg);				// copy as our image
#else
    return (i18n("TIFF not supported"));
#endif							// HAVE_TIFF
    return (QString());					// TIFF read succeeded
}


int ScanImage::subImagesCount() const
{
    return (m_subImages);
}


bool ScanImage::isSubImage() const
{
    return (m_url.isValid() && m_url.fragment().toInt()>0);
}
