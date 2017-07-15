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
#include <qtooltip.h>
#include <qregexp.h>
#include <qgridlayout.h>
#include <qprogressbar.h>
#include <qpushbutton.h>
#include <qdebug.h>

#include <klocalizedstring.h>
#include <kseparator.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include <kconfigskeleton.h>

#include "kookaimage.h"
#include "kookapref.h"
#include "kookasettings.h"

#include "ocrgocrengine.h"


OcrGocrDialog::OcrGocrDialog(QWidget *parent)
    : OcrBaseDialog(parent),
      m_ocrCmd(QString::null),
      m_isBW(false)
{
}

OcrGocrDialog::~OcrGocrDialog()
{
}

QString OcrGocrDialog::ocrEngineLogo() const
{
    return ("gocr.png");
}

QString OcrGocrDialog::ocrEngineName() const
{
    return (OcrEngine::engineName(OcrEngine::EngineGocr));
}

QString OcrGocrDialog::ocrEngineDesc() const
{
    return (OcrGocrEngine::engineDesc());
}

OcrEngine::EngineError OcrGocrDialog::setupGui()
{
    OcrBaseDialog::setupGui();

    QWidget *w = addExtraSetupWidget();
    QGridLayout *gl = new QGridLayout(w);

    KConfigSkeletonItem *ski = KookaSettings::self()->ocrGocrGrayLevelItem();
    Q_ASSERT(ski!=NULL);
    QLabel *l = new QLabel(ski->label(), w);
    gl->addWidget(l, 0, 0);

    sliderGrayLevel = new KScanSlider(w, QString::null, 0, 254, true, 160);
    int numdefault = KookaSettings::ocrGocrGrayLevel();
    sliderGrayLevel->setValue(numdefault);
    sliderGrayLevel->setToolTip(ski->toolTip());
    l->setBuddy(sliderGrayLevel);
    gl->addWidget(sliderGrayLevel, 0, 1);

    ski = KookaSettings::self()->ocrGocrDustSizeItem();
    Q_ASSERT(ski!=NULL);
    l = new QLabel(ski->label(), w);
    gl->addWidget(l, 1, 0);
    sliderDustSize = new KScanSlider(w, QString::null, 0, 60, true, 10);
    numdefault = KookaSettings::ocrGocrDustSize();
    sliderDustSize->setValue(numdefault);
    sliderDustSize->setToolTip(ski->toolTip());
    l->setBuddy(sliderDustSize);
    gl->addWidget(sliderDustSize, 1, 1);

    ski = KookaSettings::self()->ocrGocrSpaceWidthItem();
    Q_ASSERT(ski!=NULL);
    l = new QLabel(ski->label(), w);
    gl->addWidget(l, 2, 0);
    sliderSpace = new KScanSlider(w, QString::null, 0, 60, true, 0);
    numdefault = KookaSettings::ocrGocrSpaceWidth();
    sliderSpace->setValue(numdefault);
    sliderSpace->setToolTip(ski->toolTip());
    l->setBuddy(sliderSpace);
    gl->addWidget(sliderSpace, 2, 1);

    gl->setRowStretch(3, 1);                // for top alignment

    /* find the GOCR binary */
    QString res = KookaSettings::ocrGocrBinary();
    if (res.isEmpty()) {
        res = KookaPref::tryFindGocr();
        if (res.isEmpty()) {
            /* Popup here telling that the config needs to be called */
            KMessageBox::sorry(this, i18n("The path to the GOCR binary is not configured or is not valid.\n"
                                          "Please enter or check the path in the Kooka configuration."),
                               i18n("GOCR Software Not Found"));
            buttonBox()->button(QDialogButtonBox::Yes)->setEnabled(false);
        }
    }

    /* retrieve program version and display */
    if (res.isEmpty()) {
        res = i18n("Not found");
    } else {
        m_ocrCmd = res;
    }
    ocrShowInfo(res, version());            // show binary and version

    progressBar()->setMaximum(0);           // animation only

    m_setupWidget = w;
    return (OcrEngine::ENG_OK);
}

void OcrGocrDialog::introduceImage(const KookaImage *img)
{
    OcrBaseDialog::introduceImage(img);
    if (img == NULL || img->isNull()) {
        return;
    }

    m_isBW = true;
    if (img->colorCount() > 0 && img->colorCount() <= 2) {
        //qDebug() << "Have" << img->numColors() << "colors on depth" << img->depth();
        /* that means it is a black-and-white image. Thus we do not need the GrayLevel slider */
        m_isBW = false;
    }

    if (sliderGrayLevel != NULL) {
        sliderGrayLevel->setEnabled(m_isBW);
    }
}

void OcrGocrDialog::slotWriteConfig(void)
{
    //qDebug();

    OcrBaseDialog::slotWriteConfig();

    KookaSettings::setOcrGocrBinary(getOCRCmd());

    KookaSettings::setOcrGocrGrayLevel(getGraylevel());
    KookaSettings::setOcrGocrDustSize(getDustsize());
    KookaSettings::setOcrGocrSpaceWidth(getSpaceWidth());
    KookaSettings::self()->save();
}

void OcrGocrDialog::enableFields(bool enable)
{
    m_setupWidget->setEnabled(enable);
}

QString OcrGocrDialog::version()
{
    //qDebug() << "of" << m_ocrCmd;

    QString vers;

    KProcess proc;
    proc.setOutputChannelMode(KProcess::MergedChannels);
    proc << m_ocrCmd << "-h";

    int status = proc.execute(5000);
    if (status == 0) {
        QByteArray output = proc.readAllStandardOutput();
        QRegExp rx("-- gocr ([\\d\\.\\s]+)");
        if (rx.indexIn(output) > -1) {
            vers = rx.cap(1);
        } else {
            vers = i18n("Unknown");
        }
    } else {
        //qDebug() << "failed with status" << status;
        vers = i18n("Error");
    }

    return (vers);
}
