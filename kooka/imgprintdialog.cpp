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

#include <qmap.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qgroupbox.h>
#include <qspinbox.h>

#include <qdebug.h>
#include <klocalizedstring.h>

#include "kookaimage.h"


#define ID_SCREEN 0
#define ID_ORIG   1
#define ID_CUSTOM 2
#define ID_FITPAGE 3



#define DPM_TO_DPI(d)		qRound((d)*2.54/100)	// dots/metre -> dots/inch
#define DPI_TO_DPM(d)		qRound((d)*100/2.54)	// dots/inch -> dots/metre





ImgPrintDialog::ImgPrintDialog(const KookaImage *img, QWidget *pnt)
#ifdef KDE3
    : KPrintDialogPage(pnt, name),
#else
    : QWidget(pnt),
#endif

      m_image(img),
      m_ignoreSignal(false)
{
    setWindowTitle(i18nc("@title:tab", "Image"));	// used as tab title

    QVBoxLayout *layout = new QVBoxLayout(this);

    QGroupBox *grp = new QGroupBox(i18nc("@title:group", "Scaling"), this);

    // TODO: grid unified, radios and entry fields together
    QVBoxLayout *vbl = new QVBoxLayout(grp);
    //vbl->setMargin(0);

    m_scaleRadios = new QButtonGroup(this);
    connect(m_scaleRadios, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked),
            this, &ImgPrintDialog::slotScaleChanged);

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
    connect(m_sizeW, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ImgPrintDialog::slotCustomWidthChanged);
    l->setBuddy(m_sizeW);
    vbl->addWidget(m_sizeW);

    l = new QLabel(i18nc("@label:spinbox", "Image height:"), this);
    vbl->addWidget(l);

    m_sizeH = new QSpinBox(this);
    m_sizeH->setRange(10, 1000);
    m_sizeH->setSuffix(i18nc("@item:intext abbreviation for 'millimetres'", " mm"));
    connect(m_sizeH, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ImgPrintDialog::slotCustomHeightChanged);
    l->setBuddy(m_sizeH);
    vbl->addWidget(m_sizeH);

    m_ratio = new QCheckBox(i18nc("@option:check", "Maintain aspect ratio"), this);
    vbl->addWidget(m_ratio);

    QWidget *spaceEater = new QWidget(this);
    spaceEater->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored));
    layout->addWidget(spaceEater);

    // Set initial default values
    m_psDraft->setChecked(false);
    m_dpi->setValue(300);
    m_sizeW->setValue(100);
    m_sizeH->setValue(100);
    m_ratio->setChecked(true);
    m_scaleRadios->button(ID_SCREEN)->setChecked(true);
    slotScaleChanged(ID_SCREEN);
}


void ImgPrintDialog::setOptions(const QMap<QString, QString> &opts)
{
    //qDebug();

    qDebug() << "setting options:";
    for (QMap<QString, QString>::const_iterator it = opts.constBegin();
         it!=opts.constEnd(); ++it)
    {
        qDebug() << " " << qPrintable(it.key()) << it.value();
    }

    // m_autofit->setChecked(opts["app-img-autofit"] == "1");
    QString scale = opts[OPT_SCALING];
    if (scale=="scan") m_scaleRadios->button(ID_ORIG)->setChecked(true);
    else if (scale=="custom") m_scaleRadios->button(ID_CUSTOM)->setChecked(true);
    else if (scale=="fitpage") m_scaleRadios->button(ID_FITPAGE)->setChecked(true);
    else m_scaleRadios->button(ID_SCREEN)->setChecked(true);

    QString opt = opts[OPT_SCAN_RES];
    if (!opt.isEmpty()) m_dpi->setValue(opt.toInt());	// custom resolution provided?
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

    opt = opts[OPT_WIDTH];
    if (!opt.isEmpty()) m_sizeW->setValue(opt.toInt());
    opt = opts[OPT_HEIGHT];
    if (!opt.isEmpty()) m_sizeH->setValue(opt.toInt());

    mScreenDpi = opts[OPT_SCREEN_RES].toInt();
    if (mScreenDpi==0)					// screen resolution not provided
    {
        int resX = logicalDpiX();
        int resY = logicalDpiY();
        // TODO: check whether they differ by more than, say, 5%
        // and warn the user if so - scaling by screen resolution
        // in that case will not preserve the aspect ratio.
        mScreenDpi = (resX+resY)/2;
    }
    m_screenRes->setText(i18nc("@info:status", "Screen resolution: %1 dpi", mScreenDpi));

    opt = opts[OPT_PSGEN_DRAFT];
    if (!opt.isEmpty()) m_psDraft->setChecked(opt.toInt()==1);
    opt = opts[OPT_RATIO];
    if (!opt.isEmpty()) m_ratio->setChecked(opt.toInt()==1);
}

void ImgPrintDialog::getOptions(QMap<QString, QString> &opts, bool include_def)
{
    // TODO: Check for meaning of include_def !
    //
    // KDE3 kdelibs/kdeprint/kprintdialogpage.h says:
    // This function is called to fill the structure @p opts with
    // the selected options from this dialog page. If @p incldef is true,
    // include also options with default values, otherwise discard them.
    // Reimplement this function in subclasses.
    // @param opts the option set to fill
    // @param incldef if true, include also options with default values
   // //qDebug() << "In getOption with include_def: "  << include_def << endl;

    QString scale = "screen";
    if (m_scaleRadios->button(ID_ORIG)->isChecked()) scale = "scan";
    else if (m_scaleRadios->button(ID_CUSTOM)->isChecked()) scale = "custom";
    else if (m_scaleRadios->button(ID_FITPAGE)->isChecked()) scale = "fitpage";
    opts[OPT_SCALING] = scale;

    opts[OPT_SCAN_RES]    = QString::number(m_dpi->value());
    opts[OPT_WIDTH]       = QString::number(m_sizeW->value());
    opts[OPT_HEIGHT]      = QString::number(m_sizeH->value());
    opts[OPT_PSGEN_DRAFT] = QString::number(m_psDraft->isChecked());
    opts[OPT_RATIO]       = QString::number(m_ratio->isChecked());

    opts[OPT_SCREEN_RES]  = QString::number(mScreenDpi);
}


bool ImgPrintDialog::isValid(QString &msg)
{
    const int id = m_scaleRadios->checkedId();
    if (id==ID_ORIG && m_dpi->value()==0)
    {
        msg = i18n("The scan resolution must be specified for scaling to it.");
        return (false);
    }

    if (id==ID_CUSTOM && (m_sizeW->value()==0 || m_sizeH->value()==0))
    {
        msg = i18n("A valid size must be specified for custom scaling. One or both of the specified dimensions is zero.");
        return (false);
    }

    return (true);
}


void ImgPrintDialog::slotScaleChanged(int id)
{
    if (id == ID_SCREEN) {
        /* disalbe size, scan res. */
        m_dpi->setEnabled(false);
        m_ratio->setEnabled(false);
        m_sizeW->setEnabled(false);
        m_sizeH->setEnabled(false);
        m_screenRes->setEnabled(true);
    } else if (id == ID_ORIG) {
        /* disable size */
        m_dpi->setEnabled(true);
        m_ratio->setEnabled(false);
        m_sizeW->setEnabled(false);
        m_sizeH->setEnabled(false);
        m_screenRes->setEnabled(false);
    } else if (id == ID_CUSTOM) {
        m_dpi->setEnabled(false);
        m_ratio->setEnabled(true);
        m_sizeW->setEnabled(true);
        m_sizeH->setEnabled(true);
        m_screenRes->setEnabled(false);
    } else if (id == ID_FITPAGE) {
        m_dpi->setEnabled(false);
        m_ratio->setEnabled(true);
        m_sizeW->setEnabled(false);
        m_sizeH->setEnabled(false);
        m_screenRes->setEnabled(false);
    }
}

void ImgPrintDialog::slotCustomWidthChanged(int val)
{
    if (m_ignoreSignal) {
        m_ignoreSignal = false;
        return;
    }

    /* go out here if scaling is not custom */
    if (m_scaleRadios->checkedId() != ID_CUSTOM) {
        return;
    }

    /* go out here if maintain aspect ration is off */
    if (! m_ratio->isChecked()) {
        return;
    }

    // TODO: use blockSignals()
    m_ignoreSignal = true;
    //qDebug() << "Setting value to horizontal size";
    m_sizeH->setValue(int(double(val) *
                          double(m_image->height()) / double(m_image->width())));

}

void ImgPrintDialog::slotCustomHeightChanged(int val)
{
    if (m_ignoreSignal) {
        m_ignoreSignal = false;
        return;
    }

    /* go out here if scaling is not custom */
    if (m_scaleRadios->checkedId() != ID_CUSTOM) {
        return;
    }

    /* go out here if maintain aspect ration is off */
    if (! m_ratio->isChecked()) {
        return;
    }

    m_ignoreSignal = true;
    //qDebug() << "Setting value to vertical size";
    m_sizeW->setValue(int(double(val) *
                          double(m_image->width()) / double(m_image->height())));

}
