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

#include "kscancontrols.h"

#include <qgroupbox.h>
#include <qlayout.h>
#include <qtoolbutton.h>
#include <qspinbox.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qslider.h>
#include <qlineedit.h>
#include <qicon.h>
#include <qimagereader.h>
#include <qmimetype.h>
#include <qmimedatabase.h>

#include <klocalizedstring.h>
#include <kurlrequester.h>

#include "imagefilter.h"
#include "scanicons.h"
#include "libkookascan_logging.h"


//  KScanControl - base class
//  -------------------------

KScanControl::KScanControl(QWidget *parent, const QString &text)
    : QWidget(parent)
{
    mLayout = new QHBoxLayout(this);
    mLayout->setContentsMargins(0, 0, 0, 0);

    mText = text;
    if (mText.isEmpty()) {
        mText = i18n("(Unknown)");
    }
}

KScanControl::~KScanControl()
{
}

QString KScanControl::text() const
{
    return (QString());
}

void KScanControl::setText(const QString &text)
{
}

int KScanControl::value() const
{
    return (0);
}

void KScanControl::setValue(int val)
{
}

QString KScanControl::label() const
{
    return (mText + ":");
}

//  KScanSlider - slider, spin box and optional reset button
//  --------------------------------------------------------

KScanSlider::KScanSlider(QWidget *parent, const QString &text,
                         double min, double max,
                         bool haveStdButt, int stdValue)
    : KScanControl(parent, text)
{
    init(haveStdButt);
    setRange(min, max, -1, stdValue);
}

KScanSlider::KScanSlider(QWidget *parent, const QString &text, bool haveStdButt)
    : KScanControl(parent, text)
{
    init(haveStdButt);
}

void KScanSlider::init(bool haveStdButt)
{
    mStdButt = nullptr;

    mSlider = new QSlider(Qt::Horizontal, this);	// slider
    mSlider->setTickPosition(QSlider::TicksBelow);
    mSlider->setMinimumWidth(140);
    mLayout->addWidget(mSlider, 1);

    mSpinbox = new QSpinBox(this);			// spin box
    mSpinbox->setMinimumWidth(60);
    mLayout->addWidget(mSpinbox);

    if (haveStdButt)
    {
        mStdButt = new QToolButton(this);		// reset button
        mStdButt->setIcon(QIcon::fromTheme("edit-undo"));
        mLayout->addWidget(mStdButt);
    }

    connect(mSlider, &QSlider::valueChanged, this, &KScanSlider::slotSliderSpinboxChange);
    connect(mSpinbox, QOverload<int>::of(&QSpinBox::valueChanged), this, &KScanSlider::slotSliderSpinboxChange);
    if (mStdButt!=nullptr) connect(mStdButt, &QPushButton::clicked, this, &KScanSlider::slotRevertValue);

    setFocusProxy(mSlider);
    setFocusPolicy(Qt::StrongFocus);
}

void KScanSlider::setRange(int min, int max, int step, int stdValue)
{
    const double span = max-min;

    mSlider->setRange(min, max);
    mSlider->setTickInterval(qMax(qRound(span/10), 1));
    mSpinbox->setRange(min, max);

    if (step==-1)					// default step value
    {
        mSlider->setSingleStep(qMax(qRound(span/20), 1));
        mSlider->setPageStep(qMax(qRound(span/5), 1));
        mSpinbox->setSingleStep(1);
    }
    else						// specified step value
    {
        mSlider->setSingleStep(step);
        mSlider->setPageStep(step*4);
        mSpinbox->setSingleStep(step);
    }

    mStdValue = qBound(min, stdValue, max);		// limit default value to range
    mValue = mStdValue;					// set current value to that
    mSlider->setValue(mValue);
    mSpinbox->setValue(mValue);

    if (mStdButt!=nullptr) mStdButt->setToolTip(i18n("Reset this setting to its standard value, %1", QString::number(mStdValue)));
}

void KScanSlider::setValue(int val)
{
    if (val == mValue) {
        return;    // avoid recursive signals
    }
    mValue = val;

    int spin = mSpinbox->value();
    if (spin != val) {
        mSpinbox->blockSignals(true);
        mSpinbox->setValue(val);            // track in spin box
        mSpinbox->blockSignals(false);
    }

    int slid = mSlider->value();
    if (slid != val) {
        mSlider->blockSignals(true);
        mSlider->setValue(val);             // track in slider
        mSlider->blockSignals(false);
    }
}

int KScanSlider::value() const
{
    return (mValue);
}

void KScanSlider::slotSliderSpinboxChange(int val)
{
    setValue(val);
    emit settingChanged(val);
}

void KScanSlider::slotRevertValue()
{
    // only connected if button exists
    slotSliderSpinboxChange(mStdValue);
}

//  KScanStringEntry - free text entry field
//  ----------------------------------------

KScanStringEntry::KScanStringEntry(QWidget *parent, const QString &text)
    : KScanControl(parent, text)
{
    mEntry = new QLineEdit(this);
    mLayout->addWidget(mEntry);

    connect(mEntry, &QLineEdit::textChanged, this, QOverload<const QString &>::of(&KScanStringEntry::settingChanged));
    connect(mEntry, &QLineEdit::returnPressed, this, &KScanStringEntry::returnPressed);

    setFocusProxy(mEntry);
    setFocusPolicy(Qt::StrongFocus);
}

QString KScanStringEntry::text() const
{
    return (mEntry->text());
}

void KScanStringEntry::setText(const QString &text)
{
    if (text == mEntry->text()) {
        return;    // avoid recursive signals
    }
    mEntry->setText(text);
}

//  KScanNumberEntry - number entry field
//  -------------------------------------

KScanNumberEntry::KScanNumberEntry(QWidget *parent, const QString &text)
    : KScanControl(parent, text)
{
    mEntry = new QLineEdit(this);
    mEntry->setValidator(new QIntValidator);
    mLayout->addWidget(mEntry);

    connect(mEntry, &QLineEdit::textChanged, this, QOverload<const QString &>::of(&KScanNumberEntry::settingChanged));
    connect(mEntry, &QLineEdit::textChanged, this, &KScanNumberEntry::slotTextChanged);
    connect(mEntry, &QLineEdit::returnPressed, this, &KScanNumberEntry::returnPressed);

    setFocusProxy(mEntry);
    setFocusPolicy(Qt::StrongFocus);
}

int KScanNumberEntry::value() const
{
    return (mEntry->text().toInt());
}

void KScanNumberEntry::setValue(int i)
{
    mEntry->setText(QString::number(i));
}

void KScanNumberEntry::slotTextChanged(const QString &s)
{
    emit settingChanged(s.toInt());
}

//  KScanCheckbox - on/off option
//  -----------------------------

KScanCheckbox::KScanCheckbox(QWidget *parent, const QString &text)
    : KScanControl(parent, text)
{
    mCheckbox = new QCheckBox(text, this);
    mLayout->addWidget(mCheckbox);

    connect(mCheckbox, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state)
    {
        emit settingChanged(static_cast<int>(state));
    });

    setFocusProxy(mCheckbox);
    setFocusPolicy(Qt::StrongFocus);
}

int KScanCheckbox::value() const
{
    return (int(mCheckbox->isChecked()));
}

void KScanCheckbox::setValue(int i)
{
    mCheckbox->setChecked(bool(i));
}

QString KScanCheckbox::label() const
{
    return (QString());
}

//  KScanCombo - combo box with list of options
//  -------------------------------------------
//
//  This control (and currently only this control) is special, because the
//  item text is set to the translated SANE value.  But these values need
//  to be reported back unchanged, so the untranslated form is stored
//  in the itemData and the translated form is seen by the user as itemText.
//  Any access needs to only set or report the itemData, never the itemText,
//  which is why the activated(QString) signal cannot be used directly.

KScanCombo::KScanCombo(QWidget *parent, const QString &text)
    : KScanControl(parent, text)
{
    mCombo = new QComboBox(this);
    mLayout->addWidget(mCombo);

    connect(mCombo, QOverload<int>::of(&QComboBox::activated), this, &KScanCombo::slotActivated);

    setFocusProxy(mCombo);
    setFocusPolicy(Qt::StrongFocus);

    mUseModeIcons = false;
}

void KScanCombo::setList(const QList<QByteArray> &list)
{
    // An optimisation, which may turn out to be not a valid one:
    // only update the combo box if the number of items has changed.
    // If using scan mode icons, this assumption is not valid because
    // the list icons need to be finalised when the options are
    // reloaded after they have all been created.
    if (!mUseModeIcons && list.count()==mCombo->count()) return;
    //qCDebug(LIBKOOKASCAN_LOG) << "count" << mCombo->count() << "->" << list.count() << "=" << list;

    const QString cur = text();				// get current setting

    const bool bs = mCombo->blockSignals(true);
    mCombo->clear();

    for (const QByteArray &item : std::as_const(list))
    {
        // See the KI18N Programmer's Guide, "Connecting to Catalogs in Library Code"
        mCombo->addItem(ki18n(item.constData()).toString("sane-backends"), item);
        if (mUseModeIcons)				// set the scan mode icon
        {
            const int idx = mCombo->count()-1;
            mCombo->setItemIcon(idx, ScanIcons::self()->icon(item));
        }
    }

    mCombo->blockSignals(bs);
    if (!cur.isEmpty()) setText(cur);                   // try to restore old setting
}

void KScanCombo::setText(const QString &text)
{
    // This findData() must be done with a string of the same type that
    // was originally set in setList(), i.e. a QByteArray.  Otherwise the
    // exact QVariant matching carried out by default does not seem to work
    // (no matching item is found) in Qt6, although it did in Qt5.  Using
    // Qt::MatchFixedString would also work.
    int i = mCombo->findData(text.toLatin1());		// find item with that text
    if (i == -1) return;				// ignore if not present

    if (i == mCombo->currentIndex()) return;		// avoid recursive signals
    mCombo->setCurrentIndex(i);
}

void KScanCombo::setIcon(const QIcon &icon, const char *ent)
{
    int i = mCombo->findData(ent);			// find item with that text
    if (i != -1) mCombo->setItemIcon(i, icon);
}

QString KScanCombo::text() const
{
    return (textAt(mCombo->currentIndex()));
}

void KScanCombo::setValue(int i)
{
    mCombo->setCurrentIndex(i);
}

QString KScanCombo::textAt(int i) const
{
    return (i == -1 ? QString() : mCombo->itemData(i).toString());
}

int KScanCombo::count() const
{
    return (mCombo->count());
}

void KScanCombo::slotActivated(int i)
{
    emit settingChanged(i);
    emit settingChanged(textAt(i));
}

void KScanCombo::setUseModeIcons(bool on)
{
    mUseModeIcons = on;
}


//  KScanFileRequester - standard URL requester
//  -------------------------------------------

KScanFileRequester::KScanFileRequester(QWidget *parent, const QString &text)
    : KScanControl(parent, text)
{
    mEntry = new KUrlRequester(this);
    mLayout->addWidget(mEntry);

    QStringList filter(i18n("PNM Image Files (*.pnm *.pbm *.pgm *.ppm)"));
    filter += ImageFilter::qtFilterList(ImageFilter::Reading);
    mEntry->setNameFilters(filter);

    connect(mEntry, QOverload<const QString &>::of(&KUrlRequester::textChanged), this, QOverload<const QString &>::of(&KScanFileRequester::settingChanged));
    connect(mEntry, QOverload<const QString &>::of(&KUrlRequester::returnPressed), this, &KScanStringEntry::returnPressed);

    setFocusProxy(mEntry);
    setFocusPolicy(Qt::StrongFocus);
}

QString KScanFileRequester::text() const
{
    return (mEntry->url().url());
}

void KScanFileRequester::setText(const QString &text)
{
    if (text == mEntry->url().url()) {
        return;    // avoid recursive signals
    }
    mEntry->setUrl(QUrl::fromLocalFile(text));
}

//  KScanGroup - group separator
//  ----------------------------

KScanGroup::KScanGroup(QWidget *parent, const QString &text)
    : KScanControl(parent, text)
{
    mGroup = new QGroupBox(text, this);
    mGroup->setFlat(true);
    mLayout->addWidget(mGroup);
}

QString KScanGroup::label() const
{
    return (QString());
}

//  KScanPushButton - action button
//  -------------------------------

KScanPushButton::KScanPushButton(QWidget *parent, const QString &text)
    : KScanControl(parent, text)
{
    mButton = new QPushButton(text, this);
    mLayout->addWidget(mButton);

    connect(mButton, &QPushButton::clicked, this, &KScanPushButton::returnPressed);
}

QString KScanPushButton::label() const
{
    return (QString());
}
