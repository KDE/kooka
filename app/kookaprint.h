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

#ifndef KOOKAPRINT_H
#define KOOKAPRINT_H

#include <qprinter.h>

#include "kookacore_export.h"

class QPainter;
// For rigorousness, the image to be printed should be passed to us and
// stored as a ScanImage::Ptr (a QSharedPointer to a ScanImage).  Unfortunately,
// having such a pointer as a member would need to include "scanimage.h", which
// in turn means that the KCFG settings (which needs to include "kookaprint.h"
// but then cannot find "scanimage.h") will not build.
//
// Since KookaPrint only uses the image for printing and does not need to copy
// or modify it, it should be safe to store a pointer to the image here.
class QImage;


class KOOKACORE_EXPORT KookaPrint : public QPrinter
{
public:
    explicit KookaPrint();

    enum ScaleOption
    {
        ScaleScreen,					///< As seen on screen
        ScaleScan,					///< From scan resolution
        ScaleCustom,					///< Custom size
        ScaleFitPage					///< Fit to page
    };

    enum CutMarksOption
    {
        CutMarksNone,					///< No cut marks
        CutMarksMultiple,				///< Cut marks if multiple pages
        CutMarksAlways					///< Always cut marks
    };

    void recalculatePrintParameters();
    void printImage();

    void setImage(const QImage *img)			{ m_image = img; }
    void setCopyMode(bool on);
    bool isCopyMode() const				{ return (m_copyMode); }

    void setScaleOption(KookaPrint::ScaleOption opt)	{ m_scaleOption = opt; }
    KookaPrint::ScaleOption scaleOption() const		{ return (m_scaleOption); }

    void setPrintSize(const QSize &opt)			{ m_printSize = opt; }
    QSize printSize() const				{ return (m_printSize); }

    void setMaintainAspect(bool opt)			{ m_maintainAspect = opt; }
    bool maintainAspect() const				{ return (m_maintainAspect); }

    void setLowResDraft(bool opt)			{ m_lowResDraft = opt; }
    bool lowResDraft() const				{ return (m_lowResDraft); }

    void setScreenResolution(int res)			{ m_screenResolution = res; }
    int screenResolution() const			{ return (m_screenResolution); }

    void setScanResolution(int res)			{ m_scanResolution = res; }
    int scanResolution() const				{ return (m_scanResolution); }

    void setCutMarks(KookaPrint::CutMarksOption opt)	{ m_cutsOption = opt; }
    KookaPrint::CutMarksOption cutMarksOption() const	{ return (m_cutsOption); }

    QSize availablePageArea() const			{ return (QSize(qRound(mPageWidthAdjustedMm), qRound(mPageHeightAdjustedMm))); }
    QSize imagePrintArea() const			{ return (QSize(qRound(mPrintWidthMm), qRound(mPrintHeightMm))); }
    QSize pageCount() const				{ return (QSize(mPrintColumns, mPrintRows)); }

protected:
    void drawMarkerAroundPoint(QPainter *painter, const QPoint &p);
    void drawCutSign(QPainter *painter, const QPoint &p, int num, Qt::Corner dir);
    void drawCornerMarkers(QPainter *painter, const QRect &targetRect, int row, int col, int maxRows, int maxCols);

private:
    const QImage *m_image;

    KookaPrint::ScaleOption m_scaleOption;
    KookaPrint::CutMarksOption m_cutsOption;
    QSize m_printSize;
    bool m_maintainAspect;
    bool m_lowResDraft;
    int m_screenResolution;
    int m_scanResolution;

    int mImageWidthPix;					// pixel size of the image
    int mImageHeightPix;
    double mPrintWidthMm;				// print size of the image
    double mPrintHeightMm;
    double mPageWidthMm;				// print area available on page
    double mPageHeightMm;
    double mPageWidthAdjustedMm;			// print area used on page
    double mPageHeightAdjustedMm;
    int mPrintRows;					// rows/columns required
    int mPrintColumns;
    int mPrintTopPix;					// pixel position of origin
    int mPrintLeftPix;

    int mPrintResolution;				// printer resolution

    bool m_copyMode;
};

#endif							// KOOKAPRINT_H
