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

#include "ocrocraddialog.h"

#include <qlabel.h>
#include <qregexp.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qspinbox.h>
#include <qlayout.h>
#include <qprogressbar.h>
#include <qdebug.h>

#include <klocalizedstring.h>
#include <kurlrequester.h>
#include <kprocess.h>

#include "kookaimage.h"
#include "kookapref.h"
#include "kookasettings.h"
#include "kscancontrols.h"
#include "dialogbase.h"

#include "ocrocradengine.h"


OcrOcradDialog::OcrOcradDialog(AbstractOcrEngine *plugin, QWidget *pnt)
    : AbstractOcrDialogue(plugin, pnt),
      m_setupWidget(nullptr),
      m_orfUrlRequester(nullptr),
      m_layoutMode(0),
      m_ocrCmd(QString()),
      m_versionNum(0),
      m_versionStr(QString())
{
}


bool OcrOcradDialog::setupGui()
{
    AbstractOcrDialogue::setupGui();			// build the standard GUI

    // Options available vary with the OCRAD version.  So we need to find
    // the OCRAD binary and get its version before creating the GUI.
    m_ocrCmd = engine()->findExecutable(&KookaSettings::ocrOcradBinary, KookaSettings::self()->ocrOcradBinaryItem());

    if (!m_ocrCmd.isEmpty()) getVersion(m_ocrCmd);	// found, get its version
    else						// not found or invalid
    {
        engine()->setErrorText(i18n("The OCRAD executable is not configured or is not available."));
    }

    QWidget *w = addExtraSetupWidget();
    QGridLayout *gl = new QGridLayout(w);

    // Layout detection mode, dependent on OCRAD version
    KConfigSkeletonItem *ski = KookaSettings::self()->ocrOcradLayoutDetectionItem();
    Q_ASSERT(ski!=nullptr);
    QLabel *l = new QLabel(ski->label(), w);
    gl->addWidget(l, 0, 0);

    m_layoutMode = new QComboBox(w);
    m_layoutMode->addItem(i18n("No Layout Detection"), 0);
    if (m_versionNum >= 18) {           // OCRAD 0.18 or later
        // has only on/off
        m_layoutMode->addItem(i18n("Layout Detection"), 1);
    } else {                    // OCRAD 0.17 or earlier
        // had these 3 options
        m_layoutMode->addItem(i18n("Column Detection"), 1);
        m_layoutMode->addItem(i18n("Full Layout Detection"), 2);
    }

    m_layoutMode->setCurrentIndex(KookaSettings::ocrOcradLayoutDetection());
    m_layoutMode->setToolTip(ski->toolTip());
    gl->addWidget(m_layoutMode, 0, 1);
    l->setBuddy(m_layoutMode);

    gl->setRowMinimumHeight(1, DialogBase::verticalSpacing());

    // Character set, auto detected values
    QStringList vals = getValidValues("charset");
    ski = KookaSettings::self()->ocrOcradCharsetItem();
    Q_ASSERT(ski!=nullptr);
    l = new QLabel(ski->label(), w);
    gl->addWidget(l, 2, 0);
    m_characterSet = new QComboBox(w);
    m_characterSet->setToolTip(ski->toolTip());
    m_characterSet->addItem(i18n("(default)"), false);
    for (QStringList::const_iterator it = vals.constBegin(); it != vals.constEnd(); ++it) {
        m_characterSet->addItem(*it, true);
    }

    if (vals.count() == 0) m_characterSet->setEnabled(false);
    else {
        int ix = m_characterSet->findText(KookaSettings::ocrOcradCharset());
        if (ix != -1) m_characterSet->setCurrentIndex(ix);
    }
    gl->addWidget(m_characterSet, 2, 1);
    l->setBuddy(m_characterSet);

    // Filter, auto detected values
    vals = getValidValues("filter");
    ski = KookaSettings::self()->ocrOcradFilterItem();
    Q_ASSERT(ski!=nullptr);
    l = new QLabel(ski->label(), w);
    gl->addWidget(l, 3, 0);
    m_filter = new QComboBox(w);
    m_filter->setToolTip(ski->toolTip());
    m_filter->addItem(i18n("(default)"), false);
    for (QStringList::const_iterator it = vals.constBegin(); it != vals.constEnd(); ++it) {
        m_filter->addItem(*it, true);
    }

    if (vals.count() == 0) m_filter->setEnabled(false);
    else {
        int ix = m_filter->findText(KookaSettings::ocrOcradFilter());
        if (ix != -1) m_filter->setCurrentIndex(ix);
    }
    gl->addWidget(m_filter, 3, 1);
    l->setBuddy(m_filter);

    // Transform, auto detected values
    vals = getValidValues("transform");
    ski = KookaSettings::self()->ocrOcradTransformItem();
    Q_ASSERT(ski!=nullptr);
    l = new QLabel(ski->label(), w);
    gl->addWidget(l, 4, 0);
    m_transform = new QComboBox(w);
    m_transform->setToolTip(ski->toolTip());
    m_transform->addItem(i18n("(default)"), false);
    for (QStringList::const_iterator it = vals.constBegin(); it != vals.constEnd(); ++it) {
        m_transform->addItem(*it, true);
    }

    if (vals.count() == 0) m_transform->setEnabled(false);
    else {
        int ix = m_transform->findText(KookaSettings::ocrOcradTransform());
        if (ix != -1) m_transform->setCurrentIndex(ix);
    }
    gl->addWidget(m_transform, 4, 1);
    l->setBuddy(m_transform);

    gl->setRowMinimumHeight(5, DialogBase::verticalSpacing());

    // Invert option, on/off
    ski = KookaSettings::self()->ocrOcradInvertItem();
    Q_ASSERT(ski!=nullptr);
    m_invert = new QCheckBox(ski->label(), w);
    m_invert->setChecked(KookaSettings::ocrOcradInvert());
    m_invert->setToolTip(ski->toolTip());
    gl->addWidget(m_invert, 6, 1, Qt::AlignLeft);

    gl->setRowMinimumHeight(7, DialogBase::verticalSpacing());

    // Threshold, on/off and slider
    ski = KookaSettings::self()->ocrOcradThresholdEnableItem();
    Q_ASSERT(ski!=nullptr);
    m_thresholdEnable = new QCheckBox(ski->label(), w);
    m_thresholdEnable->setChecked(KookaSettings::ocrOcradThresholdEnable());
    m_thresholdEnable->setToolTip(ski->toolTip());
    gl->addWidget(m_thresholdEnable, 8, 1, Qt::AlignLeft);

    ski = KookaSettings::self()->ocrOcradThresholdValueItem();
    Q_ASSERT(ski!=nullptr);
    m_thresholdSlider = new KScanSlider(w, ski->label(), 0, 100);
    m_thresholdSlider->setValue(KookaSettings::ocrOcradThresholdValue());
    m_thresholdSlider->setToolTip(ski->toolTip());
    m_thresholdSlider->spinBox()->setSuffix("%");
    gl->addWidget(m_thresholdSlider, 9, 1);

    l = new QLabel(m_thresholdSlider->label(), w);
    gl->addWidget(l, 9, 0);
    l->setBuddy(m_thresholdSlider);

    connect(m_thresholdEnable, &QCheckBox::toggled, m_thresholdSlider, &KScanSlider::setEnabled);
    m_thresholdSlider->setEnabled(m_thresholdEnable->isChecked());

    gl->setRowStretch(10, 1);				// for top alignment
    gl->setColumnStretch(1, 1);

    ocrShowInfo(m_ocrCmd, m_versionStr);		// show the binary and version
    progressBar()->setMaximum(0);			// progress animation only

    m_setupWidget = w;
    return (!m_ocrCmd.isEmpty());
}


void OcrOcradDialog::slotWriteConfig()
{
    AbstractOcrDialogue::slotWriteConfig();

    KookaSettings::setOcrOcradBinary(getOCRCmd());
    KookaSettings::setOcrOcradLayoutDetection(m_layoutMode->currentIndex());

    int ix = m_characterSet->currentIndex();
    QString value = (m_characterSet->itemData(ix).toBool() ? m_characterSet->currentText() : QString());
    KookaSettings::setOcrOcradCharset(value);

    ix = m_filter->currentIndex();
    value = (m_filter->itemData(ix).toBool() ? m_filter->currentText() : QString());
    KookaSettings::setOcrOcradFilter(value);

    ix = m_transform->currentIndex();
    value = (m_transform->itemData(ix).toBool() ? m_transform->currentText() : QString());
    KookaSettings::setOcrOcradTransform(value);

    KookaSettings::setOcrOcradInvert(m_invert->isChecked());
    KookaSettings::setOcrOcradThresholdEnable(m_thresholdEnable->isChecked());
    KookaSettings::setOcrOcradThresholdValue(m_thresholdSlider->value());
}


void OcrOcradDialog::enableFields(bool enable)
{
    m_setupWidget->setEnabled(enable);
}


/* Later: Allow interactive loading of ORF files */
QString OcrOcradDialog::orfUrl() const
{
    if (m_orfUrlRequester != nullptr) {
        return (m_orfUrlRequester->url().url());
    } else {
        return (QString());
    }
}


void OcrOcradDialog::getVersion(const QString &bin)
{
    //qDebug() << "of" << bin;
    if (bin.isEmpty()) {
        return;
    }

    KProcess proc;
    proc.setOutputChannelMode(KProcess::MergedChannels);
    proc << bin << "-V";

    int status = proc.execute(5000);
    if (status == 0) {
        QByteArray output = proc.readAllStandardOutput();
        QRegExp rx("GNU Ocrad (version )?([\\d\\.]+)");
        if (rx.indexIn(output) > -1) {
            m_ocrCmd = bin;
            m_versionStr = rx.cap(2);
            m_versionNum = m_versionStr.mid(2).toInt();
            //qDebug() << "version" << m_versionStr << "=" << m_versionNum;
        }
    } else {
        //qDebug() << "failed with status" << status;
        m_versionStr = i18n("Error");
    }
}

QStringList OcrOcradDialog::getValidValues(const QString &opt)
{
    QStringList result;

    KConfigSkeletonItem *ski = KookaSettings::self()->ocrOcradValidValuesItem();
    Q_ASSERT(ski!=nullptr);
    QString groupName = QString("%1_v%2").arg(ski->group()).arg(m_versionStr);
    KConfigGroup grp = KookaSettings::self()->config()->group(groupName);

    if (grp.hasKey(opt)) {				// values in config already
        //qDebug() << "option" << opt << "already in config";
        result = grp.readEntry(opt, QStringList());
    } else {						// not in config, need to extract
        if (!m_ocrCmd.isEmpty()) {
            KProcess proc;
            proc.setOutputChannelMode(KProcess::MergedChannels);
            proc << m_ocrCmd << QString("--%1=help").arg(opt);

            proc.execute(5000);
            // Ignore return status, because '--OPTION=help' returns exit code 1
            QByteArray output = proc.readAllStandardOutput();
            QRegExp rx("Valid .*are:([^\n]+)");
            if (rx.indexIn(output) > -1) {
                QString values = rx.cap(1);
                result = rx.cap(1).split(QRegExp("\\s+"), QString::SkipEmptyParts);
            } else {
                //qDebug() << "cannot get values, no match in" << output;
            }
        } else {
            //qDebug() << "cannot get values, no binary";
        }
    }

    //qDebug() << "values for" << opt << "=" << result.join(",");
    if (!result.isEmpty()) {
        grp.writeEntry(opt, result);			// save for next time
        grp.sync();
    }

    return (result);
}
