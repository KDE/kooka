/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2000-2016 Klaas Freitag <freitag@suse.de>		*
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

#ifndef PREVIEWER_H
#define PREVIEWER_H

#include "kookascan_export.h"

#include <qvector.h>
#include <qframe.h>

#include <kruler.h>

/**
  *@author Klaas Freitag
  */

class KScanDevice;

class ImageCanvas;
class AutoSelectBar;


class KOOKASCAN_EXPORT Previewer : public QFrame
{
    Q_OBJECT

public:
    Previewer(QWidget *parent = NULL);
    virtual ~Previewer();

    ImageCanvas *getImageCanvas() const
    {
        return (mCanvas);
    }

    bool setPreviewImage(const QImage &image);
    void newImage(const QImage *image);
    void setScannerBedSize(int w, int h);
    void setDisplayUnit(KRuler::MetricStyle unit);
    void connectScanner(KScanDevice *scan);

    static QString previewInfoString(double widthMm, double heightMm, int resX, int resY);

public slots:
    void slotNewAreaSelected(const QRectF &rect);
    void slotNewScanResolutions(int xres, int yres);
    void slotNewScanMode(int bytes_per_pix);
    void slotSetAutoSelThresh(int);
    void slotAutoSelToggled(bool);

    void slotNewCustomScanSize(const QRect &rect);

protected slots:
    void slotAutoSelectSettingsChanged(int margin, bool bgIsWhite, int dustsize);

signals:
    void newPreviewRect(const QRect &rect);
    void autoSelectStateChanged(bool isAvailable, bool isOn);
    void previewDimsChanged(const QString &dims);
    void previewFileSizeChanged(long size);

private:
    bool checkForScannerBg();

    void updateSelectionDims();

    void resetAutoSelection();
    void setAutoSelection(bool on);
    bool imagePiece(const QVector<long> &src, int *start, int *end);

private slots:
    void slotNotifyAutoSelectChanged();
    void slotFindAutoSelection();

private:
    ImageCanvas *mCanvas;
    QImage mPreviewImage;

    int mBedWidth, mBedHeight;
    int mScanResX, mScanResY;
    int mBytesPerPix;
    double mSelectionWidthMm, mSelectionHeightMm;
    KRuler::MetricStyle mDisplayUnit;

    AutoSelectBar *mAutoSelectBar;

    KScanDevice *mScanDevice;

    bool mDoAutoSelection;
    int mAutoSelMargin;
    int mAutoSelThresh;
    int mAutoSelDustsize;
    bool mBgIsWhite;

    QVector<long> mHeightSum;
    QVector<long> mWidthSum;
};

#endif                          // PREVIEWER_H
