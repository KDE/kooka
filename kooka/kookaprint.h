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

class QPainter;
class KookaImage;


class KookaPrint : public QPrinter
{
public:
    explicit KookaPrint();

    bool printImage(int intextraMarginPercent = 10);

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

    void setImage(const KookaImage *img)		{ m_image = img; }
    const KookaImage *image() const			{ return (m_image); }

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




    /**
     * The top left edge of the required print position
     */
// TODO: no need for virtual
// TODO: no need to be public either
    virtual QPoint printPosTopLeft(const QSize &) const;
    virtual QPoint printPosTopRight(const QSize &) const;
    virtual QPoint printPosBottomLeft(const QSize &) const;
    virtual QPoint printPosBottomRight(const QSize &) const;

    virtual int extraMarginPix() const;

protected:
    typedef enum { SW, NW, NO, SO } MarkerDirection;

    virtual void drawMarkerAroundPoint(const QPoint &);
    virtual void drawCutSign(const QPoint &, int, MarkerDirection);
    virtual void drawCornerMarker(const QSize &, int, int, int, int);

private:
    QPainter *m_painter;

    QSize m_maxPageSize;
    int m_extraMarginPercent;
    const KookaImage *m_image;

    KookaPrint::ScaleOption m_scaleOption;
    KookaPrint::CutMarksOption m_cutsOption;
    QSize m_printSize;
    bool m_maintainAspect;
    bool m_lowResDraft;
    int m_screenResolution;
    int m_scanResolution;
};

#endif							// KOOKAPRINT_H
