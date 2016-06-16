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
#include <qradiobutton.h>
#include <qgroupbox.h>
#include <qspinbox.h>
#include <qtimer.h>
#include <qprintdialog.h>
#include <qdebug.h>
#include <QSignalBlocker>

#include <klocalizedstring.h>

#include "kookaimage.h"
#include "kookaprint.h"


#define ID_SCREEN 0
#define ID_ORIG   1
#define ID_CUSTOM 2
#define ID_FITPAGE 3



#define DPM_TO_DPI(d)		qRound((d)*2.54/100)	// dots/metre -> dots/inch
#define DPI_TO_DPM(d)		qRound((d)*100/2.54)	// dots/inch -> dots/metre



ImgPrintDialog::ImgPrintDialog(const KookaImage *img, KookaPrint *prt, QWidget *pnt)
#ifdef KDE3
    : KPrintDialogPage(pnt),
#else
    : QWidget(pnt),
#endif
      m_image(img)
{
    setWindowTitle(i18nc("@title:tab", "Image"));	// used as tab title

    mPrinter = prt;					// record the printer

    // Timer for delayed/combined updates of print parameters
    mUpdateTimer = new QTimer(this);
    mUpdateTimer->setSingleShot(true);
    // The default button auto-repeat delay is set to 300
    // in qtbase/src/widgets/widgets/qabstractbutton.cpp
    mUpdateTimer->setInterval(320);			// longer than button auto-repeat
    connect(mUpdateTimer, &QTimer::timeout, this, &ImgPrintDialog::updatePrintParameters);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QGroupBox *grp = new QGroupBox(i18nc("@title:group", "Scaling"), this);

    // TODO: grid unified, radios and entry fields together
    QVBoxLayout *vbl = new QVBoxLayout(grp);

    // Each control (or group of controls) created here that affect the printer
    // page or layout needs to be connected to mUpdateTimer->start().  This will
    // call updatePrintParameters() on a delay to combine updates from auto-repeating
    // controls.  Those that do not affect the printer page/layout do not need to
    // do this, the caller will perform a final updatePrintParameters() before
    // starting to print.

    m_scaleRadios = new QButtonGroup(this);
    connect(m_scaleRadios, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked),
            this, &ImgPrintDialog::slotScaleChanged);
    connect(m_scaleRadios, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked),
            mUpdateTimer, static_cast<void (QTimer::*)()>(&QTimer::start));

    QRadioButton *rb = new QRadioButton(i18nc("@option:radio", "Same size as on screen"), this);
    rb->setToolTip(i18nc("@info;tooltip", "<div>Print at the same size as seen on screen, determined by the screen resolution.</div>"));
    m_scaleRadios->addButton(rb, ID_SCREEN);
    vbl->addWidget(rb);

    rb = new QRadioButton(i18nc("@option:radio", "Size from scan resolution"), this);
    rb->setToolTip(i18nc("@info:tooltip", "<div>Print at a size determined by the scan resolution. The scan resolution is saved with the image and used if available; if it is not, it can be entered below.</div>"));
    m_scaleRadios->addButton(rb, ID_ORIG);
    vbl->addWidget(rb);

    rb = new QRadioButton(i18nc("@option:radio", "Custom size"), this);
    rb->setToolTip(i18nc("@info:tooltip", "<div>Print scaled to the size as specified below. The image is centered on the paper.</div>"));
    m_scaleRadios->addButton(rb, ID_CUSTOM);
    vbl->addWidget(rb);

    rb = new QRadioButton(i18nc("@option:radio", "Fit to page"), this);
    rb->setToolTip(i18nc("@info:tooltip", "<div>Print using as much of the available space on the paper as possible. The aspect ratio can be maintained.</div>"));
    m_scaleRadios->addButton(rb, ID_FITPAGE);
    vbl->addWidget(rb);

    layout->addWidget(grp);

    QHBoxLayout *hbox = new QHBoxLayout;
    layout->addLayout(hbox);

    /** Box for Image Resolutions **/
    QGroupBox *group1 = new QGroupBox(i18nc("@title:group", "Resolutions"), this);
    hbox->addWidget(group1);
    vbl = new QVBoxLayout(group1);

    /* Postscript generation resolution  */
    m_psDraft = new QCheckBox(i18nc("@option:check", "Generate low resolution PostScript (fast draft print)"), this);
    vbl->addWidget(m_psDraft);

    // TODO: rename this "Resolution to use"
    // show screen or image res (whatever is used)
    // then eliminate m_screenRes below

    /* Scan resolution of the image */
    QLabel *l = new QLabel(i18nc("@label:spinbox", "Scan resolution:"), this);
    vbl->addWidget(l);

    m_dpi = new QSpinBox(group1);
    m_dpi->setRange(50, 1200);
    m_dpi->setSuffix(i18nc("@item:intext abbreviation for 'dots per inch'", " dpi"));
    connect(m_dpi, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            mUpdateTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    l->setBuddy(m_dpi);
    vbl->addWidget(m_dpi);

    /* Label for displaying the screen Resolution */
    m_screenRes = new QLabel(group1);
    vbl->addWidget(m_screenRes);

    /** Box for Image Print Size **/
    QGroupBox *group2 = new QGroupBox(i18nc("@title:group", "Image Print Size"), this);
    hbox->addWidget(group2);
    vbl = new QVBoxLayout(group2);

    l = new QLabel(i18nc("@label:spinbox", "Image width:"), this);
    vbl->addWidget(l);

    m_sizeW = new QSpinBox(this);
    m_sizeW->setRange(10, 1000);
    m_sizeW->setSuffix(i18nc("@item:intext abbreviation for 'millimetres'", " mm"));
    connect(m_sizeW, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &ImgPrintDialog::slotCustomWidthChanged);
    connect(m_sizeW, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            mUpdateTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    l->setBuddy(m_sizeW);
    vbl->addWidget(m_sizeW);

    l = new QLabel(i18nc("@label:spinbox", "Image height:"), this);
    vbl->addWidget(l);

    m_sizeH = new QSpinBox(this);
    m_sizeH->setRange(10, 1000);
    m_sizeH->setSuffix(i18nc("@item:intext abbreviation for 'millimetres'", " mm"));
    connect(m_sizeH, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &ImgPrintDialog::slotCustomHeightChanged);
    connect(m_sizeH, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            mUpdateTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    l->setBuddy(m_sizeH);
    vbl->addWidget(m_sizeH);

    // TODO: need to recalculate custom size if this option is
    // set initially or is toggled on
    m_ratio = new QCheckBox(i18nc("@option:check", "Maintain aspect ratio"), this);
    connect(m_ratio, &QCheckBox::toggled,
            mUpdateTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    vbl->addWidget(m_ratio);

    QWidget *spaceEater = new QWidget(this);
    spaceEater->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored));
    layout->addWidget(spaceEater);

    // The initial default values come from the printer.

    initOptions();					// print parameters to GUI
    mUpdateTimer->stop();				// don't want a delayed update
    updatePrintParameters();				// but do it immediately
}


void ImgPrintDialog::initOptions()
{
    KookaPrint::ScaleOption scale = printer()->scaleOption();
    if (scale==KookaPrint::ScaleScan) m_scaleRadios->button(ID_ORIG)->setChecked(true);
    else if (scale==KookaPrint::ScaleCustom) m_scaleRadios->button(ID_CUSTOM)->setChecked(true);
    else if (scale==KookaPrint::ScaleFitPage) m_scaleRadios->button(ID_FITPAGE)->setChecked(true);
    else m_scaleRadios->button(ID_SCREEN)->setChecked(true);
    slotScaleChanged(m_scaleRadios->checkedId());

    const int scanRes = printer()->scanResolution();
    if (scanRes!=-1) m_dpi->setValue(scanRes);		// custom resolution provided?
    else						// if not, get from image
    {
        // Get the scan resolution from the image
        const int imgResX = DPM_TO_DPI(m_image->dotsPerMeterX());
        const int imgResY = DPM_TO_DPI(m_image->dotsPerMeterY());
        if (imgResX!=imgResY) qWarning() << "Different image resolutions" << imgResX << imgResY;
        if (imgResX!=0)
        {
            qDebug() << "Resolution from image" << imgResX;
            m_dpi->setValue(imgResX);
        }
    }

    QSize printSize = printer()->printSize();
    m_sizeW->setValue(printSize.width());
    m_sizeH->setValue(printSize.height());

    mScreenDpi = printer()->screenResolution();
    if (mScreenDpi==-1)					// screen resolution not provided
    {
        int resX = logicalDpiX();
        int resY = logicalDpiY();

        // TODO: check whether they differ by more than, say, 5%
        // and warn the user if so - scaling by screen resolution
        // in that case will not preserve the aspect ratio.
        mScreenDpi = (resX+resY)/2;
    }
    m_screenRes->setText(i18nc("@info:status", "Screen resolution: %1 dpi", mScreenDpi));

    m_ratio->setChecked(printer()->maintainAspect());
    m_psDraft->setChecked(printer()->lowResDraft());
}


QString ImgPrintDialog::checkValid() const
{
    const int id = m_scaleRadios->checkedId();
    if (id==ID_ORIG && m_dpi->value()==0)
    {
        return (i18n("The scan resolution must be specified for scaling to it."));
    }

    if (id==ID_CUSTOM && (m_sizeW->value()==0 || m_sizeH->value()==0))
    {
        return (i18n("A valid size must be specified for custom scaling. One or both of the specified dimensions is zero."));
    }

    return (QString::null);				// no problems
}


void ImgPrintDialog::slotScaleChanged(int id)
{
    m_dpi->setEnabled(id==ID_ORIG);
    m_ratio->setEnabled(id==ID_CUSTOM || id==ID_FITPAGE);
    m_sizeW->setEnabled(id==ID_CUSTOM);
    m_sizeH->setEnabled(id==ID_CUSTOM);
    m_screenRes->setEnabled(id==ID_SCREEN);
}


void ImgPrintDialog::slotCustomWidthChanged(int val)
{
    if (m_scaleRadios->checkedId()!=ID_CUSTOM) return;	// only for custom scaling
    if (!m_ratio->isChecked()) return;			// only if maintaining aspect

    QSignalBlocker blocker(m_sizeH);
    m_sizeH->setValue(qRound(double(val)*m_image->height()/m_image->width()));
}


void ImgPrintDialog::slotCustomHeightChanged(int val)
{
    if (m_scaleRadios->checkedId()!=ID_CUSTOM) return;	// only for custom scaling
    if (!m_ratio->isChecked()) return;			// only if maintaining aspect

    QSignalBlocker blocker(m_sizeW);
    m_sizeW->setValue(qRound(double(val)*m_image->width()/m_image->height()));
}


void ImgPrintDialog::updatePrintParameters()
{
    KookaPrint *pr = printer();

    const QImage *img = pr->image();
    qDebug() << "image size" << img->size();

    KookaPrint::ScaleOption scale = KookaPrint::ScaleScreen;
    if (m_scaleRadios->button(ID_ORIG)->isChecked()) scale = KookaPrint::ScaleScan;
    else if (m_scaleRadios->button(ID_CUSTOM)->isChecked()) scale = KookaPrint::ScaleCustom;
    else if (m_scaleRadios->button(ID_FITPAGE)->isChecked()) scale = KookaPrint::ScaleFitPage;
    qDebug() << "scale option" << scale;
    printer()->setScaleOption(scale);

    const QSize size(m_sizeW->value(), m_sizeH->value());
    qDebug() << "print size" << size;
    printer()->setPrintSize(size);

    const bool asp = m_ratio->isChecked();
    qDebug() << "maintain aspect?" << asp;
    printer()->setMaintainAspect(asp);

    const bool draft = m_psDraft->isChecked();
    qDebug() << "low res draft?" << draft;
    printer()->setLowResDraft(draft);

    const int scanRes = m_dpi->value();
    qDebug() << "scan res" << scanRes;
    printer()->setScanResolution(scanRes);

    qDebug() << "screen res" << mScreenDpi;		// calculated in initOptions()
    printer()->setScreenResolution(mScreenDpi);



    // get printer to calculate page parameters

    // reflect them in GUI


}
