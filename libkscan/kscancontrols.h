/* This file is part of the KDE Project
   Copyright (C) 2000 Klaas Freitag <freitag@suse.de>
   Copyright (C) 2010 Jonathan Marten <jjm@keelhaul.me.uk>

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

#ifndef KSCANCONTROLS_H
#define KSCANCONTROLS_H

#include "libkscanexport.h"

#include <qwidget.h>


class QHBoxLayout;
class QToolButton;
class QSpinBox;
class QComboBox;
class QCheckBox;
class QSlider;
class QLineEdit;
class QStringList;

class KUrlRequester;


/**
 * An abstract base class representing a GUI for a single scan
 * parameter control.  All scanner controls are one of the subclasses
 * of this, their precise appearance and operation depending on the
 * SANE type of the parameter.
 */

class KScanControl : public QWidget
{
    Q_OBJECT

public:
    /**
     * The internal type of the control (regardless of GUI appearance).
     *
     * A @c Text control maintains a string value, a @Number control a
     * numeric value.
     */
    enum ControlType
    {
        Text,
        Number
    };

    /**
     * Creates a control.
     *
     * @param parent The parent widget
     * @param text Text label for control
     */
    KScanControl(QWidget *parent, const QString &text);

    /**
     * Destructs the control and any child widgets that it uses.
     */
    virtual ~KScanControl();

    /**
     * Control type.
     *
     * @return the control type
     * @see ControlType
     */
    virtual KScanControl::ControlType type() const = 0;

    /**
     * Set the control's text value, for controls of type @c Text.
     * Ignored for controls of type @c Number.
     *
     * @param text The new text value
     */
    virtual void setText(const QString &text);

    /**
     * Get the control's current text value.
     *
     * @return the text value, or @c QString::null for a @c Number control.
     */
    virtual QString text() const;

    /**
     * Set the control's numeric value, for controls of type @c Number.
     * Ignored for controls of type @c Text.
     *
     * @param val The new numeric value
     */
    virtual void setValue(int val);

    /**
     * Get the control's current numeric value.
     *
     * @return the numeric value, or @c 0 for a @c Text control.
     */
    virtual int value() const;

    /**
     * Get a descriptive label text for the control.
     *
     * For all controls except check boxes, this is the original @p text
     * parameter to the constructor with a ":" appended.  For a check box,
     * this is null (a check box has its own label in the usual place,
     * on the right).
     *
     * @return the label string
     */
    virtual QString label() const;

protected:
    QHBoxLayout *mLayout;
    QString mText;

signals:
    /**
     * The setting of a @c Text control has changed.
     *
     * @param text The new text setting
     */
    void settingChanged(const QString &text);

    /**
     * The setting of a @c Number control has changed.
     *
     * @param val The new numeric setting
     */
    void settingChanged(int val);

    /**
     * The Return/Enter key has been pressed in an entry field.
     *
     * Not all control types provide this signal.
     */
    void returnPressed();
};


/**
 * A slider combined with a spin box, providing the possibility of either
 * selecting a value with the slider or entering a precise value in the
 * spin box.  There can also optionally be a 'reset' button which returns
 * the setting to a default value.
 *
 * @see QSlider, QSpinBox
 */

class KSCAN_EXPORT KScanSlider : public KScanControl
{
    Q_OBJECT

public:
    /**
     * Creates the control.
     *
     * @param parent parent widget
     * @param text descriptive label for the control
     * @param min minimum slider value
     * @param max maximum slider value
     * @param haveStdButt if @c true, the 'reset' button will be present
     * @param stdValue the value to which the 'reset' button resets the setting
     */
    KScanSlider(QWidget *parent, const QString &text,
		double min, double max,
                bool haveStdButt = false, int stdValue = 0);

    KScanControl::ControlType type() const { return (KScanControl::Number); }

    int value() const;
    void setValue(int val);

protected slots:
    void slotValueChange(int val);
    void slotRevertValue();

private:
    QSlider *mSlider;
    QSpinBox *mSpinbox;
    int mStdValue;
    QToolButton *mStdButt;
};


/**
 * A free text entry field.
 *
 * @see QLineEdit
 */

class KSCAN_EXPORT KScanEntry : public KScanControl
{
    Q_OBJECT

public:
    /**
     * Creates the control.
     *
     * @param parent parent widget
     * @param text descriptive label for the control
     */
    KScanEntry(QWidget *parent, const QString &text);

    KScanControl::ControlType type() const { return (KScanControl::Text); }

    QString text() const;
    void setText(const QString& text);

private:
    QLineEdit *mEntry;
};


/**
 * A check box for an on/off option.
 *
 * @see QCheckBox
 */

class KSCAN_EXPORT KScanCheckbox : public KScanControl
{
    Q_OBJECT

public:
    /**
     * Creates the control.
     *
     * @param parent parent widget
     * @param text descriptive label for the control
     */
    KScanCheckbox(QWidget *parent, const QString &text);

    KScanControl::ControlType type() const { return (KScanControl::Number); }

    int value() const;
    void setValue(int i);

    QString label() const;

private:
    QCheckBox *mCheckbox;
};


/**
 * A combo box for a list of options.
 *
 * @see QComboBox
 */

class KSCAN_EXPORT KScanCombo : public KScanControl
{
    Q_OBJECT

public:
    /**
     * Creates the control.
     *
     * @param parent parent widget
     * @param text descriptive label for the control
     * *param list list of options to fill the combo box
     */
    KScanCombo(QWidget *parent, const QString &text, const QList<QByteArray> &list);
    KScanCombo(QWidget *parent, const QString &text, const QStringList &list);

    KScanControl::ControlType type() const { return (KScanControl::Text); }

    QString text() const;
    void setText(const QString &text);
    void setValue(int i);

    /**
     * Get the text at a specified index in the combo box.
     *
     * @param i the requested index
     * @return the text at that index
     */
    QString textAt(int i) const;

    /**
     * Count how many combo box entries there are.
     *
     * @return the number of entries
     */
    int count() const;

    /**
     * Set an icon for an item in the combo box.
     *
     * @param pix the pixmap to set
     * @param ent the entry for which the pixmap should be set
     */
    void setIcon(const QIcon &pix, const QString &ent);

private:
    void init();
    QComboBox *mCombo;
};


/**
 * A standard URL requester for a file name.
 *
 * @see KUrlRequester
 */

class KSCAN_EXPORT KScanFileRequester : public KScanControl
{
    Q_OBJECT

public:
    /**
     * Creates the control.
     *
     * @param parent parent widget
     * @param text descriptive label for the control
     */
    KScanFileRequester(QWidget *parent, const QString &text);

    KScanControl::ControlType type() const { return (KScanControl::Text); }

    QString text() const;
    void setText(const QString &text);

private:
    KUrlRequester *mEntry;
};


#endif							// KSCANCONTROLS_H
