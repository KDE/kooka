/***************************************************************************
                          kocrstartdia.cpp  -  description
                             -------------------
    begin                : Fri Now 10 2000
    copyright            : (C) 2000 by Klaas Freitag
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

#include "ocrkadmosdialog.h"

#include <qlayout.h>
#include <qlabel.h>
#include <qfileinfo.h>
#include <qtooltip.h>
#include <qdir.h>
#include <qmap.h>
//#include <q3buttongroup.h>
#include <qradiobutton.h>
#include <qcombobox.h>
//#include <q3groupbox.h>
#include <qcheckbox.h>
#include <qstringlist.h>

#include <kconfig.h>
#include <kglobal.h>
#include <QDebug>
#include <KLocalizedString>
#include <kseparator.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "ocrkadmosengine.h"

/* defines for konfig-reading */
#define CFG_GROUP_KADMOS "Kadmos"
#define CFG_KADMOS_CLASSIFIER_PATH "classifierPath"
#define CFG_KADMOS_CLASSIFIER      "classifier"

#define CNTRY_CZ i18n( "Czech Republic, Slovakia")
#define CNTRY_GB i18n( "Great Britain, USA" )

KadmosDialog::KadmosDialog(QWidget *parent)
    : OcrBaseDialog(parent),
      m_cbNoise(0),
      m_cbAutoscale(0),
      m_haveNorm(false)
{
    //qDebug();
    // Layout-Boxes
    findClassifiers();
}

QString KadmosDialog::ocrEngineLogo() const
{
    return ("kadmoslogo.png");
}

QString KadmosDialog::ocrEngineName() const
{
    return (OcrEngine::engineName(OcrEngine::EngineKadmos));
}

QString KadmosDialog::ocrEngineDesc() const
{
    return (OcrKadmosEngine::engineDesc());
}

OcrEngine::EngineError KadmosDialog::findClassifiers()
{
    findClassifierPath();

    KLocale *locale = KLocale::global();
    const QStringList allCountries = locale->allCountriesList();
    for (QStringList::const_iterator it = allCountries.constBegin();
            it != allCountries.constEnd(); ++it) {
        m_longCountry2short[locale->countryCodeToName(*it)] = *it;
    }
    m_longCountry2short[i18n("European Countries")] = "eu";
    m_longCountry2short[CNTRY_CZ] = "cz";
    m_longCountry2short[CNTRY_GB] = "us";

    QStringList lst;

    /* custom Path */
    if (! m_customClassifierPath.isEmpty()) {
        QDir dir(m_customClassifierPath);

        QStringList lst1 = dir.entryList(QStringList("ttf*.rec"));

        for (QStringList::Iterator it = lst1.begin(); it != lst1.end(); ++it) {
            lst << m_customClassifierPath + *it;
        }

        lst1 = dir.entryList(QStringList("hand*.rec"));

        for (QStringList::Iterator it = lst1.begin(); it != lst1.end(); ++it) {
            lst << m_customClassifierPath + *it;
        }

        lst1 = dir.entryList(QStringList("norm*.rec"));

        for (QStringList::Iterator it = lst1.begin(); it != lst1.end(); ++it) {
            lst << m_customClassifierPath + *it;
        }
    } else {
        /* standard location */
        KStandardDirs stdDir;
        //qDebug() << "Starting to read resources";

        lst = stdDir.findAllResources("data",
                                      "kooka/classifiers/*.rec",
                                      KStandardDirs::Recursive | KStandardDirs::NoDuplicates);
    }

    /* no go through lst and sort out hand-, ttf- and norm classifier */
    for (QStringList::Iterator it = lst.begin(); it != lst.end(); ++it) {
        //qDebug() << "Checking file:" << (*it);
        QFileInfo fi(*it);
        QString name = fi.fileName().toLower();

        if (name.startsWith("ttf")) {
            QString lang = name.mid(3, 2);
            if (allCountries.contains(lang)) {
                QString lngCountry = locale->countryCodeToName(lang);
                if (lngCountry.isEmpty()) {
                    lngCountry = name;
                }
                m_ttfClassifier << lngCountry;
                //qDebug() << "TTF: Insert country" << lngCountry;
            } else if (lang == "cz") {
                m_ttfClassifier << CNTRY_CZ;
            } else if (lang == "us") {
                m_ttfClassifier << CNTRY_GB;
            } else {
                m_ttfClassifier << name;
                //qDebug() << "TTF: Unknown country";
            }
        } else if (name.startsWith("hand")) {
            QString lang = name.mid(4, 2);
            if (allCountries.contains(lang)) {
                QString lngCountry = locale->countryCodeToName(lang);
                if (lngCountry.isEmpty()) {
                    lngCountry = name;
                }
                m_handClassifier << lngCountry;
            } else if (lang == "cz") {
                m_handClassifier << i18n("Czech Republic, Slovakia");
            } else if (lang == "us") {
                m_handClassifier << i18n("Great Britain, USA");
            } else {
                //qDebug() << "HAND: Unknown country" << lang;
                m_handClassifier << name;
            }
        } else if (name.startsWith("norm")) {
            m_haveNorm = true;
        }

        //qDebug() << "Found classifier:" << (*it);
        m_classifierPath << *it;
    }

    if (m_handClassifier.count() + m_ttfClassifier.count() > 0) {
        /* There are classifiers */
        return OcrEngine::ENG_OK;
    } else {
        /* Classifier are missing */
        return OcrEngine::ENG_DATA_MISSING;
    }
}

OcrEngine::EngineError KadmosDialog::findClassifierPath()
{
    KStandardDirs stdDir;
    OcrEngine::EngineError err = OcrEngine::ENG_OK;

    const KConfigGroup grp = KSharedConfig::openConfig()->group(CFG_GROUP_KADMOS);
    m_customClassifierPath = grp.readPathEntry(CFG_KADMOS_CLASSIFIER_PATH, "");
#if 0
    if (m_customClassifierPath == "NotFound") {
        /* Wants the classifiers from the standard kde paths */
        KMessageBox::error(0, i18n("The classifier files for KADMOS could not be found.\n"
                                   "OCR with KADMOS will not be possible.\n\n"
                                   "Change the OCR engine in the preferences dialog."),
                           i18n("Installation Error"));
    } else {
        m_classifierPath = customPath;
    }
#endif
    return err;

}

OcrEngine::EngineError KadmosDialog::setupGui()
{

    OcrEngine::EngineError err = OcrBaseDialog::setupGui();

    // setupPreprocessing( addVBoxPage(   i18n("Preprocessing")));
    // setupSegmentation(  addVBoxPage(   i18n("Segmentation")));
    // setupClassification( addVBoxPage( i18n("Classification")));

    /* continue page setup on the first page */
    QWidget *page = new QWidget(this);
    QVBoxLayout *pageVBoxLayout = new QVBoxLayout(page);
    pageVBoxLayout->setMargin(0);

    // FIXME: dynamic classifier reading.

    new QLabel(i18n("Please classify the font type and language of the text on the image:"), page);
    QWidget *locBox = new QWidget(page);
    QHBoxLayout *locBoxHBoxLayout = new QHBoxLayout(locBox);
    locBoxHBoxLayout->setMargin(0);
    pageVBoxLayout->addWidget(locBox);
    m_bbFont = new Q3ButtonGroup(1, Qt::Horizontal, i18n("Font Type Selection"), locBox);
    locBoxHBoxLayout->addWidget(m_bbFont);

    m_rbMachine = new QRadioButton(i18n("Machine print"), m_bbFont);
    m_rbHand    = new QRadioButton(i18n("Hand writing"),  m_bbFont);
    m_rbNorm    = new QRadioButton(i18n("Norm font"),     m_bbFont);

    m_gbLang = new Q3GroupBox(1, Qt::Horizontal, i18n("Country"), locBox);
    locBoxHBoxLayout->addWidget(m_gbLang);

    m_cbLang = new QComboBox(m_gbLang);
    m_cbLang->setItemText(m_cbLang->currentIndex(), KLocale::defaultCountry());

    connect(m_bbFont, &Q3ButtonGroup::clicked, this, &KadmosDialog::slFontChanged);
    m_rbMachine->setChecked(true);

    /* --- */
    QWidget *innerBox = new QWidget(page);
    QHBoxLayout *innerBoxHBoxLayout = new QHBoxLayout(innerBox);
    innerBoxHBoxLayout->setMargin(0);
    pageVBoxLayout->addWidget(innerBox);
    innerBoxHBoxLayout->setSpacing(KDialog::spacingHint());

    Q3ButtonGroup *cbGroup = new Q3ButtonGroup(1, Qt::Horizontal, i18n("OCR Modifier"), innerBox);
    innerBoxHBoxLayout->addWidget(cbGroup);
    Q_CHECK_PTR(cbGroup);

    m_cbNoise = new QCheckBox(i18n("Enable automatic noise reduction"), cbGroup);
    m_cbAutoscale = new QCheckBox(i18n("Enable automatic scaling"), cbGroup);

    addExtraSetupWidget(page);

    if (err != OcrEngine::ENG_OK) {
        enableFields(false);
        enableButton(User1, false);
    }

    if (m_ttfClassifier.count() == 0) {
        m_rbMachine->setEnabled(false);
    }
    if (m_handClassifier.count() == 0) {
        m_rbHand->setEnabled(false);
    }
    if (!m_haveNorm) {
        m_rbNorm->setEnabled(false);
    }

    if ((m_ttfClassifier.count() + m_handClassifier.count()) == 0 && ! m_haveNorm) {
        KMessageBox::error(0, i18n("The classifier files for KADMOS could not be found.\n"
                                   "OCR with KADMOS will not be possible.\n\n"
                                   "Change the OCR engine in the preferences dialog."),
                           i18n("Installation Error"));
        err = OcrEngine::ENG_BAD_SETUP;
    } else {
        slotFontChanged(0);    // Load machine print font language list
    }

    ocrShowInfo(QString());
    return err;
}

void KadmosDialog::slotFontChanged(int id)
{
    m_cbLang->clear();

    const KConfigGroup grp = KSharedConfig::openConfig()->group(CFG_GROUP_KADMOS);
    m_customClassifierPath = grp.readPathEntry(CFG_KADMOS_CLASSIFIER_PATH, "");

    bool enable = true;

    if (id == 0) { /* Machine Print */
        m_cbLang->addItems(m_ttfClassifier);
    } else if (id == 1) { /* Hand Writing */
        m_cbLang->addItems(m_handClassifier);
    } else if (id == 2) { /* Norm Font */
        enable = false;
    }
    m_cbLang->setEnabled(enable);
}

void KadmosDialog::setupPreprocessing(QWidget *box)
{
}

void KadmosDialog::setupSegmentation(QWidget *box)
{
}

void KadmosDialog::setupClassification(QWidget *box)
{
}

/*
 * returns the complete path of the classifier selected in the
 * GUI in the parameter path. The result value indicates if there
 * was one found.
 */

bool KadmosDialog::getSelClassifier(QString &path) const
{
    QString classifier = getSelClassifierName();

    QString cmplPath;
    /*
     * Search the complete path for the classifier file name
     * returned from the getSelClassifierName method
     */
    for (QStringList::ConstIterator it = m_classifierPath.begin();
            it != m_classifierPath.end(); ++it) {
        QFileInfo fi(*it);
        if (fi.fileName() == classifier) {
            cmplPath = *it;
            break;
        }
    }

    bool res = true;

    if (cmplPath.isEmpty()) {
        /* hm, no path was found */
        //qDebug() << "Error: The entire path is empty";
        res = false;
    } else {
        /* Check if the classifier exists on the HD. If not, return an empty string */
        QFileInfo fi(cmplPath);

        if (res && ! fi.exists()) {
            //qDebug() << "Classifier file does not exist";
            path = i18n("Classifier file %1 does not exist", classifier);
            res = false;
        }

        if (res && ! fi.isReadable()) {
            //qDebug() << "Classifier file could not be read";
            path = i18n("Classifier file %1 is not readable", classifier);
            res = false;
        }

        if (res) {
            path = cmplPath;
        }
    }
    return res;
}

QString KadmosDialog::getSelClassifierName() const
{
    QAbstractButton *butt = m_bbFont->selected();

    QString fType, rType;

    if (butt) {
        int fontTypeID = m_bbFont->id(butt);
        if (fontTypeID == 0) {
            fType = "ttf";
        } else if (fontTypeID == 1) {
            fType = "hand";
        } else if (fontTypeID == 2) {
            fType = "norm";
        } else {
            //qDebug() << "Error: Wrong font type ID";
        }
    }

    /* Get the long text from the combo box */
    QString selLang = m_cbLang->currentText();
    QString trans;
    if (fType != "norm" && m_longCountry2short.contains(selLang)) {
        QString langType = m_longCountry2short[selLang];
        trans = fType + langType + ".rec";
    } else {
        if (selLang.endsWith(".rec")) {
            /* can be a undetected */
            trans = selLang;
        } else if (fType == "norm") {
            trans = "norm.rec";
        } else {
            //qDebug() << "Error: Not a valid classifier";
        }
    }
    //qDebug() << "Returning" << trans;
    return (trans);
}

bool KadmosDialog::getAutoScale()
{
    return (m_cbAutoscale ? m_cbAutoscale->isChecked() : false);
}

bool KadmosDialog::getNoiseReduction()
{
    return (m_cbNoise ? m_cbNoise->isChecked() : false);

}

KadmosDialog::~KadmosDialog()
{

}

void KadmosDialog::slotWriteConfig()
{
    //qDebug();

    OcrBaseDialog::slotWriteConfig();

    //KConfig *conf = KSharedConfig::openConfig();
    // TODO: must be something to save here!
}

void KadmosDialog::enableFields(bool state)
{
    m_cbNoise->setEnabled(state);
    m_cbAutoscale->setEnabled(state);

    m_bbFont->setEnabled(state);
    m_gbLang->setEnabled(state);
}
