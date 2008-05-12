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

#include <qwidget.h>
#include <qimage.h>
#include <qrect.h>

#include <kruler.h>

/**
  *@author Klaas Freitag
  */
class ImageCanvas;
class SizeIndicator;
class KScanDevice;
class QLabel;


class Previewer : public QWidget
{
    Q_OBJECT

public:
    Previewer(QWidget *parent = NULL);
    ~Previewer();

    ImageCanvas *getImageCanvas() const		{ return (img_canvas); }

    /**
     * Static function that returns the image gallery base dir.
     */
    static QString galleryRoot();

    bool setPreviewImage(const QImage &image);
    void newImage(const QImage *image);
    void setScannerBedSize(int w,int h);
    void setDisplayUnit(KRuler::MetricStyle unit);
    void connectScanner(KScanDevice *scan);

public slots:
    void slotNewAreaSelected(QRect r);
    void slNewScanResolutions(int xres,int yres);
    void slNewScanMode(int bytes_per_pix);
    void slSetAutoSelThresh(int);
    void slSetAutoSelDustsize(int);

    void slotNewCustomScanSize(QRect rect);

protected slots:
    void slScaleToWidth();
    void slScaleToHeight();
    void slAutoSelToggled(bool);
    void slScanBackgroundChanged(int);

signals:
    void newPreviewRect(QRect rect);

private:
    void checkForScannerBg();
    void setScannerBgIsWhite(bool isWhite);

    void updateSelectionDims();
    void findSelection();

    bool imagePiece(QMemArray<long> src,int &start,int &end);

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

#endif
