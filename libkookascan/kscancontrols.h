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

#include "kookascan_export.h"

#include <qwidget.h>

class QHBoxLayout;
class QToolButton;
class QSpinBox;
class QComboBox;
class QCheckBox;
class QSlider;
class QLineEdit;
class QGroupBox;
class QPushButton;

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
     * A @c Text control maintains a string value, a @c Number control a
     * numeric value.  @c Group and @c Button controls do not have values.
     */
    enum ControlType {
        Text,
        Number,
        Group,
        Button
    };

    /**
     * Creates a control.
     *
     * @param parent The parent widget
     * @param text Text label for control
     */
    explicit KScanControl(QWidget *parent, const QString &text);

    /**
     * Destroys the control and any child widgets that it uses.
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
     * Ignored for controls of type @c Number or @c Group.
     *
     * @param text The new text value
     */
    virtual void setText(const QString &text);

    /**
     * Get the control's current text value.
     *
     * @return the text value, or @c QString() for a @c Number
     * or @c Group control.
     */
    virtual QString text() const;

    /**
     * Set the control's numeric value, for controls of type @c Number.
     * Ignored for controls of type @c Text or @c Group.
     *
     * @param val The new numeric value
     */
    virtual void setValue(int val);

    /**
     * Get the control's current numeric value.
     *
     * @return the numeric value, or @c 0 for a @c Text
     * or @c Group control.
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
     * The Return/Enter key has been pressed in a @c Text or
     * @c Number entry field, or a @c Button control has been pressed.
     * @c Group control types do not provide this signal (nor either of
     * the above).
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

class KOOKASCAN_EXPORT KScanSlider : public KScanControl
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
     * @param stdValue the value to which the 'reset' button resets the control
     */
    Q_DECL_DEPRECATED_X("Use KScanSlider(QWidget *,const QString &, bool) then setRange()")
    KScanSlider(QWidget *parent, const QString &text,
                double min, double max,
                bool haveStdButt = false, int stdValue = 0);

    /**
     * Creates the control.
     *
     * @param parent parent widget
     * @param text descriptive label for the control
     * @param haveStdButt if @c true, the 'reset' button will be present
     */
    KScanSlider(QWidget *parent, const QString &text, bool haveStdButt = false);

    KScanControl::ControlType type() const override		{ return (KScanControl::Number); }

    int value() const override;
    void setValue(int val) override;
    QSpinBox *spinBox() const					{ return (mSpinbox); }

    /**
     * Sets the allowed range and step for the slider.
     *
     * @param min minimum slider value
     * @param max maximum slider value
     * @param step value step, or -1 for a default setting
     * @param stdValue the value to which the 'reset' button resets the control
     *
     * @note The current control value is set to @p stdValue.
     */
    void setRange(int min, int max, int step = -1, int stdValue = 0);

protected slots:
    void slotSliderSpinboxChange(int val);
    void slotRevertValue();

private:
    void init(bool haveStdButt);

private:
    QSlider *mSlider;
    QSpinBox *mSpinbox;
    QToolButton *mStdButt;

    int mValue;
    int mStdValue;
};

/**
 * A free text entry field.
 *
 * @see QLineEdit
 */

class KOOKASCAN_EXPORT KScanStringEntry : public KScanControl
{
    Q_OBJECT

public:
    /**
     * Creates the control.
     *
     * @param parent parent widget
     * @param text descriptive label for the control
     */
    KScanStringEntry(QWidget *parent, const QString &text);

    KScanControl::ControlType type() const override
    {
        return (KScanControl::Text);
    }

    QString text() const override;
    void setText(const QString &text) override;

private:
    QLineEdit *mEntry;
};

/**
 * A numeric entry field.
 *
 * @see QLineEdit
 */

class KOOKASCAN_EXPORT KScanNumberEntry : public KScanControl
{
    Q_OBJECT

public:
    /**
     * Creates the control.
     *
     * @param parent parent widget
     * @param text descriptive label for the control
     */
    KScanNumberEntry(QWidget *parent, const QString &text);

    KScanControl::ControlType type() const override
    {
        return (KScanControl::Number);
    }

    int value() const override;
    void setValue(int i) override;

protected slots:
    void slotTextChanged(const QString &s);

private:
    QLineEdit *mEntry;
};

/**
 * A check box for an on/off option.
 *
 * @see QCheckBox
 */

class KOOKASCAN_EXPORT KScanCheckbox : public KScanControl
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

    KScanControl::ControlType type() const override
    {
        return (KScanControl::Number);
    }

    int value() const override;
    void setValue(int i) override;

    QString label() const override;

private:
    QCheckBox *mCheckbox;
};

/**
 * A combo box for a list of options.
 *
 * @see QComboBox
 */

class KOOKASCAN_EXPORT KScanCombo : public KScanControl
{
    Q_OBJECT

public:
    /**
     * Creates the control.
     *
     * @param parent parent widget
     * @param text descriptive label for the control
     */
    KScanCombo(QWidget *parent, const QString &text);

    KScanControl::ControlType type() const override
    {
        return (KScanControl::Text);
    }

    QString text() const override;
    void setText(const QString &text) override;
    void setValue(int i) override;

    /**
     * Populate the combo box with a list of values.
     *
     * @param list list of options to fill the combo box
     */
    void setList(const QList<QByteArray> &list);

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
    void setIcon(const QIcon &pix, const char *ent);

    /**
     * Indicate the the combo box should display scan mode icons.
     *
     * @param on @c true if icons should be used
     */
    void setUseModeIcons(bool on);

protected slots:
    void slotActivated(int i);

private:
    QComboBox *mCombo;
    bool mUseModeIcons;
};

/**
 * A standard URL requester for a file name.
 *
 * @see KUrlRequester
 */

class KOOKASCAN_EXPORT KScanFileRequester : public KScanControl
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

    KScanControl::ControlType type() const override
    {
        return (KScanControl::Text);
    }

    QString text() const override;
    void setText(const QString &text) override;

private:
    KUrlRequester *mEntry;
};

/**
 * A line separator between option groups.
 *
 * @see QGroupBox
 */

class KOOKASCAN_EXPORT KScanGroup : public KScanControl
{
    Q_OBJECT

public:
    /**
     * Creates the control.
     *
     * @param parent parent widget
     * @param text descriptive label for the control
     */
    KScanGroup(QWidget *parent, const QString &text);

    KScanControl::ControlType type() const override
    {
        return (KScanControl::Group);
    }

    QString label() const override;

private:
    QGroupBox *mGroup;
};

/**
 * A button to perform an action.
 *
 * @see QPushButton
 */

class KOOKASCAN_EXPORT KScanPushButton : public KScanControl
{
    Q_OBJECT

public:
    /**
     * Creates the control.
     *
     * @param parent parent widget
     * @param text descriptive label for the control
     */
    KScanPushButton(QWidget *parent, const QString &text);

    KScanControl::ControlType type() const override
    {
        return (KScanControl::Button);
    }

    QString label() const override;

private:
    QPushButton *mButton;
};

#endif                          // KSCANCONTROLS_H
