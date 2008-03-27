/***************************************************************************
                          kocrgocr.cpp  - GOCR ocr dialog
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

#include <kconfig.h>
#include <kglobal.h>
#include <kdebug.h>
#include <klocale.h>
#include <kseparator.h>
#include <kmessagebox.h>
#include <kprocess.h>

#include <qvbox.h>
#include <qlabel.h>
#include <qtooltip.h>
#include <qregexp.h>
#include <qgrid.h>

#include "kookaimage.h"
#include "kookapref.h"

#include "ocrgocrengine.h"

#include "ocrgocrdialog.h"
#include "ocrgocrdialog.moc"

/* defines for konfig-reading */

#define CFG_GROUP_GOCR "gocr"
#define CFG_GOCR_DUSTSIZE "gocrDustSize"
#define CFG_GOCR_GRAYLEVEL "gocrGrayLevel"
#define CFG_GOCR_SPACEWIDTH "gocrSpaceWidth"


OcrGocrDialog::OcrGocrDialog( QWidget *parent,KSpellConfig *spellConfig)
    : OcrBaseDialog(parent,spellConfig),
      m_proc(NULL),
      m_ocrCmd(QString::null),
      m_isBW(false)
{
}


OcrGocrDialog::~OcrGocrDialog()
{
    if (m_proc!=NULL) delete m_proc;
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

    QVBox *page = ocrPage();
    KConfig *conf = KGlobal::config();
    conf->setGroup(CFG_GROUP_GOCR);

    new KSeparator(KSeparator::HLine,page);

    /* Sliders for OCR-Options */
    QGrid *g = new QGrid(2,Qt::Horizontal,page);
    g->setSpacing(KDialog::spacingHint());

    QLabel *l = new QLabel(i18n("Gray level:"),g);
    sliderGrayLevel = new KScanSlider( g , QString::null, 0, 254, true, 160 );
    int numdefault = conf->readNumEntry( CFG_GOCR_GRAYLEVEL, 160 );
    sliderGrayLevel->slSetSlider( numdefault );
    QToolTip::add( sliderGrayLevel,
                   i18n( "The threshold value below which gray pixels are\nconsidered to be black.\n\nThe default is 160."));
    l->setBuddy(sliderGrayLevel);

    l = new QLabel(i18n("Dust size:"),g);
    sliderDustSize = new KScanSlider( g, QString::null, 0, 60, true, 10 );
    numdefault = conf->readNumEntry( CFG_GOCR_DUSTSIZE, 10 );
    sliderDustSize->slSetSlider( numdefault );
    QToolTip::add( sliderDustSize,
                   i18n( "Clusters smaller than this value\nwill be considered to be dust, and\nremoved from the image.\n\nThe default is 10."));
    l->setBuddy(sliderDustSize);

    l = new QLabel(i18n("Space width:"),g);
    sliderSpace = new KScanSlider( g, QString::null, 0, 60, true, 0 );
    numdefault = conf->readNumEntry( CFG_GOCR_SPACEWIDTH, 0 );
    sliderSpace->slSetSlider( numdefault );
    QToolTip::add( sliderSpace, i18n("Spacing between characters.\n\nThe default is 0 which means autodetection."));
    l->setBuddy(sliderSpace);

    /* find the GOCR binary */
    conf->setGroup(CFG_GROUP_OCR_DIA);
    QString res = conf->readPathEntry(CFG_GOCR_BINARY);
    if (res.isEmpty())
    {
        res = KookaPref::tryFindGocr();
        if (res.isEmpty())
        {
            /* Popup here telling that the config needs to be called */
            KMessageBox::sorry(this,i18n("The path to the GOCR binary is not configured or is not valid.\n"
                                         "Please enter or check the path in the Kooka configuration."),
                               i18n("GOCR Software Not Found"));
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


void OcrGocrDialog::introduceImage(const KookaImage *img)
{
    if (img==NULL) return;
    OcrBaseDialog::introduceImage(img);

    m_isBW = true;
    if (img->numColors()>0 && img->numColors()<3)
    {
        kdDebug(28000) << k_funcinfo << "Have " << img->numColors() << " colors on depth " << img->depth() << endl;
        /* that means it is a black-and-white image. Thus we do not need the GrayLevel slider */
        m_isBW = false;
    }

    if (sliderGrayLevel!=NULL) sliderGrayLevel->setEnabled(m_isBW);
}


void OcrGocrDialog::writeConfig( void )
{
    kdDebug(28000) << k_funcinfo << endl;

    OcrBaseDialog::writeConfig();

    KConfig *conf = KGlobal::config();
    conf->setGroup(CFG_GROUP_OCR_DIA);
    conf->writePathEntry(CFG_GOCR_BINARY,QString(getOCRCmd()));

    conf->setGroup(CFG_GROUP_GOCR);
    conf->writeEntry(CFG_GOCR_GRAYLEVEL,getGraylevel());
    conf->writeEntry(CFG_GOCR_DUSTSIZE,getDustsize());
    conf->writeEntry(CFG_GOCR_SPACEWIDTH,getSpaceWidth());
}


void OcrGocrDialog::enableFields(bool enable)
{
    sliderGrayLevel->setEnabled(enable && m_isBW);
    sliderDustSize->setEnabled(enable);
    sliderSpace->setEnabled(enable);
}


void OcrGocrDialog::version(const QString &exe)
{
    if (m_proc!=NULL) delete m_proc;

    m_proc = new KProcess;

    kdDebug(28000) << k_funcinfo << "version of " << exe << endl;
    *m_proc << exe;
    *m_proc << "-h";

    connect(m_proc,SIGNAL(receivedStderr(KProcess *,char *,int)),
            SLOT(slReceiveStdErr(KProcess *,char *,int)));

    m_proc->start(KProcess::NotifyOnExit,KProcess::Stderr);
}


void OcrGocrDialog::slReceiveStdErr(KProcess *proc,char *buffer,int buflen)
{
    QString vstr = QString::fromUtf8(buffer,buflen);

    QRegExp rx("-- gocr ([\\d\\.]+)");
    if (rx.search(vstr)>-1)
    {
        QString vStr = rx.cap(1);
        ocrShowVersion(vStr);
    }
}
