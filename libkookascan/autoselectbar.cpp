/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2013-2016 Jonathan Marten <jjm@keelhaul.me.uk>	*
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

#include "autoselectbar.h"

#include <qtoolbutton.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qslider.h>
#include <qdebug.h>
#include <qicon.h>
#include <qframe.h>
#include <qpalette.h>

#include <klocalizedstring.h>

#include "autoselectdialog.h"
#include "kscancontrols.h"
#include "scansettings.h"
#include "dialogbase.h"


AutoSelectBar::AutoSelectBar(int initialValue, QWidget *parent)
    : QWidget(parent)
{
    //qDebug();
    setObjectName("AutoSelectBar");

    QHBoxLayout *hbl = new QHBoxLayout;

    QLabel *l = new QLabel(xi18nc("@info", "<emphasis strong=\"1\">Auto Select</emphasis>"));
    hbl->addWidget(l);

    hbl->addSpacing(2*DialogBase::horizontalSpacing());

    // Threshold setting label
    const KConfigSkeletonItem *item = ScanSettings::self()->previewAutoselThresholdItem();
    l = new QLabel(item->label());
    hbl->addWidget(l);

    // Threshold setting slider/spinbox
    int maxThresh = item->maxValue().toInt();
    mThresholdSlider = new KScanSlider(nullptr, QString(), 0, maxThresh);
    mThresholdSlider->setValue(initialValue);
    mThresholdSlider->setToolTip(item->toolTip());
    l->setBuddy(mThresholdSlider);

    connect(mThresholdSlider, SIGNAL(settingChanged(int)), SLOT(slotThresholdChanged(int)));
    hbl->addWidget(mThresholdSlider);
    hbl->setStretchFactor(mThresholdSlider, 1);

    mColourPatch = new QFrame(this);
    // from kdelibs4support/src/kdeui/kcolordialog.cpp
    mColourPatch->setFrameStyle(QFrame::StyledPanel|QFrame::Sunken);
    mColourPatch->setMinimumWidth(32);
    mColourPatch->setAutoFillBackground(true);
    mColourPatch->setToolTip(i18nc("@info:tooltip", "This is the grayscale value of the selected threshold"));
    hbl->addWidget(mColourPatch);

    hbl->addSpacing(DialogBase::horizontalSpacing());

    // Refresh/recalculate button
    QToolButton *but = new QToolButton;
    but->setIcon(QIcon::fromTheme("view-refresh"));
    but->setToolTip(i18nc("@info:tooltip", "Perform the auto-detection again"));
    connect(but, SIGNAL(clicked()), SIGNAL(performSelection()));
    hbl->addWidget(but);

    // Advanced settings button
    but = new QToolButton;
    but->setIcon(QIcon::fromTheme("configure"));
    but->setToolTip(i18nc("@info:tooltip", "Advanced settings for auto-detection"));
    connect(but, SIGNAL(clicked()), SLOT(slotShowSettings()));
    hbl->addWidget(but);

    setLayout(hbl);

    slotThresholdChanged(mThresholdSlider->value());    // update the colour patch
}

AutoSelectBar::~AutoSelectBar()
{
}

void AutoSelectBar::slotThresholdChanged(int value)
{
    int colValue = value;
    if (mBgIsWhite) {
        colValue = 255 - colValue;
    }

    QPalette pal = mColourPatch->palette();
    pal.setColor(QPalette::Normal, QPalette::Window, qRgb(colValue, colValue, colValue));
    mColourPatch->setPalette(pal);
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
{
    // just retained for dialogue
    mMargin = margin;
    mBgIsWhite = bgIsWhite;
    mDustsize = dustsize;
}
