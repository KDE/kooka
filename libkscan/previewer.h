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
#include <qrect.h>
#include <qvector.h>

#include <kruler.h>

/**
  *@author Klaas Freitag
  */
class QLabel;

class KScanDevice;

class ImageCanvas;
class SizeIndicator;


class KSCAN_EXPORT Previewer : public QWidget
{
    Q_OBJECT

public:
    Previewer(QWidget *parent = NULL);
    ~Previewer();

    ImageCanvas *getImageCanvas() const		{ return (img_canvas); }

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
    void findSelection();

    bool imagePiece(QVector<long> src,int &start,int &end);

    QLabel *selSize1;
    QLabel *selSize2;
    SizeIndicator *fileSize;

    ImageCanvas *img_canvas;
    QImage m_previewImage;

    KRuler::MetricStyle displayUnit;
    int bedWidth,bedHeight;

    int  scanResX, scanResY;
    int  m_bytesPerPix;
    double selectionWidthMm;
    double selectionHeightMm;

    class PreviewerPrivate;
    PreviewerPrivate *d;
};

#endif							// PREVIEWER_H
