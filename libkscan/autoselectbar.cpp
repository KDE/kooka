/************************************************************************
 *									*
 *  This file is part of libkscan, a KDE scanning library.		*
 *									*
 *  Copyright (C) 2013 Jonathan Marten <jjm@keelhaul.me.uk>		*
 *									*
 *  This library is free software; you can redistribute it and/or	*
 *  modify it under the terms of the GNU Library General Public		*
 *  License as published by the Free Software Foundation and appearing	*
 *  in the file COPYING included in the packaging of this file;		*
 *  either version 2 of the License, or (at your option) any later	*
 *  version.								*
 *									*
 *  This program is distributed in the hope that it will be useful,	*
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of	*
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*
 *  GNU General Public License for more details.			*
 *									*
 *  You should have received a copy of the GNU General Public License	*
 *  along with this program;  see the file COPYING.  If not, write to	*
 *  the Free Software Foundation, Inc., 51 Franklin Street,		*
 *  Fifth Floor, Boston, MA 02110-1301, USA.				*
 *									*
 ************************************************************************/

#include "autoselectbar.h"
#include "autoselectbar.moc"

#include <qtoolbutton.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qslider.h>

#include <klocale.h>
#include <kdebug.h>
#include <kdialog.h>
#include <kicon.h>
#include <kcolordialog.h>

#include "autoselectdialog.h"
#include "kscancontrols.h"
#include "scansettings.h"


AutoSelectBar::AutoSelectBar(int initialValue, QWidget *parent)
    : QWidget(parent)
{
    kDebug();
    setObjectName("AutoSelectBar");

    QHBoxLayout *hbl = new QHBoxLayout;

    QLabel *l = new QLabel(i18nc("@title:window", "<qt><b>Auto Select</b>"));
    hbl->addWidget(l);

    hbl->addSpacing(2*KDialog::spacingHint());

    // Threshold setting label
    const KConfigSkeletonItem *item = ScanSettings::self()->previewAutoselThresholdItem();
    l = new QLabel(item->label());
    hbl->addWidget(l);

    // Threshold setting slider/spinbox
    int maxThresh = item->maxValue().toInt();
    mThresholdSlider = new KScanSlider(NULL, QString::null, 0, maxThresh);
    mThresholdSlider->setValue(initialValue);
    mThresholdSlider->setToolTip(item->toolTip());
    l->setBuddy(mThresholdSlider);

    connect(mThresholdSlider, SIGNAL(settingChanged(int)), SLOT(slotThresholdChanged(int)));
    hbl->addWidget(mThresholdSlider);
    hbl->setStretchFactor(mThresholdSlider, 1);

    mColourPatch = new KColorPatch(this);
    mColourPatch->setMinimumWidth(32);
    mColourPatch->setToolTip(i18nc("@info:tooltip", "This is the grayscale value of the selected threshold"));
    hbl->addWidget(mColourPatch);

    hbl->addSpacing(KDialog::spacingHint());

    // Refresh/recalculate button
    QToolButton *but = new QToolButton;
    but->setIcon(KIcon("view-refresh"));
    but->setToolTip(i18nc("@info:tooltip", "Perform the auto-detection again"));
    connect(but, SIGNAL(clicked()), SIGNAL(performSelection()));
    hbl->addWidget(but);

    // Advanced settings button
    but = new QToolButton;
    but->setIcon(KIcon("configure"));
    but->setToolTip(i18nc("@info:tooltip", "Advanced settings for auto-detection"));
    connect(but, SIGNAL(clicked()), SLOT(slotShowSettings()));
    hbl->addWidget(but);

    setLayout(hbl);

    slotThresholdChanged(mThresholdSlider->value());	// update the colour patch
}


AutoSelectBar::~AutoSelectBar()
{
}


void AutoSelectBar::slotThresholdChanged(int value)
{
    int colValue = value;
    if (mBgIsWhite) colValue = 255-colValue;
    mColourPatch->setColor(qRgb(colValue, colValue, colValue));
    emit thresholdChanged(value);
}


void AutoSelectBar::slotShowSettings()
{
    AutoSelectDialog *d = new AutoSelectDialog(this);
    d->setSettings(mMargin, mBgIsWhite, mDustsize);
    connect(d, SIGNAL(settingsChanged(int,bool,int)), SLOT(setAdvancedSettings(int,bool,int)));
    connect(d, SIGNAL(settingsChanged(int,bool,int)), SIGNAL(advancedSettingsChanged(int,bool,int)));
    d->show();
}


void AutoSelectBar::setThreshold(int thresh)
{
    mThresholdSlider->setValue(thresh);
}


void AutoSelectBar::setAdvancedSettings(int margin, bool bgIsWhite, int dustsize)
{							// just retained for dialogue
    mMargin = margin;
    mBgIsWhite = bgIsWhite;
    mDustsize = dustsize;
}
