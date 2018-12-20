/* This file is part of the KDE Project
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

#ifndef SIZEINDICATOR_H
#define SIZEINDICATOR_H

#include "kookascan_export.h"

#include <qlabel.h>

#define DEFAULT_SMALL       (1024)          // 1Kb
#define DEFAULT_THRESHOLD   (1*1024*1024)       // 1Mb
#define DEFAULT_CRITICAL    (4*1024*1024)       // 4Mb

/**
 *  @short  small size indication widget for file sizes
 *  @author Klaas Freitag
 *
 *  the size indicator is a small widget that displays a file size in a small
 *  frame. The unit (currently kB and MB) is selected automagically.
 *  If the file size grows bigger than the threshold set in the constructor,
 *  the widget starts to change its background color to visualise to the
 *  user that he is doing something obvious.
 */

class KOOKASCAN_EXPORT SizeIndicator : public QLabel
{
    Q_OBJECT

public:
    /**
     *  Creates a size indicator widget.
     *
     *  @param thres Threshold size, above which the widget starts to turn the warning colour.
     *  @param crit Critical size, above which the widget starts to turn the error colour.
     */
    explicit SizeIndicator(QWidget *parent,
                  long thresh = DEFAULT_THRESHOLD,
                  long crit = DEFAULT_CRITICAL);

    ~SizeIndicator() override;

    /**
     * Sets the threshold size.
     *
     * @param thres the threshold size.
     */
    void setThreshold(long thresh = DEFAULT_THRESHOLD);

    /**
     * Sets the critical size.
     *
     * @param crit the critical size.
     */
    void setCritical(long crit = DEFAULT_CRITICAL);

public slots:
    /**
     * Sets the file size to display. The widget gets updated.
     *
     * @param newSize the size to set.
     */
    void setSizeInByte(long newSize);

protected:
    void paintEvent(QPaintEvent *ev) override;

private:
    long mThreshold;
    long mCritical;
    long mSizeInByte;

    class SizeIndicatorPrivate;
    SizeIndicatorPrivate *d;
};

#endif                          // SIZEINDICATOR_H
