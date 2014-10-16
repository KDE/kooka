/* This file is part of the KDE Project
   Copyright (C) 2002 Klaas Freitag <freitag@suse.de>

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

#ifndef KGAMMATABLE_H
#define KGAMMATABLE_H

#include "kookascan_export.h"

#include <qobject.h>
#include <qvector.h>

/**
 * @short A gamma table.
 *
 * The gamma table maps input intensity values to output values.
 * The transfer function is defined by three parameters gamma, brightness
 * and contrast.  These values are retained internally.
 *
 * When the data from the table is required, either for display or for
 * sending to the scanner, it is calculated and made available as a vector
 * of integers.  Each of these values is in the range 0..(valueRange-1).
 *
 * The @c gamma values specified and returned are expressed as a
 * percentage, so 100 is a linear transfer function.  Reasonable values
 * for the gamma are within the range 30..300 (corresponding to
 * "conventional" gamma values of 0.3..3.0).
 *
 * Reasonable values for the @c brightness and @c contrast are within
 * the range -50..+50.
 *
 * @author Klaas Freitag
 * @author Jonathan Marten
 **/

class KOOKASCAN_EXPORT KGammaTable : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int g READ getGamma WRITE setGamma)
    Q_PROPERTY(int c READ getContrast WRITE setContrast)
    Q_PROPERTY(int b READ getBrightness WRITE setBrightness)

public:
    /**
     * Constructor.
     *
     * Create a new gamma table object, with default parameters or
     * as specified.
     *
     * @param gamma Initial gamma value
     * @param brightness Initial brightness value
     * @param contrast Initial contrast value
     *
     * @note The data array is not allocated at this point.
     **/
    explicit KGammaTable(int gamma = 100, int brightness = 0, int contrast = 0);

    /**
     * Copy constructor.
     *
     * @param other Gamma table to be copied
     *
     * @note Only the gamma/brightness/contrast parameters are copied,
     * the data array is neither copied nor allocated.
     **/
    KGammaTable(const KGammaTable &other);

    /**
     * Set all of the gamma table parameters in one operation.
     *
     * @param gamma New gamma value
     * @param brightness New brightness value
     * @param contrast New contrast value
     **/
    void setAll(int gamma, int brightness, int contrast);

    /**
     * Set the gamma table parameters from a string representation.
     *
     * @param String value, in the form "gamma,brightness,contrast"
     * where each parameter is a decimal integer.
     *
     * @return @c true if the string format is valid.  If it is not
     * valid, then @c false is returned and the gamma table parameters
     * will not have been changed.
     *
     * @see toString
     **/
    bool setFromString(const QString &str);

    /**
     * Convert the gamma table parameters to a string representation.
     *
     * @return The string representation, in the form "gamma,brightness,contrast".
     *
     * @see setFromString
     **/
    QString toString() const;

    /**
     * Get the current gamma value.
     *
     * @return The gamma value
     **/
    int getGamma() const
    {
        return (mGamma);
    }

    /**
     * Get the current brightness value.
     *
     * @return The brightness value
     **/
    int getBrightness() const
    {
        return (mBrightness);
    }

    /**
     * Get the current contrast value.
     *
     * @return The contrast value
     **/
    int getContrast() const
    {
        return (mContrast);
    }

    /**
     * Get the currently allocated table size.
     *
     * @return The number of entries in the gamma table.  If the table
     * has not yet been calculated (i.e. @c getTable() has never been
     * called) then the size returned will be zero.  Otherwise, the size
     * returned is the last size requested for @c getTable(), or 256
     * if no explicit size has been requested.
     *
     * @see getTable
     **/
    int tableSize() const
    {
        return (mData.size());
    }

    /**
     * Calculate the gamma table values.
     *
     * @param size Size of the table required.  If not specified, then
     * the previous size requested for the table is retained.  If no
     * explicit size has ever been requested, then 256 is assumed.
     *
     * @return A pointer to the array op values
     **/
    const int *getTable(int size = -1);

    /**
     * The range of values that will be found in the table.
     **/
    static const int valueRange = 256;

public slots:
    /**
     * Set a new brightness value.
     *
     * @param brightness New brightness value
     **/
    void setBrightness(int brightness);

    /**
     * Set a new contrast value.
     *
     * @param contrast New contrast value
     **/
    void setContrast(int contrast);

    /**
     * Set a new gamma value.
     *
     * @param gamma New gamma value
     **/
    void setGamma(int gamma);

signals:
    /**
     * Emitted when any of the table parameters (gamma, brightness or
     * contrast) have changed.
     **/
    void tableChanged();

private:
    void init();
    void calcTable();

    int mGamma;
    int mBrightness;
    int mContrast;
    bool mDirty;

    QVector<int> mData;
};

#endif                          // KGAMMATABLE_H
