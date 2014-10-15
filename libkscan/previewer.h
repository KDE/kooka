/* This file is part of the KDE Project         -*- mode:c++; -*-
   Copyright (C) 2000 Klaas Freitag <freitag@suse.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef PREVIEWER_H
#define PREVIEWER_H

#include "libkscanexport.h"

#include <qvector.h>

#include <kvbox.h>
#include <kruler.h>

/**
  *@author Klaas Freitag
  */

class KScanDevice;

class ImageCanvas;
class AutoSelectBar;

class KSCAN_EXPORT Previewer : public KVBox
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
