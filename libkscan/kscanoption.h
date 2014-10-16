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

#ifndef KSCANOPTION_H
#define KSCANOPTION_H

#include "libkscanexport_export.h"

#include <qobject.h>
#include <qbytearray.h>

#include "kscandevice.h"

extern "C" {
#include <sane/sane.h>
}

class QLabel;

class KGammaTable;
class KScanControl;
class KScanDevice;

/**
 * @short A single scanner parameter.
 *
 * A scanner may support any number of these parameters, some are are
 * well-known and common to almost all scanners while others may be
 * model-specific.
 *
 * Most options have an associated GUI element (a @c KScanControl), precisely
 * which sort of control depending on the type and constraint of the
 * scanner parameter.
 *
 * Only one KScanOption for each scanner parameter may exist.  All options
 * for a particular scanner are owned by a KScanDevice, and options may only
 * be created by KScanDevice::getOption().  This ensures that all accesses
 * to a scanner parameter are consistent.
 *
 * KScanOption implements an internal memory buffer as an intermediary between
 * the scanner and its caller.  There are four basic operations implemented to
 * access and control the scanner parameters:
 *
 * - @c set    - Copy data from a variable or structure of an appropriate type
 *               into the internal memory buffer.
 *
 * - @c apply  - Send the data from the internal memory buffer to the scanner.
 *
 * - @c reload - Fetch the scanner data into the internal memory buffer.
 *
 * - @c get    - Read the data from the internal memory buffer into a variable
 *               or structure of an appropriate type.
 *
 * @author Klaas Freitag
 * @author Jonathan Marten
 **/

class LIBKSCANEXPORT_EXPORT KScanOption : public QObject
{
    Q_OBJECT
public:

    /**
     * The type of an associated GUI widget (if there is one).
     **/
    enum WidgetType {
        Invalid,
        Bool,
        SingleValue,
        Range,
        GammaTable,
        StringList,
        String,
        Resolution,
        File,
        Group,
        Button
    };

    /**
     * Check whether the option is valid: that is, the parameter is known
     * by the scanner.
     *
     * @return @c true if the option is valid
     **/
    bool isValid() const
    {
        return (mDesc != NULL);
    }

    /**
     * Check whether the option is initialised: that is, if the initial
     * value of the parameter has been read from the scanner.
     *
     * @return @c true if the option has been initialised
     **/
    bool isInitialised() const
    {
        return (!mBufferClean);
    }

    /**
     * Check whether the option is a group, if so this is a title only
     * and many of the operations will not be available.
     *
     * @return @c true if the option is a group
     **/
    bool isGroup() const
    {
        return (mIsGroup);
    }

    /**
     * Check whether the option value can be read from the scanner
     * (SANE_CAP_SOFT_SELECT with some special exceptions).  Some
     * options that cannot be read may still be able to be set,
     * use @c isSoftwareSettable() to check.
     *
     * @return @c true if the option is readable
     * @see isSoftwareSettable
     **/
    bool isReadable() const
    {
        return (mIsReadable);
    }

    /**
     * Check whether the option is auto settable (SANE_CAP_AUTOMATIC):
     * that is, if the scanner can choose a setting for the option
     * automatically.
     *
     * @return @c true if the option can be set automatically
     **/
    bool isAutoSettable() const;

    /**
     * Check whether the option is a common option (not SANE_CAP_ADVANCED).
     *
     * @return @c true if the option is a common option,
     *         @c false if it is an advanced option.
     **/
    bool isCommonOption() const;

    /**
     * Check whether the option is currently active (SANE_OPTION_IS_ACTIVE).
     * This may change at runtime depending on the settings of other options.
     *
     * @return @c true if the option is currently active
     **/
    bool isActive() const;

    /**
     * Check whether the option is can be set by software
     * (SANE_OPTION_IS_SETTABLE).  Some scanner options cannot be set,
     * use @c isReadable() to check if they can be read.
     *
     * @return @c true if the option can be set
     * @see isReadable
     **/
    bool isSoftwareSettable() const;

    /**
     * Check whether the option has an associated GUI element
     * (not all types of options do).
     *
     * @return @c true if the option has a GUI widget
     **/
    bool isGuiElement() const
    {
        return (mControl != NULL);
    }

    /**
     * Check whether the option value has been sent to the scanner:
     * that is, whether @c apply() has been used.  This is used by
     * @c KScanDevice to maintain its list of "dirty" options.
     *
     * @return @c true if the option has been applied
     * @see KScanDevice
     * @see setApplied
     **/
    bool isApplied() const
    {
        return (mApplied);
    }

    /**
     * Set or clear the "applied" flag.
     *
     * @param app New value for the flag
     * @see isApplied
     * @see apply
     **/
    void setApplied(bool app = true)
    {
        mApplied = app;
    }

    /**
     * Get the widget type for the option.  This is deduced from the
     * scanner parameter type and constraint.
     *
     * @return the widget type
     **/
    KScanOption::WidgetType type() const;

    /**
     * Set the option value.
     *
     * @param val A new integer value
     * @return @c true if the value was set successfully
     **/
    bool set(int val);

    /**
     * Set the option value.
     *
     * @param val A new @c double floating point value
     * @return @c true if the value was set successfully
     **/
    bool set(double val);

    /**
     * Set the option value.
     *
     * @param val A new array of integer values
     * @param size The length of the array
     * @return @c true if the value was set successfully
     **/
    bool set(const int *val, int size);

    /**
     * Set the option value.
     *
     * @param val A new formatted string value
     * @return @c true if the value was set successfully
     **/
    bool set(const QByteArray &val);

    /**
     * Set the option value.
     *
     * @param val A new boolean value
     * @return @c true if the value was set successfully
     **/
    bool set(bool val)
    {
        return (set(val ? 1 : 0));
    }

    /**
     * Set the option value.
     *
     * @param gt A new gamma table
     * @return @c true if the value was set successfully
     **/
    bool set(const KGammaTable *gt);

    /**
     * Retrieve the option value.
     *
     * @param val An integer to receive the value read
     * @return @c true if the value was read successfully
     *
     * @note If the scanner parameter is an array, only the first value
     * is retrieved from it.
     **/
    bool get(int *val) const;

    /**
     * Retrieve the option value.
     *
     * @param gt A gamma table to receive the value read
     * @return @c true in all cases (unless the @p gt parameter is @c NULL)
     **/
    bool get(KGammaTable *gt) const;

    /**
     * Retrieve the option value.
     *
     * @return The formatted string value, or @c "?" if the value could
     * not be read.
     **/
    QByteArray get() const;

    /**
     * Retrieve a list of all possible option values.
     *
     * @return A list of formatted string values (as would be returned
     * by @c get())
     *
     * @note This works for options with SANE_CONSTRAINT_STRING_LIST,
     * SANE_CONSTRAINT_WORD_LIST and for a SANE_CONSTRAINT_RANGE which is a
     * resolution setting.  To retrieve the range for other constraint
     * types, use @c getRange().
     **/
    QList<QByteArray> getList() const;

    /**
     * Retrieve the range of possible numeric values.
     *
     * @param minp A @c double to receive the minimum value
     * @param maxp A @c double to receive the maximum value
     * @param quantp A @c double to receive the step, if it is not @c NULL.
     * @return @c true if the option is a range type
     *
     * @note For an option with SANE_CONSTRAINT_WORD_LIST, the minimum
     * and maximum values are those found in the word list and the step
     * is the range divided by the number of possible values.  This does
     * not imply that any intermediate values calculated from these are valid.
     **/
    bool getRange(double *minp, double *maxp, double *quantp = NULL) const;

    /**
     * Send the data (previously set by @c set()) to the scanner, if this
     * is possible - if the option is initialised, software settable and
     * active.
     *
     * @return @c true if a reload of other parameters is required
     * (@c sane_control_option() returned SANE_INFO_RELOAD_OPTIONS).
     **/
    bool apply();

    /**
     * Retrieve the current data from the scanner, so that it can be
     * accessed by @c get().  If the option has an associated GUI control,
     * the enabled/disabled state of that is set appropriately.
     **/
    void reload();

    /**
     * Create a GUI widget for the scanner option, depending on its type.
     *
     * - Boolean options have a check box (a @c KScanCheckbox).
     * - Numeric ranges have a slider/spinbox combination (a @c KScanSlider),
     *   except for the special case of a resolution option which generates
     *   a combo box (@c KScanCombo) - this is done for user convenience.
     * - String and numeric lists generate a combo box (@c KScanCombo).
     * - Unconstrained string and numeric options generate an entry box
     *   (@c KScanStringEntry or @c KScanNumberEntry respectively).
     * - An option which is a file name (present in the SANE "pnm" test device)
     *   generates a file requester (@c KScanFileRequester).
     * - Group options generate a separator line (@c KScanGroup), although
     *   obviously no interaction is allowed.
     * - Button options generate a clickable button (@c KScanPushButton).
     *
     * Ownership of the widget is retained by the @c KScanOption object, so it
     * should not be deleted by the caller.
     *
     * @param parent Parent for the created widget.
     * @return The created widget, or @c NULL if it could not be created.
     **/
    KScanControl *createWidget(QWidget *parent);

    /**
     * Create a label widget for the scanner option.  @c createWidget() must
     * have been used to create the control first.
     *
     * If the option is a common option (i.e. not advanced), then the label's
     * buddy will be set to the control.

     * @param parent Parent widget for the created label.
     * @param alwaysBuddy If this is @c true, the label's buddy will always
     * be set even if this is an advanced option.
     * @return The created label widget.
     **/
    QLabel *getLabel(QWidget *parent, bool alwaysBuddy = false) const;

    /**
     * Create a label widget for the SANE unit of this option.
     * @c createWidget() must have been used to create the control first.
     *
     * @param parent Parent widget for the created label.
     * @return The created label widget, or @c NULL if no unit is
     * applicable.
     **/
    QLabel *getUnit(QWidget *parent) const;

    /**
     * Get the name of the option.
     *
     * @return The name of the option
     **/
    QByteArray getName() const
    {
        return (mName);
    }

    /**
     * Get the GUI widget for the option, if applicable and one has been
     * created by @c createWidget()).
     *
     * @return The widget, or @c NULL if there is none.
     **/
    KScanControl *widget() const
    {
        return (mControl);
    }

    /**
     * Update the GUI widget to reflect the current value of the option.
     **/
    void redrawWidget();

protected slots:
    /**
     * Called when the contents of the GUI widget has changed, if the
     * option has a GUI element (not all have).  This slot is used for
     * options that have a @c KScanControl of type @c KScanControl::Text.
     *
     * @param t New string contents of the widget
     **/
    void slotWidgetChange(const QString &t);

    /**
     * Called when the contents of the GUI widget has changed, if the
     * option has a GUI element (not all have).  This slot is used for
     * options that have a @c KScanControl of type @c KScanControl::Number.
     *
     * @param i New index or setting of the widget
     **/
    void slotWidgetChange(int i);

    /**
     * Called when a GUI widget is activated, if the option has a GUI
     * element (not all have).  This slot is used for options that have
     * a @c KScanControl of type @c KScanControl::Button.
     **/
    void slotWidgetChange();

signals:
    /**
     * Emitted when the user changes the GUI setting of the option.
     * The new setting will have been @c set(), but not yet @c apply()'ed.
     *
     * @param so The @c KScanOption which has changed
     **/
    void guiChange(KScanOption *so);

private:
    /**
     * Create a new object for the named option belonging to the
     * specified scanner device. After construction, if the option
     * is valid it will initally contain the current parameter value
     * retrieved from the scanner.
     *
     * @param name Name of the scanner option
     * @param scandev Scanner device
     *
     * @note This constructor is private.  All @c KScanOption's are
     * created by @c KScanDevice; a new or existing @c KScanOption can
     * be obtained via @c KScanDevice::getOption().
     **/
    KScanOption(const QByteArray &name, KScanDevice *scandev);
    friend KScanOption *KScanDevice::getOption(const QByteArray &, bool);

    /**
     * Destructor.
     *
     * @note This destructor is private.  All @c KScanOption's are
     * owned by @c KScanDevice, and are deleted when the scan device
     * is closed.
     **/
    ~KScanOption();
    friend void KScanDevice::closeDevice();

    bool initOption(const QByteArray &name);
    void allocForDesc();
    void allocBuffer(long size);

    KScanControl *createToggleButton(QWidget *parent, const QString &text);
    KScanControl *createStringEntry(QWidget *parent, const QString &text);
    KScanControl *createNumberEntry(QWidget *parent, const QString &text);
    KScanControl *createSlider(QWidget *parent, const QString &text);
    KScanControl *createComboBox(QWidget *parent, const QString &text);
    KScanControl *createFileField(QWidget *parent, const QString &text);
    KScanControl *createGroupSeparator(QWidget *parent, const QString &text);
    KScanControl *createActionButton(QWidget *parent, const QString &text);

    KScanDevice *mScanDevice;
    int mIndex;
    const SANE_Option_Descriptor *mDesc;
    QByteArray mName;
    QString mText;

    bool mIsGroup;
    bool mIsReadable;

    KScanControl *mControl;

    QByteArray mBuffer;
    bool mBufferClean;
    bool mApplied;

    KGammaTable *mGammaTable;
};

#endif                          // KSCANOPTION_H
