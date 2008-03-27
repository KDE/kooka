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

#include <kglobal.h>
#include <kdebug.h>
#include <klocale.h>
#include <kanimwidget.h>
#include <kseparator.h>
#include <kmessagebox.h>
#include <kurlrequester.h>
#include <kprocess.h>

#include <qlabel.h>
#include <qregexp.h>
#include <qvbox.h>
#include <qhbox.h>
#include <qcombobox.h>

#include "kookaimage.h"
#include "kookapref.h"

#include "ocrocradengine.h"

#include "ocrocraddialog.h"
#include "ocrocraddialog.moc"


OcrOcradDialog::OcrOcradDialog(QWidget *parent,KSpellConfig *spellConfig)
    : OcrBaseDialog(parent,spellConfig),
      m_ocrCmd(QString::null),
      m_orfUrlRequester(NULL),
      m_layoutMode(0),
      m_proc(NULL),
      m_version(0)
{
}


OcrOcradDialog::~OcrOcradDialog()
{
    if (m_proc!=NULL) delete m_proc;
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
    OcrBaseDialog::setupGui();

    QVBox *page = ocrPage();
    KConfig *conf = KGlobal::config();

    /* layout detection mode combo */
    conf->setGroup(CFG_GROUP_OCRAD);
    int layoutDetect = conf->readNumEntry(CFG_OCRAD_LAYOUT_DETECTION,0);

    new KSeparator(KSeparator::HLine,page);
    QHBox *hb1 = new QHBox(page);
    hb1->setSpacing(KDialog::spacingHint());

    new QLabel(i18n("Layout analysis mode:"),hb1);

    m_layoutMode = new QComboBox(hb1);
    m_layoutMode->insertItem(i18n("No Layout Detection"),0);
    m_layoutMode->insertItem(i18n("Column Detection"),1);
    m_layoutMode->insertItem(i18n("Full Layout Detection"),2);
    m_layoutMode->setCurrentItem(layoutDetect);

    /* find the OCRAD binary */
    conf->setGroup(CFG_GROUP_OCR_DIA);
    QString res = conf->readPathEntry(CFG_OCRAD_BINARY);
    if (res.isEmpty())
    {
        res = KookaPref::tryFindOcrad();
        if (res.isEmpty())
        {
            /* Popup here telling that the config needs to be called */
            KMessageBox::sorry(this,i18n("The path to the OCRAD binary is not configured or is not valid.\n"
                                         "Please enter or check the path in the Kooka configuration."),
                               i18n("OCRAD Software Not Found"));
            enableButton(KDialogBase::User1,false);
        }
    }

    /* retrieve program version and display */
    if (res.isEmpty()) res = i18n("Not found");
    else
    {
        m_ocrCmd = res;
        version(m_ocrCmd);				// start process to get version
    }
    ocrShowInfo(res);					// show binary, ready for version

    return (OcrEngine::ENG_OK);
}


void OcrOcradDialog::writeConfig()
{
    kdDebug(28000) << k_funcinfo << endl;

    OcrBaseDialog::writeConfig();

    KConfig *conf = KGlobal::config();
    conf->setGroup(CFG_GROUP_OCR_DIA);
    conf->writePathEntry(CFG_OCRAD_BINARY,QString(getOCRCmd()));

    conf->setGroup(CFG_GROUP_OCRAD);
    conf->writeEntry(CFG_OCRAD_LAYOUT_DETECTION,layoutDetectionMode());
}


void OcrOcradDialog::enableFields(bool enable)
{
    m_layoutMode->setEnabled(enable);
}


int OcrOcradDialog::layoutDetectionMode() const
{
    return (m_layoutMode->currentItem());
}


/* Later: Allow interactive loading of orf files
 *  for now, return emty string
 */
QString OcrOcradDialog::orfUrl() const
{
    if( m_orfUrlRequester )
	return m_orfUrlRequester->url();
    else
	return QString();
}


void OcrOcradDialog::version(const QString &exe)
{
    if (m_proc!=NULL) delete m_proc;

    m_proc = new KProcess;

    kdDebug(28000) << k_funcinfo << "version of " << exe << endl;
    *m_proc << exe;
    *m_proc << "-V";

    connect(m_proc,SIGNAL(receivedStdout(KProcess *,char *,int)),
            SLOT(slReceiveStdIn(KProcess *,char *,int)));

    m_proc->start(KProcess::NotifyOnExit,KProcess::Stdout);
}


/*
 * returns the numeric version of the ocrad program. It is queried in the slot
 * slReceiveStdIn, which parses the output of the ocrad -V call.
 *
 * Attention: This method returns 10 for ocrad v. 0.10 and 8 for ocrad-0.8
 */
void OcrOcradDialog::slReceiveStdIn(KProcess *proc,char *buffer,int buflen)
{
    QString vstr = QString::fromUtf8(buffer,buflen);

    QRegExp rx("GNU Ocrad version ([\\d\\.]+)");
    if (rx.search(vstr)>-1)
    {
        QString vStr = rx.cap(1);
        ocrShowVersion(vStr);

        vStr.remove(0,2);
        m_version = vStr.toInt();
    }
}


