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

#include "ocrgocrdialog.h"

#include <qlabel.h>
#include <qgridlayout.h>
#include <qprogressbar.h>
#include <qregularexpression.h>
#include <qspinbox.h>

#include <klocalizedstring.h>
#include <kprocess.h>
#include <kconfigskeleton.h>

#include "scanimage.h"
#include "kookapref.h"
#include "kookasettings.h"

#include "ocrgocrengine.h"
#include "kscancontrols.h"


OcrGocrDialog::OcrGocrDialog(AbstractOcrEngine *plugin, QWidget *pnt)
    : AbstractOcrDialogue(plugin, pnt),
      m_ocrCmd(QString())
{
}


bool OcrGocrDialog::setupGui()
{
    AbstractOcrDialogue::setupGui();

    QWidget *w = addExtraSetupWidget();
    QGridLayout *gl = new QGridLayout(w);

    KConfigSkeletonItem *ski = KookaSettings::self()->ocrGocrGrayLevelItem();
    Q_ASSERT(ski!=nullptr);
    QLabel *l = new QLabel(ski->label(), w);
    gl->addWidget(l, 0, 0);

    sliderGrayLevel = new KScanSlider(w, QString(), true);
    sliderGrayLevel->setRange(0, 254, -1, 160);
    sliderGrayLevel->setValue(KookaSettings::ocrGocrGrayLevel());
    sliderGrayLevel->setToolTip(ski->toolTip());
    sliderGrayLevel->spinBox()->setSpecialValueText(i18n("Automatic"));
    l->setBuddy(sliderGrayLevel);
    gl->addWidget(sliderGrayLevel, 0, 1);

    ski = KookaSettings::self()->ocrGocrDustSizeItem();
    Q_ASSERT(ski!=nullptr);
    l = new QLabel(ski->label(), w);
    gl->addWidget(l, 1, 0);
    sliderDustSize = new KScanSlider(w, QString(), true);
    sliderDustSize->setRange(-1, 20, -1, 10);
    sliderDustSize->setValue(KookaSettings::ocrGocrDustSize());
    sliderDustSize->setToolTip(ski->toolTip());
    sliderDustSize->spinBox()->setSpecialValueText(i18n("Automatic"));
    l->setBuddy(sliderDustSize);
    gl->addWidget(sliderDustSize, 1, 1);

    ski = KookaSettings::self()->ocrGocrSpaceWidthItem();
    Q_ASSERT(ski!=nullptr);
    l = new QLabel(ski->label(), w);
    gl->addWidget(l, 2, 0);
    sliderSpace = new KScanSlider(w, QString(), true);
    sliderSpace->setRange(0, 60, -1, 0);
    sliderSpace->setValue(KookaSettings::ocrGocrSpaceWidth());
    sliderSpace->setToolTip(ski->toolTip());
    sliderSpace->spinBox()->setSpecialValueText(i18n("Automatic"));
    l->setBuddy(sliderSpace);
    gl->addWidget(sliderSpace, 2, 1);

    ski = KookaSettings::self()->ocrGocrCertaintyItem();
    Q_ASSERT(ski!=nullptr);
    l = new QLabel(ski->label(), w);
    gl->addWidget(l, 3, 0);
    sliderCertainty = new KScanSlider(w, QString(), true);
    sliderCertainty->setRange(5, 100, -1, 95);
    sliderCertainty->setValue(KookaSettings::ocrGocrCertainty());
    sliderCertainty->setToolTip(ski->toolTip());
    l->setBuddy(sliderCertainty);
    gl->addWidget(sliderCertainty, 3, 1);

    gl->setRowStretch(4, 1);				// for top alignment

    /* find the GOCR binary */
    m_ocrCmd = engine()->findExecutable(&KookaSettings::ocrGocrBinary, KookaSettings::self()->ocrGocrBinaryItem());
    if (m_ocrCmd.isEmpty())				// found, get its version
    {
        engine()->setErrorText(i18n("The GOCR executable is not configured or is not available."));
    }

    ocrShowInfo(m_ocrCmd, version());			// the show binary and version
    progressBar()->setMaximum(0);			// progress animation only

    m_setupWidget = w;
    return (!m_ocrCmd.isEmpty());
}


void OcrGocrDialog::introduceImage(ScanImage::Ptr img)
{
    AbstractOcrDialogue::introduceImage(img);
    if (img==nullptr || img->isNull()) return;

    // See if the image is black-and-white, where the GreyLevel slider is not needed.
    // This was originally (as a member variable) called 'm_isBW', but the logic was
    // the other way round!
    if (sliderGrayLevel!=nullptr) sliderGrayLevel->setEnabled(!engine()->isBW());
}


void OcrGocrDialog::slotWriteConfig()
{
    AbstractOcrDialogue::slotWriteConfig();

    KookaSettings::setOcrGocrBinary(getOCRCmd());

    KookaSettings::setOcrGocrGrayLevel(getGraylevel());
    KookaSettings::setOcrGocrDustSize(getDustsize());
    KookaSettings::setOcrGocrSpaceWidth(getSpaceWidth());
    KookaSettings::setOcrGocrCertainty(getCertainty());
    KookaSettings::self()->save();
}

void OcrGocrDialog::enableFields(bool enable)
{
    m_setupWidget->setEnabled(enable);
}


int OcrGocrDialog::getGraylevel() const
{
    return (sliderGrayLevel->value());
}


int OcrGocrDialog::getDustsize() const
{
    return (sliderDustSize->value());
}


int OcrGocrDialog::getSpaceWidth() const
{
    return (sliderSpace->value());
}


int OcrGocrDialog::getCertainty() const
{
    return (sliderCertainty->value());
}


QString OcrGocrDialog::version()
{
    QString vers;

    KProcess proc;
    proc.setOutputChannelMode(KProcess::MergedChannels);
    proc << m_ocrCmd << "-h";

    int status = proc.execute(5000);
    if (status == 0)
    {
        QByteArray output = proc.readAllStandardOutput();
        const QRegularExpression rx("-- gocr ([\\d\\.\\s]+)");
        const QRegularExpressionMatch match = rx.match(output);
        if (match.hasMatch()) vers = match.captured(1);
        else vers = i18n("Unknown");
    }
    else vers = i18n("Error");

    return (vers);
}
