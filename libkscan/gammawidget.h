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

#ifndef GAMMAWIDGET_H
#define GAMMAWIDGET_H

#include "libkscanexport_export.h"

#include <qwidget.h>
#include <qsizepolicy.h>
#include <qsize.h>

class QPaintEvent;
class KGammaTable;

/**
 * @short A widget to display a gamma table.
 *
 * The table is displayed in a square, regardless of the aspect ratio
 * of the widget, and aligned to the top left.  Grid lines are
 * automatically displayed.
 *
 * The display is redrawn whenever the associated gamma table changes.
 *
 * @author Klaas Freitag
 * @author Jonathan Marten
 **/

class LIBKSCANEXPORT_EXPORT GammaWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * Constructor.
     *
     * @param table The gamma table to display
     * @param parent Parent widget
     **/
    GammaWidget(KGammaTable *table, QWidget *parent = NULL);

    /**
     * Destructor.
     **/
    virtual ~GammaWidget() {}

    /**
     * Reimplemented
     **/
    QSize sizeHint() const;

protected:
    /**
     * Reimplemented
     **/
    void paintEvent(QPaintEvent *ev);

private:
    KGammaTable *mTable;
};

#endif                          // GAMMAWIDGET_H
