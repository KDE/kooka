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

#ifndef GAMMADIALOG_H
#define GAMMADIALOG_H

#include "kookascan_export.h"

#include "dialogbase.h"

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

class KOOKASCAN_EXPORT GammaDialog : public DialogBase
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
    virtual ~GammaDialog()			{}

    /**
     * Get the internal gamma table.
     *
     * This is a deep copy of the table that was originally specified to
     * the constructor.
     *
     * @return The gamma table
     **/
    const KGammaTable *gammaTable() const	{ return (mTable); }

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
