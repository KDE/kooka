/***************************************************************************
                     kocrbase.cpp - base dialog for ocr
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
#include <kanimwidget.h>
#include <kactivelabel.h>
#include <kstandarddirs.h>
#include <ksconfig.h>
#include <kseparator.h>

#include <kio/job.h>
#include <kio/previewjob.h>

#include <qlabel.h>
#include <qhbox.h>
#include <qvbox.h>
#include <qgrid.h>
#include <qsizepolicy.h>
#include <qgroupbox.h>
#include <qcheckbox.h>

#include <qlayout.h>

#include "ocrengine.h"
#include "kookaimage.h"

#include "ocrbasedialog.h"
#include "ocrbasedialog.moc"


#define CFG_OCR_SPELL    "ocrSpellSettings"
#define CFG_WANT_KSPELL   "ocrKSpellEnabled"

#define CFG_OCR_KSPELL    "KSpellSettings"
#define CFG_KS_NOROOTAFFIX  "KSpell_NoRootAffix"
#define CFG_KS_RUNTOGETHER  "KSpell_RunTogether"
#define CFG_KS_DICTIONARY   "KSpell_Dictionary"
#define CFG_KS_DICTFROMLIST "KSpell_DictFromList"
#define CFG_KS_ENCODING     "KSpell_Encoding"
#define CFG_KS_CLIENT       "KSpell_Client"



OcrBaseDialog::OcrBaseDialog(QWidget *parent,KSpellConfig *spellConfig,KDialogBase::DialogType face)
    : KDialogBase(face,i18n("Optical Character Recognition"),
                  KDialogBase::User1|KDialogBase::User2|KDialogBase::Close,
                  KDialogBase::User1,parent,NULL,false,true,
                  KGuiItem(i18n("Start OCR"),"launch",i18n("Start the Optical Character Recognition process")),
                  KGuiItem(i18n("Cancel"),"stopocr",i18n("Stop the OCR Process"))),
      m_animation(NULL),
      m_metaBox(NULL),
      m_imgHBox(NULL),
      m_previewPix(NULL),
      m_spellConfig(spellConfig),
      m_wantSpellCfg(true),
      m_userWantsSpellCheck(true),
      m_cbWantCheck(NULL),
      m_gbSpellOpts(NULL),
      m_lVersion(NULL)
{
    kdDebug(28000) << k_funcinfo << endl;

    KConfig *konf = KGlobal::config();
    KConfigGroupSaver gs(konf,CFG_OCR_SPELL);
    m_userWantsSpellCheck = konf->readBoolEntry(CFG_WANT_KSPELL,true);

    if (m_spellConfig!=NULL && konf->hasGroup(CFG_OCR_KSPELL))
    {
        konf->setGroup(CFG_OCR_KSPELL);
        // from kdelibs/kdeui/ksconfig.cpp KSpellConfig:readGlobalSettings()
        m_spellConfig->setNoRootAffix(konf->readNumEntry(CFG_KS_NOROOTAFFIX,0));
        m_spellConfig->setRunTogether(konf->readNumEntry(CFG_KS_RUNTOGETHER,0));
        m_spellConfig->setDictionary(konf->readEntry(CFG_KS_DICTIONARY));
        m_spellConfig->setDictFromList(konf->readNumEntry(CFG_KS_DICTFROMLIST,false));
        m_spellConfig->setEncoding(konf->readNumEntry(CFG_KS_ENCODING,0));	// ASCII
        m_spellConfig->setClient(konf->readNumEntry(CFG_KS_CLIENT,0));		// ISpell
    }

    /* Connect signals which disable the fields and store the configuration */
    connect(this,SIGNAL(user1Clicked()),SLOT(writeConfig()));
    connect(this,SIGNAL(user1Clicked()),SLOT(startOCR()));
    connect(this,SIGNAL(user2Clicked()),SLOT(stopOCR()));

    m_previewSize.setWidth(200);
    m_previewSize.setHeight(300);

    enableButton(KDialogBase::User1,true);		/* Start OCR */
    enableButton(KDialogBase::User2,false);		/* Cancel */
    enableButton(KDialogBase::Close,true);		/* Close */
}


OcrBaseDialog::~OcrBaseDialog()
{
}


KAnimWidget* OcrBaseDialog::getAnimation(QWidget *parent)
{
   if (m_animation==NULL) m_animation = new KAnimWidget("kde",48,parent);
   return (m_animation);
}


OcrEngine::EngineError OcrBaseDialog::setupGui()
{
    ocrIntro();
    imgIntro();
    if (m_wantSpellCfg) spellCheckIntro();
    return (OcrEngine::ENG_OK);
}


void OcrBaseDialog::imgIntro()
{
    m_imgPage = addVBoxPage( i18n("Source") );
    new QLabel(i18n("<b>Source Image Information</b>"),m_imgPage);

    // Caption - Label and image
    m_imgHBox = new QHBox( m_imgPage );

    m_imgHBox->setSpacing( KDialog::spacingHint());

    m_previewPix = new QLabel( m_imgHBox );
    m_previewPix->setPixmap(QPixmap());
    m_previewPix->setFixedSize(m_previewSize);
    m_previewPix->setAlignment( Qt::AlignCenter );
    m_previewPix->setFrameStyle( QFrame::Panel | QFrame::Sunken );
    // m_previewPix->resize(m_previewSize);

    /* See introduceImage where the meta box is filled with data from the
     * incoming widget.
     */
    m_metaBox = new QVBox( m_imgHBox );
}


/*
 * This creates a Tab OCR
 */
void OcrBaseDialog::ocrIntro()
{
    m_ocrPage = addVBoxPage(i18n("OCR"));

    QWidget *topWid = new QWidget(m_ocrPage);		// engine title/logo/description
    QGridLayout *gl = new QGridLayout(topWid,3,2,0,KDialog::spacingHint());

    QLabel *l = new QLabel(i18n("<b>Optical Character Recognition using %1</b><br>")
                          .arg(ocrEngineName()),topWid);
    gl->addWidget(l,0,0);

    // Find the logo and display if available
    KStandardDirs stdDir;
    QString logoFile = KGlobal::dirs()->findResource("data","kooka/pics/"+ocrEngineLogo());
    QPixmap pix(logoFile);
    if (!pix.isNull())
    {
        QLabel *imgLab = new QLabel(topWid);
        imgLab->setPixmap(pix);
        gl->addWidget(imgLab,0,1,Qt::AlignRight|Qt::AlignTop);
    }

    gl->setRowSpacing(1,KDialog::spacingHint());

    KActiveLabel *al = new KActiveLabel(ocrEngineDesc(),topWid);
    gl->addMultiCellWidget(al,2,2,0,1);

    gl->setRowStretch(2,1);
    gl->setColStretch(0,1);
}



void OcrBaseDialog::ocrShowInfo(const QString &binary,const QString &version)
{
    new KSeparator(KSeparator::HLine,m_ocrPage);

    QWidget *botWid = new QWidget(m_ocrPage);		// engine path/version/spinner
    QGridLayout *gl = new QGridLayout(botWid,3,3,0,KDialog::spacingHint());
    QLabel *l;

    if (!binary.isNull())
    {
        l = new QLabel(i18n("%1 binary:").arg(ocrEngineName()),botWid);
        gl->addWidget(l,0,0,Qt::AlignRight);

        l = new QLabel(QString("%1").arg(binary),botWid);
        gl->addWidget(l,0,1,Qt::AlignLeft);

        l = new QLabel(i18n("Version:"),botWid);
        gl->addWidget(l,1,0,Qt::AlignRight);

        m_lVersion = new QLabel((!version.isNull() ? version : i18n("unknown")),botWid);
        gl->addWidget(m_lVersion,1,1,Qt::AlignLeft);
    }

    gl->addMultiCellWidget(getAnimation(botWid),0,2,2,2,Qt::AlignTop|Qt::AlignRight);
    gl->setColStretch(1,1);
}


void OcrBaseDialog::ocrShowVersion(const QString &version)
{
    if (m_lVersion!=NULL) m_lVersion->setText(version);
}



void OcrBaseDialog::spellCheckIntro()
{
    m_spellchkPage = addVBoxPage( i18n("Spell Check") );

    new QLabel(i18n("<b>OCR Post Processing</b><br>"),m_spellchkPage);

    /* Want the spell checking at all? Checkbox here */
    m_cbWantCheck = new QCheckBox( i18n("Spell-check the OCR results"),
                                   m_spellchkPage );
    /* Spellcheck options */
    m_gbSpellOpts = new QGroupBox( 1, Qt::Horizontal, i18n("Spell-check Options"),
                                   m_spellchkPage );

    KSpellConfig *sCfg = new KSpellConfig(m_gbSpellOpts,NULL,m_spellConfig,false);
    m_spellConfig = sCfg;				// use our copy from now on

    /* connect toggle button */
    connect(m_cbWantCheck,SIGNAL(toggled(bool)),SLOT(slWantSpellcheck(bool)));
    m_cbWantCheck->setChecked(m_userWantsSpellCheck);
    m_gbSpellOpts->setEnabled(m_userWantsSpellCheck);

    /* A space eater */
    QWidget *spaceEater = new QWidget(m_spellchkPage);
    spaceEater->setSizePolicy( QSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored ));
}


void OcrBaseDialog::stopAnimation()
{
    if (m_animation!=NULL) m_animation->stop();
}


void OcrBaseDialog::startAnimation()
{
    if (m_animation!=NULL) m_animation->start();
}


void OcrBaseDialog::introduceImage(const KookaImage *img)
{
    if (img==NULL) return;
    kdDebug(28000) << k_funcinfo << "url=" << img->url() << " filebound=" << img->isFileBound() << endl;

    delete m_metaBox;
    m_metaBox = new QVBox(m_imgHBox);

    if (img->isFileBound())				// image backed by a file
    {
        /* Start to create a preview job for the thumb */
        KURL::List li(img->url());
        KIO::PreviewJob *m_job = KIO::filePreview(li,m_previewSize.width(),m_previewSize.height());
        if (m_job!=NULL)
        {
            m_job->setIgnoreMaximumSize();

            connect(m_job,SIGNAL(result(KIO::Job *)),
                    SLOT(slPreviewResult(KIO::Job *)));
            connect(m_job,SIGNAL(gotPreview(const KFileItem *,const QPixmap &)),
                    SLOT(slGotPreview(const KFileItem *,const QPixmap &)));
            /* KIO::Job result is called in any way: Success, Failed, Error,
             * thus connecting the failed is not really necessary.
             */
        }
    }
    else						// image only exists in memory
    {							// do the preview ourselves
        QImage qimg = img->smoothScale(m_previewSize,QImage::ScaleMin);
        slGotPreview(NULL,QPixmap(qimg));
    }

    KFileMetaInfo info = img->fileMetaInfo();
    if (info.isValid())					// available from file
    {
        QStringList groups = info.preferredGroups();
        for ( QStringList::const_iterator it = groups.constBegin();
              it!=groups.constEnd(); ++it)
        {
            const QString theGroup(*it);
            QStringList keys = info.group(theGroup).supportedKeys();
            if (keys.count()>0)
            {
                // info.groupInfo( theGroup )->translatedName()
                // FIXME: howto get the translated group name?
                QLabel *lGroup = new QLabel(theGroup,m_metaBox);
                lGroup->setBackgroundColor(QColor(gray));
                lGroup->setMargin(KDialog::spacingHint());

                QGrid *nGrid = new QGrid(2,m_metaBox);
                nGrid->setSpacing(KDialog::spacingHint());
                for (QStringList::const_iterator keyIt = keys.constBegin();
                     keyIt!=keys.constEnd(); ++keyIt)
                {
                    KFileMetaInfoItem item = info.item(*keyIt);
                    QString itKey = item.translatedKey();
                    if (itKey.isEmpty()) itKey = item.key();
                    if (!itKey.isEmpty())
                    {
                        new QLabel(item.translatedKey()+": ",nGrid);
                        new QLabel(item.string(),nGrid);
                        kdDebug(28000) << "hasKey " << *keyIt << endl;
                    }
                }
            }
        }
    }
    else						// basic information by hand
    {
        QLabel *lGroup = new QLabel(i18n("Selection"),m_metaBox);
        lGroup->setBackgroundColor(QColor(gray));
        lGroup->setMargin(KDialog::spacingHint());

        QGrid *nGrid = new QGrid(2,m_metaBox);
        nGrid->setSpacing(KDialog::spacingHint());

        new QLabel(i18n("Dimensions:"),nGrid);
        new QLabel(QString("%1 x %2 pixels").arg(img->width()).arg(img->height()),nGrid);

        new QLabel(i18n("Bit Depth:"),nGrid);
        new QLabel(QString("%1 bpp").arg(img->depth()),nGrid);
    }

    QWidget *spaceEater = new QWidget(m_metaBox);
    spaceEater->setSizePolicy(QSizePolicy(QSizePolicy::Ignored,QSizePolicy::Ignored));
    m_metaBox->show();
}


void OcrBaseDialog::slPreviewResult(KIO::Job *job)
{
    if (job!=NULL && job->error()>0) job->showErrorDialog(NULL);
}


void OcrBaseDialog::slGotPreview(const KFileItem *item,const QPixmap &newPix)
{
    kdDebug(28000) << k_funcinfo << endl;
    if (m_previewPix!=NULL) m_previewPix->setPixmap(newPix);
}


void OcrBaseDialog::writeConfig()
{
    kdDebug(28000) << k_funcinfo << endl;

    KConfig *conf = KGlobal::config();
    conf->setGroup(CFG_OCR_SPELL);
    conf->writeEntry(CFG_WANT_KSPELL,m_cbWantCheck->isChecked());

    if (m_spellConfig!=NULL)
    {
        conf->setGroup(CFG_OCR_KSPELL);
        // from kdelibs/kdeui/ksconfig.cpp KSpellConfig::writeGlobalSettings()
        conf->writeEntry(CFG_KS_NOROOTAFFIX,(int) m_spellConfig->noRootAffix());
        conf->writeEntry(CFG_KS_RUNTOGETHER,(int) m_spellConfig->runTogether());
        conf->writeEntry(CFG_KS_DICTIONARY,m_spellConfig->dictionary());
        conf->writeEntry(CFG_KS_DICTFROMLIST,(int) m_spellConfig->dictFromList());
        conf->writeEntry(CFG_KS_ENCODING,(int) m_spellConfig->encoding());
        conf->writeEntry(CFG_KS_CLIENT,m_spellConfig->client());
    }
}


void OcrBaseDialog::startOCR()
{
    enableFields(false);
    startAnimation();

    enableButton(KDialogBase::User1,false);		/* Start OCR */
    enableButton(KDialogBase::User2,true);		/* Cancel */
    enableButton(KDialogBase::Close,false);
}


void OcrBaseDialog::stopOCR()
{
    enableFields(true);
    stopAnimation();

    enableButton(KDialogBase::User1,true);
    enableButton(KDialogBase::User2,false);
    enableButton(KDialogBase::Close,true);
}


void OcrBaseDialog::slWantSpellcheck(bool wantIt)
{
    m_gbSpellOpts->setEnabled(wantIt);
    m_userWantsSpellCheck = wantIt;
}
