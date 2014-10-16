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

#ifndef GAMMADIALOG_H
#define GAMMADIALOG_H

#include "kookascan_export.h"

#include <QDialog>

class KScanSlider;
class KGammaTable;
class GammaWidget;

/**
 * @short A dialogue to allow editing of a gamma table.
 *
 * The dialogue has three combination slider/spinboxes to set the
 * brightness, contrast and gamma of the table.  A preview of the table
 * is displayed to the right (in a @c GammaWidget).
 *
 * @author Klaas Freitag
 * @author Jonathan Marten
 **/

class KOOKASCAN_EXPORT GammaDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * Constructor.
     *
     * @param table Gamma table to take initial values from
     * @param parent Parent widget
     **/
    GammaDialog(const KGammaTable *table, QWidget *parent = NULL);

    /**
     * Destructor.
     **/
    virtual ~GammaDialog() {}

    /**
     * Get the internal gamma table.
     *
     * This is a deep copy of the table that was originally specified to
     * the constructor.
     *
     * @return The gamma table
     **/
    const KGammaTable *gammaTable() const
    {
        return (mTable);
    }

protected slots:
    /**
     * Apply the currently set values via the @c gammaToApply() signal.
     **/
    void slotApply();

    /**
     * Reset the gamma values to the default of a linear transfer function.
     **/
    void slotReset();

signals:
    /**
     * Emitted when the gamma table has been changed and one of the "OK" or
     * "Apply" buttons is clicked.
     *
     * @param table The new gamma table
     **/
    void gammaToApply(const KGammaTable *table);

private:
    KGammaTable *mTable;
    GammaWidget *mGtDisplay;

    KScanSlider *mSetBright;
    KScanSlider *mSetContrast;
    KScanSlider *mSetGamma;
};

#endif
