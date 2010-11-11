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

#ifndef KSCANOPTION_H
#define KSCANOPTION_H

#include "libkscanexport.h"

#include <qobject.h>
#include <qbytearray.h>

extern "C" {
#include <sane/sane.h>
}


#define PARAM_ERROR	"parametererror"


class QLabel;

class KGammaTable;
class KScanControl;


/**
 *  This is KScanOption, a class which holds a single scanner option.
 *
 *  @short KScanOption
 *  @author Klaas Freitag
 *  @version 0.1alpha
 *
 **/

class KSCAN_EXPORT KScanOption : public QObject
{
    Q_OBJECT

public:

    enum WidgetType
    {
        Invalid,
        Bool,
        SingleValue,
        Range,
        GammaTable,
        StringList,
        String,
        Resolution,
        File,
        Group
    };

    /**
     * Creates a new option object for the named option. After that, the
     * option is valid and contains the current value retrieved from the
     * scanner.
     **/
    KScanOption(const QByteArray &name);

    /**
     * Copy constructor
     **/
    KScanOption(const KScanOption &so);

    /**
     * Destructor
     **/
    ~KScanOption();

    /**
     * Assignment operator
     **/
    // TODO: try to eliminate
    // used only by KScanOptSet::backupOption(const KScanOption &opt)
    const KScanOption &operator=(const KScanOption &so);

    /**
     * Check whether the option is valid, i.e. the option is known
     * by the scanner.
     **/
    bool isValid() const		{ return (mDesc!=NULL); }

    /**
     * Check whether the option is initialised, i.e. if the setting
     * for the option has been read from the scanner.
     **/
    bool isInitialised() const		{ return (!mBufferClean); }

    /**
     * Check whether the option is a group, this is a title only
     * and many of the operations are not available.
     **/
    bool isGroup() const		{ return (mIsGroup); }

    /**
     * Check whether the option is can be read from the scanner
     * (SANE_CAP_SOFT_SELECT with some special exceptions).
    **/
    bool isReadable() const		{ return (mIsReadable); }

    /**
     * Check whether the option is auto setable (SANE_CAP_AUTOMATIC),
     * i.e. the scanner can choose a setting for the option automatically.
    **/
    bool isAutoSettable() const;

    /**
     * Check whether the option is a common option (not SANE_CAP_ADVANCED).
     **/
    bool isCommonOption() const;

    /**
     * Check whether the option is currently active (SANE_OPTION_IS_ACTIVE).
     * This may change at runtime due to the settings of mode, resolutions etc.
     **/
    bool isActive() const;

    /**
     * Check whether the option is setable by software (SANE_OPTION_IS_SETTABLE).
     * Some scanner options can not be set by software.
     **/
    bool isSoftwareSettable() const;

    /**
     * Return the type of the option.
     **/
    KScanOption::WidgetType type() const;

    /**
     * Set the option value, depending on its type.
     **/
    bool set(int val);
    bool set(double val);
    bool set(const int *p_val, int size );
    bool set(const QByteArray &c_string);
    bool set(bool b)		{ return (set(b ? 1 : 0)); }
    bool set(KGammaTable *gt);

    /**
     * Retrieve the option value, depending on its type.
     **/
    bool get(int *val) const;
    bool get(KGammaTable *gt) const;
    QByteArray get() const;

    /**
     * Creates a widget for the scanner option, depending on its type.
     *
     * For boolean options, a checkbox is generated.
     *
     * For ranges, a KSaneSlider is generated, except for the special case of
     * a resolution option which generates a KScanCombo.
     *
     * For a String list such as mode etc., a KScanCombo is generated.
     *
     * For the option types string and gamma table, no widget is generated.
     *
     * The widget is maintained completely by the KScanOption object, so it
     * should not be deleted.
     *
     **/
    KScanControl *createWidget(QWidget *parent, const QString &descr = QString::null);
    QLabel *getLabel(QWidget *parent, bool alwaysBuddy = false) const;
    QLabel *getUnit(QWidget *parent) const;
   
    // Possible Values
    QList<QByteArray> getList() const;
    bool getRange(double *minp, double *maxp, double *quantp = NULL) const;

    QByteArray getName() const		{ return (mName); }
    void *getBuffer()			{ return (mBuffer.data()); }
    KScanControl *widget() const	{ return (mControl); }
    QString configLine() const		{ return (get()); }

    void redrawWidget();
    void reload();

protected slots:
    /**
     *  These slots are called if an option has a GUI element (not all have)
     *  and if the setting of the widget is changed.
     **/
    void slotWidgetChange(const QString &t);
    void slotWidgetChange(int i);
	
signals:
    /**
     *  User has changed a GUI setting
     */
    void guiChange(KScanOption *so);

private:
#ifdef APPLY_IN_SITU
    bool applyVal();
#endif
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
	
    int mIndex;
    const SANE_Option_Descriptor *mDesc;
    QByteArray mName;
    QString mText;

    bool mIsGroup;
    bool mIsReadable;

    KScanControl *mControl;

    QByteArray mBuffer;
    bool mBufferClean;

    /* For gamma-Tables remember gamma, brightness, contrast */
    int gamma, brightness, contrast;
};


#endif							// KSCANOPTION_H
