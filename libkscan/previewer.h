/* This file is part of the KDE Project			-*- mode:c++; -*-
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

#include <qwidget.h>
#include <qvector.h>

#include <kruler.h>

/**
  *@author Klaas Freitag
  */
class QLabel;
class QGroupBox;
class QSlider;
class QComboBox;

class KScanDevice;

class ImageCanvas;
class SizeIndicator;


class KSCAN_EXPORT Previewer : public QWidget
{
    Q_OBJECT

public:
    Previewer(QWidget *parent = NULL);
    virtual ~Previewer();

    ImageCanvas *getImageCanvas() const		{ return (mCanvas); }

    bool setPreviewImage(const QImage &image);
    void newImage(const QImage *image);
    void setScannerBedSize(int w,int h);
    void setDisplayUnit(KRuler::MetricStyle unit);
    void connectScanner(KScanDevice *scan);

public slots:
    void slotNewAreaSelected(const QRect &rect);
    void slotNewScanResolutions(int xres,int yres);
    void slotNewScanMode(int bytes_per_pix);
    void slotSetAutoSelThresh(int);
    void slotSetAutoSelDustsize(int);

    void slotNewCustomScanSize(const QRect &rect);

protected slots:
    void slotAutoSelToggled(bool);
    void slotScanBackgroundChanged(int);

signals:
    void newPreviewRect(const QRect &rect);

private:
    bool checkForScannerBg();
    void setScannerBgIsWhite(bool isWhite);

    void updateSelectionDims();

    void findAutoSelection();
    void resetAutoSelection();
    void setAutoSelection(bool on);
    bool imagePiece(const QVector<long> &src, int *start, int *end);

private:
    ImageCanvas *mCanvas;
    QImage mPreviewImage;

    int mBedWidth,mBedHeight;
    int mScanResX,mScanResY;
    int mBytesPerPix;
    double mSelectionWidthMm,mSelectionHeightMm;
    KRuler::MetricStyle mDisplayUnit;

    QLabel *mSelSizeLabel1;
    QLabel *mSelSizeLabel2;
    SizeIndicator *mFileSizeLabel;
    QSlider *mSliderThresh;
    QSlider *mSliderDust;
    QComboBox *mBackgroundCombo;
    QGroupBox *mAutoSelGroup;

    KScanDevice *mScanDevice;

    bool mDoAutoSelection;
    int mAutoSelThresh;
    int mAutoSelDustsize;
    bool mBgIsWhite;

    QVector<long> mHeightSum;
    QVector<long> mWidthSum;
};

#endif							// PREVIEWER_H
