/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2020      Jonathan Marten <jjm@keelhaul.me.uk>	*
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

#include "ocrtesseractdialog.h"

#include <qlabel.h>
#include <qregexp.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qprogressbar.h>

#include <klocalizedstring.h>
#include <kurlrequester.h>
#include <kprocess.h>

#include "kookapref.h"
#include "kookasettings.h"
#include "dialogbase.h"

#include "ocrtesseractengine.h"
#include "ocr_logging.h"


OcrTesseractDialog::OcrTesseractDialog(AbstractOcrEngine *plugin, QWidget *pnt)
    : AbstractOcrDialogue(plugin, pnt),
      m_setupWidget(nullptr),
      m_ocrCmd(QString()),
      m_versionNum(0),
      m_versionStr(QString())
{
}


bool OcrTesseractDialog::setupGui()
{
    AbstractOcrDialogue::setupGui();			// build the standard GUI

    // Options available vary with the Tesseract version.  So we need to find
    // the Tesseract binary and get its version before creating the GUI.
    m_ocrCmd = engine()->findExecutable(&KookaSettings::ocrTesseractBinary, KookaSettings::self()->ocrTesseractBinaryItem());

    if (!m_ocrCmd.isEmpty()) getVersion(m_ocrCmd);	// found, get its version
    else						// not found or invalid
    {
        engine()->setErrorText(i18n("The Tesseract executable is not configured or is not available."));
    }

    QWidget *w = addExtraSetupWidget();
    QGridLayout *gl = new QGridLayout(w);
    int row = 0;

    // Language, auto detected values
    QMap<QString,QString> vals = getValidValues("list-langs");
    KConfigSkeletonItem *ski = KookaSettings::self()->ocrTesseractLanguageItem();
    Q_ASSERT(ski!=nullptr);
    QLabel *l = new QLabel(ski->label(), w);
    gl->addWidget(l, row, 0);
    m_language = new QComboBox(w);
    m_language->setToolTip(ski->toolTip());
    m_language->addItem(i18n("(default)"), QString());
    for (QMap<QString,QString>::const_iterator it = vals.constBegin(); it!=vals.constEnd(); ++it)
    {
        m_language->addItem((!it.value().isEmpty() ? it.value() : it.key()), it.key());
    }
 
    if (vals.isEmpty()) m_language->setEnabled(false);
    else
    {
        int ix = m_language->findData(KookaSettings::ocrTesseractLanguage());
        if (ix!=-1) m_language->setCurrentIndex(ix);
    }

    gl->addWidget(m_language, row, 1);
    l->setBuddy(m_language);
    ++row;

    // User words, from file
    ski = KookaSettings::self()->ocrTesseractUserWordsItem();
    Q_ASSERT(ski!=nullptr);
    l = new QLabel(ski->label(), w);
    gl->addWidget(l, row, 0);
    m_userWords = new KUrlRequester(w);
    m_userWords->setAcceptMode(QFileDialog::AcceptOpen);
    m_userWords->setMode(KFile::File|KFile::ExistingOnly|KFile::LocalOnly);
    m_userWords->setPlaceholderText(i18n("Select a file if required..."));

    QUrl u = KookaSettings::ocrTesseractUserWords();
    if (u.isValid()) m_userWords->setUrl(u);

    gl->addWidget(m_userWords, row, 1);
    l->setBuddy(m_userWords);
    ++row;

    // User patterns, from file
    ski = KookaSettings::self()->ocrTesseractUserPatternsItem();
    Q_ASSERT(ski!=nullptr);
    l = new QLabel(ski->label(), w);
    gl->addWidget(l, row, 0);
    m_userPatterns = new KUrlRequester(w);
    m_userPatterns->setAcceptMode(QFileDialog::AcceptOpen);
    m_userPatterns->setMode(KFile::File|KFile::ExistingOnly|KFile::LocalOnly);
    m_userPatterns->setPlaceholderText(i18n("Select a file if required..."));

    u = KookaSettings::ocrTesseractUserPatterns();
    if (u.isValid()) m_userPatterns->setUrl(u);

    gl->addWidget(m_userPatterns, row, 1);
    l->setBuddy(m_userPatterns);
    ++row;

    gl->setRowMinimumHeight(row, DialogBase::verticalSpacing());
    ++row;

    // Page segmentation mode, auto detected values
    vals = getValidValues("help-psm");
    ski = KookaSettings::self()->ocrTesseractSegmentationModeItem();
    Q_ASSERT(ski!=nullptr);
    l = new QLabel(ski->label(), w);
    gl->addWidget(l, row, 0);
    m_segmentationMode = new QComboBox(w);
    m_segmentationMode->setToolTip(ski->toolTip());
    m_segmentationMode->addItem(i18n("(default)"), QString());
    for (QMap<QString,QString>::const_iterator it = vals.constBegin(); it != vals.constEnd(); ++it)
    {
        m_segmentationMode->addItem(it.value(), it.key());
    }
 
    if (vals.isEmpty()) m_segmentationMode->setEnabled(false);
    else
    {
        int ix = m_segmentationMode->findData(KookaSettings::ocrTesseractSegmentationMode());
        if (ix!=-1) m_segmentationMode->setCurrentIndex(ix);
    }

    gl->addWidget(m_segmentationMode, row, 1);
    l->setBuddy(m_segmentationMode);
    ++row;

    // OCR engine mode, auto detected values
    vals = getValidValues("help-oem");
    ski = KookaSettings::self()->ocrTesseractEngineModeItem();
    Q_ASSERT(ski!=nullptr);
    l = new QLabel(ski->label(), w);
    gl->addWidget(l, row, 0);
    m_engineMode = new QComboBox(w);
    m_engineMode->setToolTip(ski->toolTip());
    m_engineMode->addItem(i18n("(default)"), QString());
    for (QMap<QString,QString>::const_iterator it = vals.constBegin(); it != vals.constEnd(); ++it)
    {
        m_engineMode->addItem(it.value(), it.key());
    }
 
    if (vals.isEmpty()) m_engineMode->setEnabled(false);
    else
    {
        int ix = m_engineMode->findData(KookaSettings::ocrTesseractEngineMode());
        if (ix!=-1) m_engineMode->setCurrentIndex(ix);
    }

    gl->addWidget(m_engineMode, row, 1);
    l->setBuddy(m_engineMode);
    ++row;

    gl->setRowMinimumHeight(row, DialogBase::verticalSpacing());
    ++row;

    gl->setRowStretch(row-1, 1);			// for top alignment
    gl->setColumnStretch(1, 1);

    ocrShowInfo(m_ocrCmd, m_versionStr);		// show the binary and version
    progressBar()->setRange(0, 0);			// progress animation only

    m_setupWidget = w;
    return (!m_ocrCmd.isEmpty());
}


void OcrTesseractDialog::slotWriteConfig()
{
    AbstractOcrDialogue::slotWriteConfig();

    KookaSettings::setOcrTesseractBinary(getOCRCmd());

    KookaSettings::setOcrTesseractLanguage(m_language->currentData().toString());
    KookaSettings::setOcrTesseractUserWords(m_userWords->url());
    KookaSettings::setOcrTesseractUserPatterns(m_userPatterns->url());
    KookaSettings::setOcrTesseractSegmentationMode(m_segmentationMode->currentData().toString());
    KookaSettings::setOcrTesseractEngineMode(m_engineMode->currentData().toString());
}


void OcrTesseractDialog::enableFields(bool enable)
{
    m_setupWidget->setEnabled(enable);
}


void OcrTesseractDialog::getVersion(const QString &bin)
{
    qCDebug(OCR_LOG) << "of" << bin;
    if (bin.isEmpty()) return;

    KProcess proc;
    proc.setOutputChannelMode(KProcess::MergedChannels);
    proc << bin << "-v";

    int status = proc.execute(5000);
    if (status==0)
    {
        QByteArray output = proc.readAllStandardOutput();
        QRegExp rx("tesseract ([\\d\\.]+)");
        if (rx.indexIn(output)>-1)
        {
            m_ocrCmd = bin;
            m_versionStr = rx.cap(1);
            m_versionNum = m_versionStr.left(1).toInt();
            qCDebug(OCR_LOG) << "version" << m_versionStr << "=" << m_versionNum;
        }
    }
    else
    {
        qCDebug(OCR_LOG) << "failed with status" << status;
        m_versionStr = i18n("Error");
    }
}


QMap<QString,QString> OcrTesseractDialog::getValidValues(const QString &opt)
{
    // The values displayed by and passed to Tesseract for OEM and PSM are integers,
    // but the values for the language are strings.  For simplicity we treat the
    // OEM/PSM settings as strings also.
    QMap<QString,QString> result;

    KConfigSkeletonItem *ski = KookaSettings::self()->ocrTesseractValidValuesItem();
    Q_ASSERT(ski!=nullptr);
    QString groupName = QString("%1_v%2").arg(ski->group()).arg(m_versionStr);
    KConfigGroup grp = KookaSettings::self()->config()->group(groupName);

    if (grp.hasKey(opt+"_keys"))			// values in config already
    {
        qCDebug(OCR_LOG) << "option" << opt << "already in config";

        const QStringList keys = grp.readEntry(opt+"_keys", QStringList());
        const QStringList descs = grp.readEntry(opt+"_descs", QStringList());
        for (int i = 0; i<keys.count(); ++i) result.insert(keys.at(i), descs.at(i));
    }
    else						// not in config, need to extract
    {
        if (m_ocrCmd.isEmpty())
        {
            qCWarning(OCR_LOG) << "cannot get values, no binary";
            return (result);
        }

        KProcess proc;
        proc.setOutputChannelMode(KProcess::MergedChannels);
        proc << m_ocrCmd << QString("--%1").arg(opt);

        proc.execute(5000);
        const QByteArray output = proc.readAllStandardOutput();
        const QList<QByteArray> lines = output.split('\n');
        for (const QByteArray &line : lines)
        {
            const QString lineStr = QString::fromLocal8Bit(line);
            qCDebug(OCR_LOG) << "line:" << lineStr;

            QRegExp rx;
            if (opt=="list-langs") rx.setPattern("^\\s*(\\w+)()$");
            else rx.setPattern("^\\s*(\\d+)\\s+(\\w.+)?$");
            if (rx.indexIn(lineStr)>-1)
            {
                const QString value = rx.cap(1);
                QString desc = rx.cap(2).simplified();
                if (desc.endsWith(QLatin1Char('.')) || desc.endsWith(QLatin1Char(','))) desc.chop(1);
                result.insert(value, desc);
            }
        }

        qCDebug(OCR_LOG) << "parsed result count" << result.count();
        if (!result.isEmpty())
        {						   	// save result for next time
            grp.writeEntry(opt+"_keys", result.keys());	   	// ordered list of keys
            grp.writeEntry(opt+"_descs", result.values());	// same-ordered list of values
            grp.sync();
        }
    }

    qCDebug(OCR_LOG) << "values for" << opt << "=" << result.keys();
    return (result);
}
