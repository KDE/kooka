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

    void setOptions(const QMap<QString, QString> *opts);


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


// TODO: do these need to be slots?
public slots:
    bool printImage(const KookaImage *img, int intextraMarginPercent = 10);
    void printFittingToPage(const KookaImage *img);

protected:
    typedef enum { SW, NW, NO, SO } MarkerDirection;

    virtual void drawMarkerAroundPoint(const QPoint &);
    virtual void drawCutSign(const QPoint &, int, MarkerDirection);
    virtual void drawCornerMarker(const QSize &, int, int, int, int);

private:
    /**
     * The pixel size of the current page.
     **/
    QSize maxPageSize(int extraShrinkPercent = 0) const;

private:
    QPainter *m_painter;
    const QMap<QString, QString> *m_options;

    QSize m_maxPageSize;
    int m_extraMarginPercent;
};

#endif							// KOOKAPRINT_H
