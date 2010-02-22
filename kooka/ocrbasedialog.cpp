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
#include <q3groupbox.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qfile.h>
#include <qprogressbar.h>
#include <qapplication.h>

#include <kconfig.h>
#include <kglobal.h>
#include <kdebug.h>
#include <klocale.h>
#include <kstandarddirs.h>
#ifndef KDE4
#include <ksconfig.h>
#endif
#include <kseparator.h>
#include <kvbox.h>

#include <kio/job.h>
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
      m_setupPage(NULL),
      m_sourcePage(NULL),
      m_enginePage(NULL),
      m_spellPage(NULL),
      m_debugPage(NULL),
      m_previewPix(NULL),
      m_previewLabel(NULL),
      m_spellConfig(spellConfig),
      m_wantSpellCfg(true),
      m_userWantsSpellCheck(true),
      m_wantDebugCfg(true),
      m_cbWantCheck(NULL),
      m_gbSpellOpts(NULL),
      m_cbRetainFiles(NULL),
      m_lVersion(NULL),
      m_progress(NULL)
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

    // Signals which tell our caller what the user is doing
    connect(this, SIGNAL(user1Clicked()), SLOT(slotStartOCR()));
    connect(this, SIGNAL(user2Clicked()), SLOT(slotStopOCR()));
    connect(this, SIGNAL(closeClicked()), SLOT(slotCloseOCR()));

    m_previewSize.setWidth(380);			// minimum preview size
    m_previewSize.setHeight(250);

    enableButton(KDialog::User1, true);			// Start OCR
    enableButton(KDialog::User2, false);		// Stop OCR
    enableButton(KDialog::Close, true);			// Close
}


OcrBaseDialog::~OcrBaseDialog()
{
    kDebug();
}


OcrEngine::EngineError OcrBaseDialog::setupGui()
{
    setupSetupPage();
    if (m_wantSpellCfg) setupSpellPage();
    setupSourcePage();
    setupEnginePage();
    // TODO: preferences option for debug
    if (m_wantDebugCfg) setupDebugPage();

    return (OcrEngine::ENG_OK);
}


void OcrBaseDialog::setupSetupPage()
{
    QWidget *w = new QWidget(this);
    QGridLayout *gl = new QGridLayout(w);

    m_progress = new QProgressBar(this);
    m_progress->setVisible(false);

    m_setupPage = addPage(w, i18n("Setup"));
    m_setupPage->setHeader(i18n("Optical Character Recognition using %1", ocrEngineName()));
    m_setupPage->setIcon(KIcon("ocr"));
}


QWidget *OcrBaseDialog::addExtraPageWidget(KPageWidgetItem *page, QWidget *wid, bool stretchBefore)
{
    QGridLayout *gl = static_cast<QGridLayout *>(page->widget()->layout());
    int nextrow = gl->rowCount();
    // rowCount() seems to return 1 even if the layout is empty...
    if (gl->itemAtPosition(0,0)==NULL) nextrow = 0;

    if (stretchBefore)					// stretch before new row
    {
        gl->setRowStretch(nextrow, 1);
        ++nextrow;
    }
    else if (nextrow>0)					// something there already,
    {							// add separator line
        gl->addWidget(new KSeparator(Qt::Horizontal, this), nextrow, 0, 1, 2);
        ++nextrow;
    }

    if (wid==NULL) wid = new QWidget(this);
    gl->addWidget(wid, nextrow, 0, 1, 2);

    return (wid);
}



QWidget *OcrBaseDialog::addExtraSetupWidget(QWidget *wid, bool stretchBefore)
{
    return (addExtraPageWidget(m_setupPage, wid, stretchBefore));
}


void OcrBaseDialog::ocrShowInfo(const QString &binary, const QString &version)
{
    QWidget *w = addExtraEngineWidget();		// engine path/version/icon
    QGridLayout *gl = new QGridLayout(w);

    if (!binary.isNull())
    {
        QLabel *l = new QLabel(i18n("Executable:"), w);
        gl->addWidget(l, 0, 0, Qt::AlignLeft|Qt::AlignTop);

        l = new QLabel(binary, w);
        gl->addWidget(l, 0, 1, Qt::AlignLeft|Qt::AlignTop);

        l = new QLabel(i18n("Version:"), w);
        gl->addWidget(l, 1, 0, Qt::AlignLeft|Qt::AlignTop);

        m_lVersion = new QLabel((!version.isEmpty() ? version : i18n("unknown")), w);
        gl->addWidget(m_lVersion, 1, 1, Qt::AlignLeft|Qt::AlignTop);
    }

    // Find the logo and display if available
    KStandardDirs stdDir;
    QString logoFile = KGlobal::dirs()->findResource("data", "kooka/pics/"+ocrEngineLogo());
    QPixmap pix(logoFile);
    if (!pix.isNull())
    {
        QLabel *l = new QLabel(w);
        l->setPixmap(pix);
        gl->addWidget(l, 0, 3, 2, 1, Qt::AlignRight);
    }

    gl->setColumnStretch(2, 1);

}


void OcrBaseDialog::ocrShowVersion(const QString &version)
{
    if (m_lVersion!=NULL) m_lVersion->setText(version);
}



void OcrBaseDialog::setupSourcePage()
{
    QWidget *w = new QWidget(this);
    QGridLayout *gl = new QGridLayout(w);

    // These labels are filled with the preview pixmap and image
    // information in introduceImage()
    m_previewPix = new QLabel(i18n("No preview available"), w);
    m_previewPix->setPixmap(QPixmap());
    m_previewPix->setMinimumSize(m_previewSize.width()+KDialog::marginHint(),
                                 m_previewSize.height()+KDialog::marginHint());
    m_previewPix->setAlignment(Qt::AlignCenter);
    m_previewPix->setFrameStyle(QFrame::Panel|QFrame::Sunken);
    gl->addWidget(m_previewPix, 0, 0);
    gl->setRowStretch(0, 1);

    m_previewLabel = new QLabel(i18n("No information available"), w);
    gl->addWidget(m_previewLabel, 1, 0, Qt::AlignHCenter);

    m_sourcePage = addPage(w, i18n("Source"));
    m_sourcePage->setHeader(i18n("Source Image Information"));
    m_sourcePage->setIcon(KIcon("dialog-information"));
}


void OcrBaseDialog::setupEnginePage()
{
    QWidget *w = new QWidget(this);			// engine title/logo/description
    QGridLayout *gl = new QGridLayout(w);

    // row 0: engine description
    QLabel *l = new QLabel(ocrEngineDesc(), w);
    l->setWordWrap(true);
    l->setOpenExternalLinks(true);
    gl->addWidget(l, 0, 0, 1, 2, Qt::AlignTop);

    gl->setRowStretch(0, 1);
    gl->setColumnStretch(0, 1);

    m_enginePage = addPage(w, i18n("OCR Engine"));
    m_enginePage->setHeader(i18n("OCR Engine Information"));
    m_enginePage->setIcon(KIcon("application-x-executable"));
}


QWidget *OcrBaseDialog::addExtraEngineWidget(QWidget *wid, bool stretchBefore)
{
    return (addExtraPageWidget(m_enginePage, wid, stretchBefore));
}


void OcrBaseDialog::setupSpellPage()
{
    KVBox *vb = new KVBox(this);

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

    m_spellPage = addPage(vb, i18n("Spell Check"));
    m_spellPage->setHeader(i18n("OCR Result Spell Checking"));
    m_spellPage->setIcon(KIcon("tools-check-spelling"));
}



void OcrBaseDialog::setupDebugPage()
{
    QWidget *w = new QWidget(this);
    QGridLayout *gl = new QGridLayout(w);

    m_cbRetainFiles = new QCheckBox(i18n("Retain temporary files"), w);
    gl->addWidget(m_cbRetainFiles, 0, 0, Qt::AlignTop);



    m_debugPage = addPage(w, i18n("Debugging"));
    m_debugPage->setHeader(i18n("OCR Debugging"));
    m_debugPage->setIcon(KIcon("tools-report-bug"));
}


void OcrBaseDialog::stopAnimation()
{
    if (m_progress!=NULL) m_progress->setVisible(false);
}


void OcrBaseDialog::startAnimation()
{
    if (!m_progress->isVisible())			// progress bar not added yet
    {
        m_progress->setValue(0);
        addExtraSetupWidget(m_progress, true);
        m_progress->setVisible(true);
    }
}


// Not sure why this uses an asynchronous preview job for the image thumbnail
// (if it is possible, i.e. the image is file bound) as opposed to just scaling
// the image (which is always loaded at this point, i.e. it is already in memory).
// Possibly because scaling a potentially very large image could introduce a
// significant delay in opening the dialogue box, so making the GUI appear
// less responsive.  So we'll keep the preview job for now.
//
// What on earth happened to KFileMetaInfo in KDE4?  This used to have a fairly
// reasonable API, returning a list of key-value pairs grouped into sensible
// categories with readable strings available for each.  Now the groups have
// gone (so for example methods such as preferredGroups(), albeit being marked as
// 'deprecated', return an empty list!) and the key of each entry is an ontology
// URL.  Not sure what to do with this URL (although I'm sure it must be of
// interest to something), and it doesn't even return the minimal useful
// information (e.g. the size/depth) for many image file types anyway.
//
// Could this be why the "Meta Info" tab of the file properties dialogue also
// seems to have disappeared?
//
// So forget about KFileMetaInfo here, just display a simple label with the
// image size and depth (which information we already have available).

void OcrBaseDialog::introduceImage(const KookaImage *img)
{
    if (img==NULL) return;
    kDebug() << "url" << img->url() << "filebound" << img->isFileBound();

    if (img->isFileBound())				// image backed by a file
    {
        /* Start to create a preview job for the thumb */
        KUrl::List li(img->url());
        KIO::PreviewJob *job = KIO::filePreview(li, m_previewSize.width(), m_previewSize.height());
        if (job!=NULL)
        {
            job->setIgnoreMaximumSize();
            connect(job, SIGNAL(gotPreview(const KFileItem &, const QPixmap &)),
                    SLOT(slotGotPreview(const KFileItem &, const QPixmap &)));
        }
    }
    else						// selection only in memory,
    {							// do the preview ourselves
        QImage qimg = img->scaled(m_previewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        slotGotPreview(KFileItem(), QPixmap::fromImage(qimg));
    }

    if (m_previewLabel!=NULL)
    {
        m_previewLabel->setText(i18n("%1: %2 x %3 pixels, %4 bpp",
                                     (img->isFileBound() ? i18n("Image") : i18n("Selection")),
                                     img->width(), img->height(),
                                     img->depth()));
    }
}


void OcrBaseDialog::slotGotPreview(const KFileItem &item, const QPixmap &newPix)
{
    kDebug() << "pixmap" << newPix.size();
    if (m_previewPix!=NULL)
    {
        m_previewPix->setText(QString::null);
        m_previewPix->setPixmap(newPix);
    }
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
    setCurrentPage(m_setupPage);			// force back to first page
    enableGUI(true);					// disable while running

    slotWriteConfig();					// save configuration
    emit signalOcrStart();				// start the OCR process
}


void OcrBaseDialog::slotStopOCR()
{
    emit signalOcrStop();				// stop the OCR process
    enableGUI(false);					// enable everything again
}


void OcrBaseDialog::slotCloseOCR()
{
    emit signalOcrClose();				// indicate we're closed
}


void OcrBaseDialog::enableGUI(bool running)
{
    m_sourcePage->setEnabled(!running);
    m_enginePage->setEnabled(!running);
    if (m_spellPage!=NULL) m_spellPage->setEnabled(!running);
    if (m_debugPage!=NULL) m_debugPage->setEnabled(!running);
    enableFields(!running);				// engine's GUI widgets

    if (running) startAnimation();			// start our progress bar
    else stopAnimation();				// stop our progress bar

    enableButton(KDialog::User1, !running);		// Start OCR
    enableButton(KDialog::User2, running);		// Stop OCR
    enableButton(KDialog::Close, !running);		// Close

    QApplication::processEvents();			// ensure GUI up-to-date
}


void OcrBaseDialog::slotWantSpellcheck(bool wantIt)
{
    m_gbSpellOpts->setEnabled(wantIt);
    m_userWantsSpellCheck = wantIt;
}
