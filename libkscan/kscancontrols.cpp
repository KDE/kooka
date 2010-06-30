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

#include "kscancontrols.h"
#include "kscancontrols.moc"

#include <qlayout.h>
#include <qtoolbutton.h>
#include <qspinbox.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qslider.h>
#include <qlineedit.h>

#include <klocale.h>
#include <kdebug.h>
#include <kurlrequester.h>
#include <kimageio.h>


//  KScanControl - base class
//  -------------------------

KScanControl::KScanControl(QWidget *parent, const QString &text)
    : QWidget(parent)
{
    mLayout = new QHBoxLayout(this);
    mLayout->setMargin(0);

    mText = text;
    if (mText.isEmpty()) mText = i18n("(Unknown)");
}


KScanControl::~KScanControl()				{}

QString KScanControl::text() const			{ return (QString::null); }
void KScanControl::setText(const QString &text)		{}

int KScanControl::value() const				{ return (0); }
void KScanControl::setValue(int val)			{}

QString KScanControl::label() const			{ return (mText+":"); }


//  KScanSlider - slider, spin box and optional reset button
//  --------------------------------------------------------

KScanSlider::KScanSlider(QWidget *parent, const QString &text,
                         double min, double max,
                         bool haveStdButt, int stdValue)
    : KScanControl(parent,text),
      mStdValue(stdValue),
      mStdButt(NULL)
{
    mSlider = new QSlider(Qt::Horizontal, this);	// slider
    mSlider->setRange(((int) min), ((int) max));
    mSlider->setTickPosition(QSlider::TicksBelow );
    mSlider->setTickInterval(qMax(((int)((max-min)/10)), 1));
    mSlider->setSingleStep(qMax(((int)((max-min)/20)), 1));
    mSlider->setPageStep(qMax(((int)((max-min)/10)), 1));
    mSlider->setMinimumWidth(140);
    mSlider->setValue(((int) min)-1);			// set to initial value
    mLayout->addWidget(mSlider, 1);

    mSpinbox = new QSpinBox(this);			// spin box
    mSpinbox->setRange((int) min, (int) max);
    mSpinbox->setSingleStep(1);
    mLayout->addWidget(mSpinbox);

    if (haveStdButt)
    {
        mStdButt = new QToolButton(this);		// reset button
        mStdButt->setIcon(KIcon("edit-undo"));
        mStdButt->setToolTip(i18n("Reset this setting to its standard value, %1", stdValue));
        mLayout->addWidget(mStdButt);
    }

    connect(mSlider, SIGNAL(valueChanged(int)), SLOT(slotValueChange(int)));
    connect(mSpinbox, SIGNAL(valueChanged(int)), SLOT(slotValueChange(int)));
    if (mStdButt!=NULL) connect(mStdButt, SIGNAL(clicked()), SLOT(slotRevertValue()));

    setFocusProxy(mSlider);
}


void KScanSlider::setValue(int val)
{
    if (val==mSlider->value()) return;			// avoid recursive signals
    slotValueChange(val);
}


int KScanSlider::value() const
{
    return (mSlider->value());
}


void KScanSlider::slotValueChange(int val)
{
    int spin = mSpinbox->value();
    if (spin!=val) mSpinbox->setValue(val);		// track in spin box
    int slid = mSlider->value();
    if (slid!=val) mSlider->setValue(val);		// track in slider

    emit settingChanged(val);
}


void KScanSlider::slotRevertValue()
{							// only connected if button exists
    slotValueChange(mStdValue);
}


//  KScanEntry - free text entry field
//  ----------------------------------

KScanEntry::KScanEntry(QWidget *parent, const QString &text)
    : KScanControl(parent, text)
{
    mEntry = new QLineEdit(this);
    mLayout->addWidget(mEntry);

    connect(mEntry, SIGNAL(textChanged(const QString &)), SIGNAL(settingChanged(const QString &)));
    connect(mEntry, SIGNAL(returnPressed()), SIGNAL(returnPressed()));

    setFocusProxy(mEntry);
}


QString KScanEntry::text() const
{
    return (mEntry->text());
}


void KScanEntry::setText(const QString &text)
{
    if (text==mEntry->text()) return;			// avoid recursive signals
    mEntry->setText(text);
}


//  KScanCheckbox - on/off option
//  -----------------------------

KScanCheckbox::KScanCheckbox(QWidget *parent, const QString &text)
    : KScanControl(parent, text)
{
    mCheckbox = new QCheckBox(text, this);
    mLayout->addWidget(mCheckbox);

    connect(mCheckbox, SIGNAL(stateChanged(int)), SIGNAL(settingChanged(int)));

    setFocusProxy(mCheckbox);
}


int KScanCheckbox::value() const
{
    return ((int) mCheckbox->isChecked());
}


void KScanCheckbox::setValue(int i)
{
    mCheckbox->setChecked((bool) i);
}


QString KScanCheckbox::label() const
{
    return (QString::null);
}


//  KScanCombo - combo box with list of options
//  -------------------------------------------

KScanCombo::KScanCombo(QWidget *parent, const QString &text,
                       const QList<QByteArray> &list)
    : KScanControl(parent, text)
{
    init();

    for (QList<QByteArray>::const_iterator it = list.constBegin();
         it!=list.constEnd(); ++it)
    {
	mCombo->addItem(i18n(*it));
    }
}


KScanCombo::KScanCombo(QWidget *parent, const QString &text,
                       const QStringList &list)
    : KScanControl(parent, text)
{
    init();

    for (QStringList::const_iterator it = list.constBegin();
         it!=list.constEnd(); ++it)
    {
        mCombo->addItem(*it);
    }
}


void KScanCombo::init()
{
    mCombo = new QComboBox(this);
    mLayout->addWidget(mCombo);

    connect(mCombo, SIGNAL(activated( const QString &)), SIGNAL(settingChanged(const QString &)));
    connect(mCombo, SIGNAL(activated(int)), SIGNAL(settingChanged(int)));

    setFocusProxy(mCombo);
}


void KScanCombo::setText(const QString &text)
{
    int i = mCombo->findText(text);			// find item with that text
    if (i==-1) return;					// ignore if not present

    if (i==mCombo->currentIndex()) return;		// avoid recursive signals
    mCombo->setCurrentIndex(i);
}


void KScanCombo::setIcon(const QIcon &icon, const char *ent)
{
    int i = mCombo->findText(ent);
    if (i!=-1) mCombo->setItemIcon(i, icon);
    i = mCombo->findText(i18n(ent));
    if (i!=-1) mCombo->setItemIcon(i, icon);
}


QString KScanCombo::text() const
{
    return (mCombo->currentText());
}


void KScanCombo::setValue(int i)
{
    mCombo->setCurrentIndex(i);
}


QString KScanCombo::textAt(int i) const
{
    return (mCombo->itemText(i));
}


int KScanCombo::count() const
{
    return (mCombo->count());
}


//  KScanFileRequester - standard URL requester 
//  -------------------------------------------

KScanFileRequester::KScanFileRequester(QWidget *parent, const QString &text)
    : KScanControl(parent, text)
{
    mEntry = new KUrlRequester(this);
    mLayout->addWidget(mEntry);

    QString fileSelector = "*.pnm *.PNM *.pbm *.PBM *.pgm *.PGM *.ppm *.PPM|PNM Image Files (*.pnm,*.pbm,*.pgm,*.ppm)\n";
    fileSelector += KImageIO::pattern()+"\n";
    fileSelector += i18n("*|All Files");
    mEntry->setFilter(fileSelector);

    connect(mEntry, SIGNAL(textChanged(const QString& )), SIGNAL(settingChanged(const QString&)));
    connect(mEntry, SIGNAL(returnPressed()), SIGNAL(returnPressed()));

    setFocusProxy(mEntry);
}


QString KScanFileRequester::text() const
{
    return (mEntry->url().url());
}


void KScanFileRequester::setText(const QString &text)
{
    if (text==mEntry->url().url()) return;		// avoid recursive signals
    mEntry->setUrl(text);
}
