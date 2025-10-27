/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2025      Jonathan Marten <jjm@keelhaul.me.uk>	*
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

#include "multipageoptionsdialog.h"

#include <qcombobox.h>
#include <qformlayout.h>
#include <qboxlayout.h>
#include <qspinbox.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>

#include <klocalizedstring.h>

#include "papersizes.h"
#include "destination_logging.h"

//////////////////////////////////////////////////////////////////////////
//									//
//  MultipageOptionsDialog - A dialogue to replace QPageSetupDialog	//
//  containing only the options that are needed and allowing for the	//
//  possibility of more that may be needed later.			//
//									//
//////////////////////////////////////////////////////////////////////////

MultipageOptionsDialog::MultipageOptionsDialog(const QSizeF &pageSize, QWidget *pnt)
    : DialogBase(pnt)
{
    setObjectName("MultipageOptionsDialog");

    qCDebug(DESTINATION_LOG) << "page size" << pageSize;

    setButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    setWindowTitle(i18n("PDF Page Setup"));
    setModal(true);

    QWidget *w = new QWidget(this);
    setMainWidget(w);
    QFormLayout *fl = new QFormLayout(w);

    // Row 0: Page size combo box
    mPageSizeCombo = new QComboBox(w);
    mPageSizeCombo->setSizePolicy(QSizePolicy::MinimumExpanding, mPageSizeCombo->sizePolicy().verticalPolicy());

    int sizeIndex = -1;
    Qt::Orientation orient;

    const PaperSize *sizes = PaperSizes::self()->papers();
    for (int i = 0; sizes[i].name!=nullptr; ++i)
    {
        const double sw = sizes[i].width;
        const double sh = sizes[i].height;
        mPageSizeCombo->addItem(sizes[i].name, QSizeF(sw, sh));
        if (!pageSize.isValid()) continue;

        if (qFuzzyCompare(sw, pageSize.width()) && qFuzzyCompare(sh, pageSize.height()))
        {						// portrait orientation matches
            sizeIndex = i;
            orient = Qt::Vertical;
            qCDebug(DESTINATION_LOG) << "found portrait size" << sizes[i].name;
        }
        else if (qFuzzyCompare(sh, pageSize.width()) && qFuzzyCompare(sw, pageSize.height()))
        {						// landscape orientation matches
            sizeIndex = i;
            orient = Qt::Horizontal;
            qCDebug(DESTINATION_LOG) << "found landscape size" << sizes[i].name;
        }
    }
    mPageSizeCombo->addItem(i18n("Custom..."));

    fl->addRow(i18n("Page size:"), mPageSizeCombo);

    // Row 1: Page size display and custom entry
    QHBoxLayout *hb = new QHBoxLayout;
    hb->setContentsMargins(0, 0, 0, 0);

    mCustomWidthSpinbox = new QDoubleSpinBox(w);
    mCustomWidthSpinbox->setDecimals(1);
    mCustomWidthSpinbox->setRange(1.0, 2000.0);
    mCustomWidthSpinbox->setStepType(QAbstractSpinBox::AdaptiveDecimalStepType);
    mCustomWidthSpinbox->setSuffix(i18nc("unit suffix for millimetres", " mm"));
    mCustomWidthSpinbox->setEnabled(false);
    hb->addWidget(mCustomWidthSpinbox);

    QLabel *l = new QLabel(i18nc("centered 'x' separator for height/width", "\303\227"), w);
    hb->addWidget(l);

    mCustomHeightSpinbox = new QDoubleSpinBox(w);
    mCustomHeightSpinbox->setDecimals(1);
    mCustomHeightSpinbox->setRange(1.0, 2000.0);
    mCustomHeightSpinbox->setStepType(QAbstractSpinBox::AdaptiveDecimalStepType);
    mCustomHeightSpinbox->setSuffix(i18nc("unit suffix for millimetres", " mm"));
    mCustomHeightSpinbox->setEnabled(false);
    hb->addWidget(mCustomHeightSpinbox);

    hb->addStretch();
    fl->addRow("", hb);

    // Row 2: Spacing
    fl->setItem(fl->rowCount(), QFormLayout::SpanningRole, DialogBase::verticalSpacerItem());

    // Row 3: Portrait/Landscape radio buttons
    QButtonGroup *bg = new QButtonGroup(w);

    hb = new QHBoxLayout;
    hb->setContentsMargins(0, 0, 0, 0);

    mPortraitRadio = new QRadioButton(i18nc("@option:radio paper orientation", "Portrait"), w);
    bg->addButton(mPortraitRadio);
    hb->addWidget(mPortraitRadio);

    mLandscapeRadio = new QRadioButton(i18nc("@option:radio paper orientation", "Landscape"), w);
    bg->addButton(mLandscapeRadio);
    hb->addWidget(mLandscapeRadio);

    hb->addStretch();
    fl->addRow(i18n("Orientation:"), hb);

    // Set initial GUI values from the provided page size, if a match
    // was found above.
    if (sizeIndex!=-1)					// a preset size matched
    {
        mPageSizeCombo->setCurrentIndex(sizeIndex);
        mPortraitRadio->setChecked(orient==Qt::Vertical);
        mLandscapeRadio->setChecked(orient==Qt::Horizontal);
    }
    else if (pageSize.isValid())			// provided but not known
    {
        mPageSizeCombo->setCurrentIndex(mPageSizeCombo->count()-1);
        mCustomWidthSpinbox->setValue(pageSize.width());
        mCustomHeightSpinbox->setValue(pageSize.height());
    }
    else mPageSizeCombo->setCurrentIndex(0);		// default first in list

    slotValueChanged();
    slotSettingChanged();

    // Connect all signals now, after the settings have been adjusted
    connect(mPageSizeCombo, &QComboBox::currentIndexChanged, this, &MultipageOptionsDialog::slotSettingChanged);
    connect(bg, &QButtonGroup::buttonClicked, this, &MultipageOptionsDialog::slotSettingChanged);
    connect(mCustomWidthSpinbox, &QDoubleSpinBox::valueChanged, this, &MultipageOptionsDialog::slotValueChanged);
    connect(mCustomHeightSpinbox, &QDoubleSpinBox::valueChanged, this, &MultipageOptionsDialog::slotValueChanged);
}


void MultipageOptionsDialog::slotSettingChanged()
{
    const QSize pageSize = mPageSizeCombo->currentData().value<QSize>();
    const Qt::Orientation orient = (mLandscapeRadio->isChecked() ? Qt::Horizontal : Qt::Vertical);
    qCDebug(DESTINATION_LOG) << "selected size" << pageSize << "orient" << orient;

    QSignalBlocker block1(mCustomWidthSpinbox);		// don't want a signal from setValue()
    QSignalBlocker block2(mCustomHeightSpinbox);

    if (pageSize.isValid())				// preset paper size
    {
        if (orient==Qt::Vertical)			// portrait orientation
        {
            mCustomWidthSpinbox->setValue(pageSize.width());
            mCustomHeightSpinbox->setValue(pageSize.height());
        }
        else						// landscape orientation
        {
            mCustomWidthSpinbox->setValue(pageSize.height());
            mCustomHeightSpinbox->setValue(pageSize.width());
        }

        mCustomWidthSpinbox->setEnabled(false);
        mCustomHeightSpinbox->setEnabled(false);
    }
    else						// custom paper size
    {
        const double vw = mCustomWidthSpinbox->value();
        const double vh = mCustomHeightSpinbox->value();

        if (orient==Qt::Vertical)			// portrait orientation
        {
            mCustomWidthSpinbox->setValue(qMin(vw, vh));
            mCustomHeightSpinbox->setValue(qMax(vw, vh));
        }
        else						// landscape orientation
        {
            mCustomWidthSpinbox->setValue(qMax(vw, vh));
            mCustomHeightSpinbox->setValue(qMin(vw, vh));
        }

        mCustomWidthSpinbox->setEnabled(true);
        mCustomHeightSpinbox->setEnabled(true);
    }
}


void MultipageOptionsDialog::slotValueChanged()
{
    if (!mCustomWidthSpinbox->isEnabled())		// not setting a custom size
    {							// and not needing initialsation
        if (mPortraitRadio->isChecked() || mLandscapeRadio->isChecked()) return;
    }

    const double vw = mCustomWidthSpinbox->value();
    const double vh = mCustomHeightSpinbox->value();
    if (vw>vh) mLandscapeRadio->setChecked(true);	// set to landscape orientation
    else mPortraitRadio->setChecked(true);		// set to portrait orientation
}


QSizeF MultipageOptionsDialog::pageSize() const
{
    const QSizeF pageSize = mPageSizeCombo->currentData().value<QSizeF>();
    const Qt::Orientation orient = (mLandscapeRadio->isChecked() ? Qt::Horizontal : Qt::Vertical);
    qCDebug(DESTINATION_LOG) << "selected size" << pageSize << "orient" << orient;

    if (pageSize.isValid())				// preset paper size
    {
        return (orient==Qt::Vertical ? pageSize : pageSize.transposed());
    }
    else						// custom paper size,
    {							// directly from spin boxes
        return (QSizeF(mCustomWidthSpinbox->value(), mCustomHeightSpinbox->value()));
    }
}
