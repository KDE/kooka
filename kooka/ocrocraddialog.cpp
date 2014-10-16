/***************************************************************************
                          kocrocrad.cpp  - ocrad dialog
                             -------------------
    begin                : Tue Jul 15 2003
    copyright            : (C) 2003 by Klaas Freitag
    email                : freitag@suse.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This file may be distributed and/or modified under the terms of the    *
 *  GNU General Public License version 2 as published by the Free Software *
 *  Foundation and appearing in the file COPYING included in the           *
 *  packaging of this file.                                                *
 *
 *  As a special exception, permission is given to link this program       *
 *  with any version of the KADMOS ocr/icr engine of reRecognition GmbH,   *
 *  Kreuzlingen and distribute the resulting executable without            *
 *  including the source code for KADMOS in the source distribution.       *
 *
 *  As a special exception, permission is given to link this program       *
 *  with any edition of Qt, and distribute the resulting executable,       *
 *  without including the source code for Qt in the source distribution.   *
 *                                                                         *
 ***************************************************************************/

#include "ocrocraddialog.h"

#include <qlabel.h>
#include <qregexp.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qspinbox.h>
#include <qlayout.h>
#include <qprogressbar.h>

#include <kglobal.h>
#include <QDebug>
#include <KLocalizedString>
#include <kanimatedbutton.h>
#include <kseparator.h>
#include <kmessagebox.h>
#include <kurlrequester.h>
#include <kprocess.h>
#include <kvbox.h>
#include <khbox.h>
#include <KConfigGroup>
#include <KDialog>
#include "kookaimage.h"
#include "kookapref.h"
#include "kscancontrols.h"

#include "ocrocradengine.h"

OcrOcradDialog::OcrOcradDialog(QWidget *parent)
    : OcrBaseDialog(parent),
      m_setupWidget(NULL),
      m_orfUrlRequester(NULL),
      m_layoutMode(0),
      m_ocrCmd(QString::null),
      m_versionNum(0),
      m_versionStr(QString::null)
{
}

OcrOcradDialog::~OcrOcradDialog()
{
}

QString OcrOcradDialog::ocrEngineLogo() const
{
    return ("ocrad.png");
}

QString OcrOcradDialog::ocrEngineName() const
{
    return (OcrEngine::engineName(OcrEngine::EngineOcrad));
}

QString OcrOcradDialog::ocrEngineDesc() const
{
    return (OcrOcradEngine::engineDesc());
}

OcrEngine::EngineError OcrOcradDialog::setupGui()
{
    // Options available vary with the OCRAD version.  So we need to find
    // the OCRAD binary and get its version before creating the GUI.

    // No need to read the config here, KookaPref::tryFindBinary() does that
    //KConfigGroup grp2 = KSharedConfig::openConfig()->group(CFG_GROUP_OCR_DIA);
    //QString res = grp2.readPathEntry(CFG_OCRAD_BINARY, "");

    QString res = KookaPref::tryFindOcrad();        // read config or search
    if (res.isEmpty()) {                // not found or invalid
        KMessageBox::sorry(this, i18n("The path to the OCRAD binary is not configured or is not valid.\n"
                                      "Please enter or check the path in the Kooka configuration."),
                           i18n("OCRAD Binary Not Found"));
        //QT5 enableButton(KDialog::User1, false);
    } else {
        getVersion(res);
    }

    OcrBaseDialog::setupGui();              // now can build the GUI

    QWidget *w = addExtraSetupWidget();
    QGridLayout *gl = new QGridLayout(w);
    KConfigGroup grp = KSharedConfig::openConfig()->group(CFG_GROUP_OCRAD);

    // Layout detection mode, dependent on OCRAD version
    QLabel *l = new QLabel(i18n("Layout analysis mode:"), w);
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

    m_layoutMode->setCurrentIndex(grp.readEntry(CFG_OCRAD_LAYOUT_DETECTION, 0));
    gl->addWidget(m_layoutMode, 0, 1);
    l->setBuddy(m_layoutMode);

    gl->setRowMinimumHeight(1, KDialog::spacingHint());

    // Character set, auto detected values
    QStringList vals = getValidValues("charset");

    l = new QLabel(i18n("Character set:"), w);
    gl->addWidget(l, 2, 0);
    m_characterSet = new QComboBox(w);
    m_characterSet->addItem(i18n("(default)"), false);
    for (QStringList::const_iterator it = vals.constBegin(); it != vals.constEnd(); ++it) {
        m_characterSet->addItem(*it, true);
    }

    if (vals.count() == 0) {
        m_characterSet->setEnabled(false);
    } else {
        int ix = m_characterSet->findText(grp.readEntry(CFG_OCRAD_CHARSET, ""));
        if (ix != -1) {
            m_characterSet->setCurrentIndex(ix);
        }
    }
    gl->addWidget(m_characterSet, 2, 1);
    l->setBuddy(m_characterSet);

    // Filter, auto detected values
    vals = getValidValues("filter");

    l = new QLabel(i18n("Filter:"), w);
    gl->addWidget(l, 3, 0);
    m_filter = new QComboBox(w);
    m_filter->addItem(i18n("(default)"), false);
    for (QStringList::const_iterator it = vals.constBegin(); it != vals.constEnd(); ++it) {
        m_filter->addItem(*it, true);
    }

    if (vals.count() == 0) {
        m_filter->setEnabled(false);
    } else {
        int ix = m_filter->findText(grp.readEntry(CFG_OCRAD_FILTER, ""));
        if (ix != -1) {
            m_filter->setCurrentIndex(ix);
        }
    }
    gl->addWidget(m_filter, 3, 1);
    l->setBuddy(m_filter);

    // Transform, auto detected values
    vals = getValidValues("transform");

    l = new QLabel(i18n("Transform:"), w);
    gl->addWidget(l, 4, 0);
    m_transform = new QComboBox(w);
    m_transform->addItem(i18n("(default)"), false);
    for (QStringList::const_iterator it = vals.constBegin(); it != vals.constEnd(); ++it) {
        m_transform->addItem(*it, true);
    }

    if (vals.count() == 0) {
        m_transform->setEnabled(false);
    } else {
        int ix = m_transform->findText(grp.readEntry(CFG_OCRAD_TRANSFORM, ""));
        if (ix != -1) {
            m_transform->setCurrentIndex(ix);
        }
    }
    gl->addWidget(m_transform, 4, 1);
    l->setBuddy(m_transform);

    gl->setRowMinimumHeight(5, KDialog::spacingHint());

    // Invert option, on/off
    m_invert = new QCheckBox(i18n("Invert input"), w);
    m_invert->setChecked(grp.readEntry(CFG_OCRAD_INVERT, false));
    gl->addWidget(m_invert, 6, 1, Qt::AlignLeft);

    gl->setRowMinimumHeight(7, KDialog::spacingHint());

    // Threshold, on/off and slider
    m_thresholdEnable = new QCheckBox(i18n("Set threshold"), w);
    m_thresholdEnable->setChecked(grp.readEntry(CFG_OCRAD_THRESHOLD_ENABLE, false));
    gl->addWidget(m_thresholdEnable, 8, 1, Qt::AlignLeft);

    m_thresholdSlider = new KScanSlider(w, i18n("Threshold"), 0, 100);
    m_thresholdSlider->setValue(grp.readEntry(CFG_OCRAD_THRESHOLD_VALUE, 50));
    gl->addWidget(m_thresholdSlider, 9, 1);
    m_thresholdSlider->spinBox()->setSuffix("%");

    l = new QLabel(m_thresholdSlider->label(), w);
    gl->addWidget(l, 9, 0);
    l->setBuddy(m_thresholdSlider);

    connect(m_thresholdEnable, &QCheckBox::toggled, m_thresholdSlider, &KScanSlider::setEnabled);
    m_thresholdSlider->setEnabled(m_thresholdEnable->isChecked());

    gl->setRowStretch(10, 1);               // for top alignment
    gl->setColumnStretch(1, 1);

    ocrShowInfo((!m_ocrCmd.isEmpty() ? m_ocrCmd : i18n("Not found")),
                (!m_versionStr.isEmpty() ? m_versionStr : i18n("Unknown")));
    // show binary and version
    progressBar()->setMaximum(0);           // animation only

    m_setupWidget = w;
    return (OcrEngine::ENG_OK);
}

void OcrOcradDialog::slotWriteConfig()
{
    //qDebug();

    OcrBaseDialog::slotWriteConfig();

    KConfigGroup grp1 = KSharedConfig::openConfig()->group(CFG_GROUP_OCR_DIA);
    grp1.writePathEntry(CFG_OCRAD_BINARY, getOCRCmd());

    KConfigGroup grp2 = KSharedConfig::openConfig()->group(CFG_GROUP_OCRAD);
    grp2.writeEntry(CFG_OCRAD_LAYOUT_DETECTION, m_layoutMode->currentIndex());

    int ix = m_characterSet->currentIndex();
    if (m_characterSet->itemData(ix).toBool()) {
        grp2.writeEntry(CFG_OCRAD_CHARSET, m_characterSet->currentText());
    } else {
        grp2.deleteEntry(CFG_OCRAD_CHARSET);
    }

    ix = m_filter->currentIndex();
    if (m_filter->itemData(ix).toBool()) {
        grp2.writeEntry(CFG_OCRAD_FILTER, m_filter->currentText());
    } else {
        grp2.deleteEntry(CFG_OCRAD_FILTER);
    }

    ix = m_transform->currentIndex();
    if (m_transform->itemData(ix).toBool()) {
        grp2.writeEntry(CFG_OCRAD_TRANSFORM, m_transform->currentText());
    } else {
        grp2.deleteEntry(CFG_OCRAD_TRANSFORM);
    }

    grp2.writeEntry(CFG_OCRAD_INVERT, m_invert->isChecked());

    grp2.writeEntry(CFG_OCRAD_THRESHOLD_ENABLE, m_thresholdEnable->isChecked());
    grp2.writeEntry(CFG_OCRAD_THRESHOLD_VALUE, m_thresholdSlider->value());
}

void OcrOcradDialog::enableFields(bool enable)
{
    m_setupWidget->setEnabled(enable);
}

/* Later: Allow interactive loading of orf files
 *  for now, return emty string
 */
QString OcrOcradDialog::orfUrl() const
{
    if (m_orfUrlRequester != NULL) {
        return (m_orfUrlRequester->url().url());
    } else {
        return (QString::null);
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

    QString groupName = QString("%1_v%2").arg(CFG_GROUP_OCRAD).arg(m_versionStr);
    KConfigGroup grp = KSharedConfig::openConfig()->group(groupName);

    if (grp.hasKey(opt)) {              // values in config already
        //qDebug() << "option" << opt << "already in config";
        result = grp.readEntry(opt, QStringList());
    } else {                    // not in config, need to extract
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
        grp.writeEntry(opt, result);    // save for next time
    }

    return (result);
}
