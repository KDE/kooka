/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2003-2016 Klaas Freitag <freitag@suse.de>		*
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

#include "imgprintdialog.h"

#include <qlayout.h>
#include <qlabel.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qradiobutton.h>
#include <qgroupbox.h>
#include <qspinbox.h>
#include <qlineedit.h>
#include <qtimer.h>
#include <qgridlayout.h>

#include <QSignalBlocker>

#include <klocalizedstring.h>

#include "scanimage.h"

#include "kookaprint.h"
#include "kscandevice.h"
#include "dialogbase.h"
#include "kooka_logging.h"


ImgPrintDialog::ImgPrintDialog(ScanImage::Ptr img, KookaPrint *prt, QWidget *pnt)
    : QWidget(pnt)
{
    m_image = img;					// record the image
    qCDebug(KOOKA_LOG) << "image size" << img->size();
    mPrinter = prt;					// record the printer

    setWindowTitle(i18nc("@title:tab", "Image"));	// used as tab title

    // Timer for delayed/combined updates of print parameters
    mUpdateTimer = new QTimer(this);
    mUpdateTimer->setSingleShot(true);
    // The default button auto-repeat delay is set to 300
    // in qtbase/src/widgets/widgets/qabstractbutton.cpp
    mUpdateTimer->setInterval(320);			// longer than button auto-repeat
    connect(mUpdateTimer, &QTimer::timeout, this, &ImgPrintDialog::updatePrintParameters);

    QVBoxLayout *layout = new QVBoxLayout(this);

    // Each control (or group of controls) created here that affect the printer
    // page or layout needs to be connected to mUpdateTimer->start().  This will
    // call updatePrintParameters() on a delay to combine updates from auto-repeating
    // controls.  Those that do not affect the printer page/layout do not need to
    // do this, the caller will perform a final updatePrintParameters() before
    // starting to print.

    // "Scaling" group box
    QGroupBox *grp = new QGroupBox(i18nc("@title:group", "Scaling"), this);
    QGridLayout *gl = new QGridLayout(grp);

    m_scaleRadios = new QButtonGroup(this);
    connect(m_scaleRadios, &QButtonGroup::idClicked, this, &ImgPrintDialog::slotScaleChanged);
    connect(m_scaleRadios, &QButtonGroup::idClicked, mUpdateTimer, QOverload<>::of(&QTimer::start));

    // Option 1: ScaleScreen
    QRadioButton *rb = new QRadioButton(i18nc("@option:radio", "Size as on screen"), this);
    rb->setToolTip(i18nc("@info:tooltip", "<div>Print at the same size as seen on screen, determined by the screen resolution.</div>"));
    m_scaleRadios->addButton(rb, KookaPrint::ScaleScreen);
    gl->addWidget(rb, 0, 0, Qt::AlignLeft);

    QLabel *l = new QLabel(i18n("Screen resolution:"), this);
    gl->addWidget(l, 0, 2, Qt::AlignRight);

    m_screenRes = new QLineEdit(this);			// for a consistent appearance
    m_screenRes->setReadOnly(true);
    m_screenRes->setToolTip(i18nc("@info:tooltip", "<div>This is the current screen resolution. It cannot be changed here.</div>"));
    gl->addWidget(m_screenRes, 0, 3);

    // Option 2: ScaleScan
    rb = new QRadioButton(i18nc("@option:radio", "Size as scanned"), this);
    rb->setToolTip(i18nc("@info:tooltip", "<div>Print at a size determined by the scan resolution. The resolution is saved with the image and used if available; if it is not, it can be entered here.</div>"));
    m_scaleRadios->addButton(rb, KookaPrint::ScaleScan);
    gl->addWidget(rb, 1, 0, Qt::AlignLeft);

    l = new QLabel(i18n("Scan resolution:"), this);
    gl->addWidget(l, 1, 2, Qt::AlignRight);

    m_dpi = new QSpinBox(this);
    m_dpi->setRange(50, 1200);
    m_dpi->setSuffix(i18nc("@item:intext abbreviation for 'dots per inch'", " dpi"));
    m_dpi->setToolTip(i18nc("@info:tooltip", "<div>This is the scan resolution as saved with the image. It can be changed if necessary.</div>"));
    connect(m_dpi, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            mUpdateTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    l->setBuddy(m_dpi);
    gl->addWidget(m_dpi, 1, 3);

    // Option 3: ScaleCustom
    rb = new QRadioButton(i18nc("@option:radio", "Custom size"), this);
    rb->setToolTip(i18nc("@info:tooltip", "<div>Print scaled to the specified size. The image is centered on the paper.</div>"));
    m_scaleRadios->addButton(rb, KookaPrint::ScaleCustom);
    gl->addWidget(rb, 2, 0, Qt::AlignLeft);

    l = new QLabel(i18nc("@label:spinbox", "Image width:"), this);
    gl->addWidget(l, 2, 2, Qt::AlignRight);

    m_sizeW = new QSpinBox(this);
    m_sizeW->setRange(10, 1000);
    m_sizeW->setSuffix(i18nc("@item:intext abbreviation for 'millimetres'", " mm"));
    m_sizeW->setToolTip(i18nc("@info:tooltip", "<div>The width at which the image will be printed.</div>"));
    connect(m_sizeW, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &ImgPrintDialog::slotCustomWidthChanged);
    connect(m_sizeW, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            mUpdateTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    l->setBuddy(m_sizeW);
    gl->addWidget(m_sizeW, 2, 3);

    l = new QLabel(i18nc("@label:spinbox", "Image height:"), this);
    gl->addWidget(l, 3, 2, Qt::AlignRight);

    m_sizeH = new QSpinBox(this);
    m_sizeH->setRange(10, 1000);
    m_sizeH->setSuffix(i18nc("@item:intext abbreviation for 'millimetres'", " mm"));
    m_sizeH->setToolTip(i18nc("@info:tooltip", "<div>The height at which the image will be printed.</div>"));
    connect(m_sizeH, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &ImgPrintDialog::slotCustomHeightChanged);
    connect(m_sizeH, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            mUpdateTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    l->setBuddy(m_sizeH);
    gl->addWidget(m_sizeH, 3, 3);

    // Option 4: ScaleFitPage
    rb = new QRadioButton(i18nc("@option:radio", "Fit to page"), this);
    rb->setToolTip(i18nc("@info:tooltip", "<div>Print using as much of the available space on the paper as possible. The aspect ratio can be maintained.</div>"));
    m_scaleRadios->addButton(rb, KookaPrint::ScaleFitPage);
    gl->addWidget(rb, 3, 0, Qt::AlignLeft);

    gl->setColumnMinimumWidth(1, 2*DialogBase::horizontalSpacing());
    layout->addWidget(grp);

    // Horizontal box for "Other Options" and "Print Layout"
    QHBoxLayout *hbox = new QHBoxLayout;

    // "Other Options" group box
    grp = new QGroupBox(i18nc("@title:group", "Other Options"), this);
    QVBoxLayout *vbl = new QVBoxLayout(grp);

    // Maintain Aspect option
    m_ratio = new QCheckBox(i18nc("@option:check", "Maintain aspect ratio"), this);
    m_ratio->setToolTip(i18nc("@info:tooltip", "<div>Adjust the height or width to maintain the original aspect ratio of the printed image.</div>"));
    connect(m_ratio, &QCheckBox::toggled, this, &ImgPrintDialog::slotAdjustCustomSize);
    connect(m_ratio, &QCheckBox::toggled, mUpdateTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    vbl->addWidget(m_ratio);

    // Draft Print option
    m_psDraft = new QCheckBox(i18nc("@option:check", "Low resolution (fast draft print)"), this);
    m_psDraft->setToolTip(i18nc("@info:tooltip", "<div>Print at as low a resolution as possible. This may not work as intended on all printers.</div>"));
    vbl->addWidget(m_psDraft);

    // Cut Marks option
    QHBoxLayout *chb = new QHBoxLayout(this);

    l = new QLabel(i18nc("@label:listbox", "Cut marks:"), this);
    chb->addWidget(l);

    m_cutsCombo = new QComboBox(this);
    m_cutsCombo->addItem(i18nc("@item:inlistbox", "None"), KookaPrint::CutMarksNone);
    m_cutsCombo->addItem(i18nc("@item:inlistbox", "For multiple pages"), KookaPrint::CutMarksMultiple);
    m_cutsCombo->addItem(i18nc("@item:inlistbox", "Always"), KookaPrint::CutMarksAlways);
    m_cutsCombo->setToolTip(i18nc("@info:tooltip", "<div>Select whether cut/join marks are printed. The marks will reduce the available printable area.</div>"));
    connect(m_cutsCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            mUpdateTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    l->setBuddy(m_cutsCombo);
    chb->addWidget(m_cutsCombo);
    vbl->addLayout(chb);

    vbl->addStretch(1);
    hbox->addWidget(grp);

    // "Print Layout" group box
    grp = new QGroupBox(i18nc("@title:group", "Print Layout"), this);
    gl = new QGridLayout(grp);
    int row = 0;

    // The image size in pixels
    l = new QLabel(i18nc("@label:textbox", "Image size:"), this);
    gl->addWidget(l, row, 0);
    mImageSize = new QLabel(i18nc("@info:status width,height pixels",
                                   "%1 x %2 pix", m_image->width(), m_image->height()), this);
    gl->addWidget(mImageSize, row, 1);
    ++row;

    // The available print area on the paper
    l = new QLabel(i18nc("@label:textbox", "Available print area:"), this);
    gl->addWidget(l, row, 0);
    mPrintArea = new QLabel("-", this);
    gl->addWidget(mPrintArea, row, 1);
    ++row;

    // The image print area, allowing for scaling and margins
    l = new QLabel(i18nc("@label:textbox", "Image print area:"), this);
    gl->addWidget(l, row, 0);
    mImageArea = new QLabel("-", this);
    gl->addWidget(mImageArea, row, 1);
    ++row;

    // How many pages will be printed
    l = new QLabel(i18nc("@label:textbox", "Pages required:"), this);
    gl->addWidget(l, row, 0);
    mPrintPages = new QLabel("-", this);
    gl->addWidget(mPrintPages, row, 1);
    ++row;

    gl->setRowStretch(row, 1);
    hbox->addWidget(grp);
    hbox->setStretchFactor(grp, 1);			// want this one to stretch

    layout->addLayout(hbox);
    layout->addStretch(1);

    // The initial default values come from the printer.
    initOptions();					// print parameters to GUI
    mUpdateTimer->stop();				// don't want a delayed update
    updatePrintParameters();				// but do it immediately
}


void ImgPrintDialog::initOptions()
{
    KookaPrint::ScaleOption scale = mPrinter->scaleOption();
    if (mPrinter->isCopyMode())
    {
        if (scale==KookaPrint::ScaleScreen) scale = KookaPrint::ScaleScan;
        m_scaleRadios->button(KookaPrint::ScaleScreen)->setEnabled(false);
        m_screenRes->setEnabled(false);
    }
    m_scaleRadios->button(scale)->setChecked(true);
    slotScaleChanged(scale);

    const int scanRes = mPrinter->scanResolution();
    if (scanRes!=-1) m_dpi->setValue(scanRes);		// custom resolution provided?
    else						// if not, get from image
    {
        // Get the scan resolution from the image
        const int imgResX = DPM_TO_DPI(m_image->dotsPerMeterX());
        const int imgResY = DPM_TO_DPI(m_image->dotsPerMeterY());
        if (imgResX!=imgResY) qCWarning(KOOKA_LOG) << "Different image resolutions" << imgResX << imgResY;
        if (imgResX!=0)
        {
            qCDebug(KOOKA_LOG) << "Resolution from image" << imgResX;
            m_dpi->setValue(imgResX);
        }
    }

    QSize printSize = mPrinter->printSize();
    m_sizeW->setValue(printSize.width());
    m_sizeH->setValue(printSize.height());

    int screenDpi = mPrinter->screenResolution();
    if (screenDpi==-1)					// screen resolution not provided
    {
        int resX = logicalDpiX();
        int resY = logicalDpiY();

        // TODO: check whether they differ by more than, say, 5%
        // and warn the user if so - scaling by screen resolution
        // in that case will not preserve the aspect ratio.
        screenDpi = (resX+resY)/2;
        mPrinter->setScreenResolution(screenDpi);	// pass our value to printer
    }
    m_screenRes->setText(i18nc("@info:status", "%1 dpi", QString::number(screenDpi)));

    m_ratio->setChecked(mPrinter->maintainAspect());
    m_psDraft->setChecked(mPrinter->lowResDraft());

    int idx = m_cutsCombo->findData(mPrinter->cutMarksOption());
    if (idx!=-1) m_cutsCombo->setCurrentIndex(idx);

    slotAdjustCustomSize();				// adjust height for aspect
}


QString ImgPrintDialog::checkValid() const
{
    const int id = m_scaleRadios->checkedId();
    if (id==KookaPrint::ScaleScan && m_dpi->value()==0)
    {
        return (i18n("The scan resolution must be specified for scaling to it."));
    }

    if (id==KookaPrint::ScaleCustom && (m_sizeW->value()==0 || m_sizeH->value()==0))
    {
        return (i18n("A valid size must be specified for custom scaling. One or both of the specified dimensions is zero."));
    }

    return (QString());				// no problems
}


void ImgPrintDialog::slotScaleChanged(int id)
{
    m_dpi->setEnabled(id==KookaPrint::ScaleScan);
    m_ratio->setEnabled(id==KookaPrint::ScaleCustom || id==KookaPrint::ScaleFitPage);
    m_sizeW->setEnabled(id==KookaPrint::ScaleCustom);
    m_sizeH->setEnabled(id==KookaPrint::ScaleCustom);
    m_screenRes->setEnabled(id==KookaPrint::ScaleScreen);
}


void ImgPrintDialog::slotCustomWidthChanged(int val)
{
    slotAdjustCustomSize();
}


void ImgPrintDialog::slotCustomHeightChanged(int val)
{
    if (!m_ratio->isChecked()) return;			// only if maintaining aspect

    QSignalBlocker blocker(m_sizeW);
    m_sizeW->setValue(qRound(double(val)*m_image->width()/m_image->height()));
}


void ImgPrintDialog::slotAdjustCustomSize()
{
    if (!m_ratio->isChecked()) return;			// only if maintaining aspect

    const double val = m_sizeW->value();		// current width setting
    QSignalBlocker blocker(m_sizeH);			// adjust height to suit
    m_sizeH->setValue(qRound(double(val)*m_image->height()/m_image->width()));
}


void ImgPrintDialog::updatePrintParameters()
{
    // get options from GUI and update the printer
    const KookaPrint::ScaleOption scale = static_cast<KookaPrint::ScaleOption>(m_scaleRadios->checkedId());
    qCDebug(KOOKA_LOG) << "scale option" << scale;
    mPrinter->setScaleOption(scale);

    QSize size(m_sizeW->value(), m_sizeH->value());
    qCDebug(KOOKA_LOG) << "print size" << size;
    mPrinter->setPrintSize(size);

    const bool asp = m_ratio->isChecked();
    qCDebug(KOOKA_LOG) << "maintain aspect?" << asp;
    mPrinter->setMaintainAspect(asp);

    const bool draft = m_psDraft->isChecked();
    qCDebug(KOOKA_LOG) << "low res draft?" << draft;
    mPrinter->setLowResDraft(draft);

    // No need to setScreenResolution() here, that has already been done
    // in initOptions() and it never changes.

    const int scanRes = m_dpi->value();
    qCDebug(KOOKA_LOG) << "scan res" << scanRes;
    mPrinter->setScanResolution(scanRes);

    const KookaPrint::CutMarksOption cuts = static_cast<KookaPrint::CutMarksOption>(m_cutsCombo->currentData().toInt());
    qCDebug(KOOKA_LOG) << "cut marks" << cuts;
    mPrinter->setCutMarks(cuts);

    // ask the printer to recalculate page parameters
    mPrinter->recalculatePrintParameters();

    // reflect them in the preview GUI
    size = mPrinter->availablePageArea();
    mPrintArea->setText(i18nc("@info:status width,height millimetres",
                              "%1 x %2 mm", size.width(), size.height()));

    size = mPrinter->imagePrintArea();
    mImageArea->setText(i18nc("@info:status width,height millimetres",
                              "%1 x %2 mm", size.width(), size.height()));

    size = mPrinter->pageCount();			// width=columns, height=rows
    int totalPages = size.height()*size.width();
    if (totalPages==1) mPrintPages->setText(i18nc("@info:status total", "%1", totalPages));
    else mPrintPages->setText(i18nc("@info:status total(rows,cols)",
                                    "%1 (%2 x %3)", totalPages, size.height(), size.width()));
}
