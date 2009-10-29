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

#include "ocrbasedialog.h"
#include "ocrbasedialog.moc"

#include <qlabel.h>
#include <qsizepolicy.h>
#include <q3groupbox.h>
#include <qcheckbox.h>
#include <qlayout.h>

#include <kconfig.h>
#include <kglobal.h>
#include <kdebug.h>
#include <klocale.h>
#include <kanimatedbutton.h>
#include <kstandarddirs.h>
#ifndef KDE4
#include <ksconfig.h>
#endif
#include <kseparator.h>
#include <khbox.h>
#include <kvbox.h>

#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <kio/previewjob.h>

#include "ocrengine.h"
#include "kookaimage.h"



#define CFG_OCR_SPELL    "ocrSpellSettings"
#define CFG_WANT_KSPELL   "ocrKSpellEnabled"

#define CFG_OCR_KSPELL    "KSpellSettings"
#define CFG_KS_NOROOTAFFIX  "KSpell_NoRootAffix"
#define CFG_KS_RUNTOGETHER  "KSpell_RunTogether"
#define CFG_KS_DICTIONARY   "KSpell_Dictionary"
#define CFG_KS_DICTFROMLIST "KSpell_DictFromList"
#define CFG_KS_ENCODING     "KSpell_Encoding"
#define CFG_KS_CLIENT       "KSpell_Client"



OcrBaseDialog::OcrBaseDialog(QWidget *parent, KSpellConfig *spellConfig)
    : KPageDialog(parent),
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
    kDebug();

    setObjectName("OcrBaseDialog");

    setModal(true);
    setButtons(KDialog::User1|KDialog::User2|KDialog::Close);
    setDefaultButton(KDialog::User1);
    setCaption(i18n("Optical Character Recognition"));
    setButtonGuiItem(KDialog::User1, KGuiItem(i18n("Start OCR"), "launch", i18n("Start the Optical Character Recognition process")));
    setButtonGuiItem(KDialog::User2, KGuiItem(i18n("Stop OCR"), "cancel", i18n("Stop the Optical Character Recognition process")));

    const KConfigGroup grp1 = KGlobal::config()->group(CFG_OCR_SPELL);
    m_userWantsSpellCheck = grp1.readEntry(CFG_WANT_KSPELL, true);

#ifndef KDE4
    if (m_spellConfig!=NULL && KGlobal::config()->hasGroup(CFG_OCR_KSPELL))
    {
        const KConfigGroup grp2 = KGlobal::config()->group(CFG_OCR_KSPELL);
        // from kdelibs/kdeui/ksconfig.cpp KSpellConfig:readGlobalSettings()
        m_spellConfig->setNoRootAffix(grp2.readEntry(CFG_KS_NOROOTAFFIX, 0));
        m_spellConfig->setRunTogether(grp2.readEntry(CFG_KS_RUNTOGETHER, 0));
        m_spellConfig->setDictionary(grp2.readEntry(CFG_KS_DICTIONARY));
        m_spellConfig->setDictFromList(grp2.readEntry(CFG_KS_DICTFROMLIST, false));
        m_spellConfig->setEncoding(grp2.readEntry(CFG_KS_ENCODING, 0));	// ASCII
        m_spellConfig->setClient(grp2.readEntry(CFG_KS_CLIENT, 0));	// ISpell
    }
#endif

    /* Connect signals which disable the fields and store the configuration */
    connect(this, SIGNAL(user1Clicked()), SLOT(slotWriteConfig()));
    connect(this, SIGNAL(user1Clicked()), SLOT(slotStartOCR()));
    connect(this, SIGNAL(user2Clicked()), SLOT(slotStopOCR()));

    m_previewSize.setWidth(200);
    m_previewSize.setHeight(300);

    enableButton(KDialog::User1, true);			/* Start OCR */
    enableButton(KDialog::User2, false);		/* Stop OCR */
    enableButton(KDialog::Close, true);			/* Close */
}


OcrBaseDialog::~OcrBaseDialog()
{
}


KAnimatedButton* OcrBaseDialog::getAnimation(QWidget *parent)
{
    if (m_animation==NULL) m_animation = new KAnimatedButton(parent);
    m_animation->setIcons("process-working-kde");
    // TODO: is this needed?
    //m_animation->setIconSize(KIconLoader::SizeLarge);
    m_animation->updateIcons();

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
    KVBox *vb = new KVBox(this);

    new QLabel(i18n("<b>Source Image Information</b>"), vb);

    // Caption - Label and image
    m_imgHBox = new KHBox(vb);
    m_imgHBox->setSpacing(KDialog::spacingHint());

    m_previewPix = new QLabel(m_imgHBox);
    m_previewPix->setPixmap(QPixmap());
    m_previewPix->setFixedSize(m_previewSize);
    m_previewPix->setAlignment( Qt::AlignCenter );
    m_previewPix->setFrameStyle( QFrame::Panel | QFrame::Sunken );
    // m_previewPix->resize(m_previewSize);

    /* See introduceImage where the meta box is filled with data from the
     * incoming widget.
     */
    m_metaBox = new KVBox(m_imgHBox);

    m_imgPage = addPage(vb, i18n("Source"));
}


/*
 * This creates a Tab OCR
 */
void OcrBaseDialog::ocrIntro()
{
    KVBox *vb = new KVBox(this);

    //new QLabel(i18n("<b>OCR Title</b><br>"), vb);

    // TODO: can topWid be combined with vb?
    QWidget *topWid = new QWidget(vb);			// engine title/logo/description
    QGridLayout *gl = new QGridLayout(topWid);

    QLabel *l = new QLabel(i18n("<b>Optical Character Recognition using %1</b><br>", ocrEngineName()),topWid);
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

    gl->setRowMinimumHeight(1, KDialog::spacingHint());

    l = new QLabel(ocrEngineDesc(),topWid);
    l->setWordWrap(true);
    l->setOpenExternalLinks(true);
    gl->addWidget(l, 2, 0, 1, 2);

    gl->setRowStretch(2, 1);
    gl->setColumnStretch(0, 1);

    m_ocrPage = addPage(vb, i18n("OCR"));
}



void OcrBaseDialog::ocrShowInfo(const QString &binary, const QString &version)
{
    QWidget *page = ocrPage()->widget();

    new KSeparator(Qt::Horizontal, page);

    QWidget *botWid = new QWidget(page);		// engine path/version/spinner
    QGridLayout *gl = new QGridLayout(botWid);
    QLabel *l;

    if (!binary.isNull())
    {
        l = new QLabel(i18n("%1 binary:", ocrEngineName()), botWid);
        gl->addWidget(l, 0, 0, Qt::AlignRight);

        l = new QLabel(binary, botWid);
        gl->addWidget(l, 0, 1, Qt::AlignLeft);

        l = new QLabel(i18n("Version:"),botWid);
        gl->addWidget(l, 1, 0, Qt::AlignRight);

        m_lVersion = new QLabel((!version.isNull() ? version : i18n("unknown")),botWid);
        gl->addWidget(m_lVersion, 1, 1,Qt::AlignLeft);
    }

    gl->addWidget(getAnimation(botWid), 0, 2, 3, 1, Qt::AlignTop|Qt::AlignRight);
    gl->setColumnStretch(1, 1);
}


void OcrBaseDialog::ocrShowVersion(const QString &version)
{
    if (m_lVersion!=NULL) m_lVersion->setText(version);
}



void OcrBaseDialog::spellCheckIntro()
{
    KVBox *vb = new KVBox(this);

    new QLabel(i18n("<b>OCR Post Processing</b><br>"), vb);

    /* Want the spell checking at all? Checkbox here */
    m_cbWantCheck = new QCheckBox( i18n("Spell-check the OCR results"),
                                   vb);
    /* Spellcheck options */
    m_gbSpellOpts = new Q3GroupBox( 1, Qt::Horizontal, i18n("Spell-check Options"),
                                   vb );
#ifndef KDE4
    KSpellConfig *sCfg = new KSpellConfig(m_gbSpellOpts,NULL,m_spellConfig,false);
    m_spellConfig = sCfg;				// use our copy from now on
#endif
    /* connect toggle button */
    connect(m_cbWantCheck,SIGNAL(toggled(bool)),SLOT(slWantSpellcheck(bool)));
    m_cbWantCheck->setChecked(m_userWantsSpellCheck);
    m_gbSpellOpts->setEnabled(m_userWantsSpellCheck);

    /* A space eater */
    QWidget *spaceEater = new QWidget(vb);
    spaceEater->setSizePolicy( QSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored ));

    m_spellChkPage = addPage(vb, i18n("Spell Check"));
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
    kDebug() << "url" << img->url() << "filebound" << img->isFileBound();

    delete m_metaBox;
    m_metaBox = new KVBox(m_imgHBox);

    if (img->isFileBound())				// image backed by a file
    {
        /* Start to create a preview job for the thumb */
        KUrl::List li(img->url());
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
        QImage qimg = img->scaled(m_previewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        slotGotPreview(NULL, QPixmap::fromImage(qimg));
    }

    KFileMetaInfo info = img->fileMetaInfo();
    if (info.isValid())					// available from file
    {
        QWidget *nGrid = new QWidget(m_metaBox);

        QGridLayout *gl = new QGridLayout(nGrid);
        int row = 0;					// row in grid layout

        const KFileMetaInfoGroupList groups = info.supportedGroups();
        for (KFileMetaInfoGroupList::const_iterator grpIt = groups.constBegin();
             grpIt!=groups.constEnd(); ++grpIt)
        {
            const KFileMetaInfoGroup theGroup(*grpIt);
            kDebug() << "group" << theGroup.name();

            const KFileMetaInfoItemList theItems = theGroup.items();
            if (!theItems.isEmpty())
            {
                QLabel *l = new QLabel(theGroup.name(), nGrid);
                QPalette pal = l->palette();
                pal.setColor(l->backgroundRole(), Qt::gray);
                l->setPalette(pal);
                l->setMargin(KDialog::spacingHint());
                gl->addWidget(l, row, 0, 1, 2);
                ++row;

                for (KFileMetaInfoItemList::const_iterator itemIt = theItems.constBegin();
                     itemIt!=theItems.constEnd(); ++itemIt)
                {
                    const KFileMetaInfoItem item = (*itemIt);
                    QString itName = item.name();
                    kDebug() << "item" << itName;

                    if (!itName.isEmpty())
                    {
                        l = new QLabel(itName+": ",nGrid);
                        gl->addWidget(l, row, 0);
                        l = new QLabel(item.prefix()+item.value().toString()+item.suffix(), nGrid);
                        gl->addWidget(l, row, 1);
                        ++row;
                    }
                }
            }
        }
    }
    else						// basic information by hand
    {
        QWidget *nGrid = new QWidget(m_metaBox);

        QGridLayout *gl = new QGridLayout(nGrid);

        QLabel *l = new QLabel(i18n("Selection"), m_metaBox);
        QPalette pal = l->palette();
        pal.setColor(l->backgroundRole(), Qt::gray);
        l->setPalette(pal);
        l->setMargin(KDialog::spacingHint());
        gl->addWidget(l, 0, 0, 1, 2);

        l = new QLabel(i18n("Dimensions:"), nGrid);
        gl->addWidget(l, 1, 0);
        l = new QLabel(i18n("%1 x %2 pixels", img->width(), img->height()), nGrid);
        gl->addWidget(l, 1, 1);

        l = new QLabel(i18n("Bit Depth:"), nGrid);
        gl->addWidget(l, 2, 0);
        l = new QLabel(i18n("%1 bpp", img->depth()), nGrid);
        gl->addWidget(l, 3, 1);
    }

    QWidget *spaceEater = new QWidget(m_metaBox);
    spaceEater->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored));
    m_metaBox->show();
}


void OcrBaseDialog::slotPreviewResult(KIO::Job *job)
{
    if (job!=NULL && job->error()>0)
    {
        job->ui()->setWindow(NULL);
        job->ui()->showErrorMessage();
    }
}


void OcrBaseDialog::slotGotPreview(const KFileItem *item, const QPixmap &newPix)
{
    kDebug();
    if (m_previewPix!=NULL) m_previewPix->setPixmap(newPix);
}


void OcrBaseDialog::slotWriteConfig()
{
    kDebug();

    KConfigGroup grp1 = KGlobal::config()->group(CFG_OCR_SPELL);
    grp1.writeEntry(CFG_WANT_KSPELL, m_cbWantCheck->isChecked());

#ifndef KDE4
    if (m_spellConfig!=NULL)
    {
        KConfigGroup grp2 = KGlobal::config()->group(CFG_OCR_KSPELL);
        // from kdelibs/kdeui/ksconfig.cpp KSpellConfig::writeGlobalSettings()
        grp2.writeEntry(CFG_KS_NOROOTAFFIX, (int) m_spellConfig->noRootAffix());
        grp2.writeEntry(CFG_KS_RUNTOGETHER, (int) m_spellConfig->runTogether());
        grp2.writeEntry(CFG_KS_DICTIONARY, m_spellConfig->dictionary());
        grp2.writeEntry(CFG_KS_DICTFROMLIST, (int) m_spellConfig->dictFromList());
        grp2.writeEntry(CFG_KS_ENCODING, (int) m_spellConfig->encoding());
        grp2.writeEntry(CFG_KS_CLIENT, m_spellConfig->client());
    }
#endif
}


void OcrBaseDialog::slotStartOCR()
{
    enableFields(false);
    startAnimation();

    enableButton(KDialog::User1,false);			/* Start OCR */
    enableButton(KDialog::User2,true);			/* Stop OCR */
    enableButton(KDialog::Close,false);			/* Close */
}


void OcrBaseDialog::slotStopOCR()
{
    enableFields(true);
    stopAnimation();

    enableButton(KDialog::User1,true);
    enableButton(KDialog::User2,false);
    enableButton(KDialog::Close,true);
}


void OcrBaseDialog::slotWantSpellcheck(bool wantIt)
{
    m_gbSpellOpts->setEnabled(wantIt);
    m_userWantsSpellCheck = wantIt;
}
