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

#include <qlayout.h>
#include <qlabel.h>
#include <qfileinfo.h>
#include <qtooltip.h>
#include <kio/job.h>
#include <kio/previewjob.h>

#include <kapplication.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kdebug.h>
#include <klocale.h>
#include <kanimwidget.h>
#include <kseparator.h>
#include <kmessagebox.h>
#include <kactivelabel.h>
#include <qhbox.h>
#include <qvbox.h>

#include "resource.h"
#include "kocrbase.h"
#include "ksaneocr.h"
#include "kookaimage.h"

#include <kscanslider.h>
#include <kstandarddirs.h>
#include <kfilemetainfo.h>
#include <ksconfig.h>
#include <qstringlist.h>
#include <qcolor.h>
#include <qgrid.h>
#include <qsizepolicy.h>
#include <qgroupbox.h>
#include <qcheckbox.h>

KOCRBase::KOCRBase( QWidget *parent, KSpellConfig *spellConfig,
                    KDialogBase::DialogType face )
   :KDialogBase( face, i18n("Optical Character Recognition"),
		 User2|Close|User1, User1, parent,0, false, true,
		 KGuiItem( i18n("Start OCR" ), "launch",
			   i18n("Start the Optical Character Recognition process" )),
                 KGuiItem( i18n("Cancel" ), "stopocr",
			   i18n("Stop the OCR Process" ))),
    m_animation(0L),
    m_metaBox(0L),
    m_imgHBox(0L),
    m_previewPix(0L),
    m_currImg(0L),
    m_spellConfig(spellConfig),
    m_wantSpellCfg(true),
    m_userWantsSpellCheck(true),
    m_cbWantCheck(0L),
    m_gbSpellOpts(0L)
{
    kdDebug(28000) << "OCR Base Dialog!" << endl;
    // Layout-Boxes

    KConfig *konf = KGlobal::config ();
    KConfigGroupSaver gs( konf, CFG_OCR_KSPELL );
    m_userWantsSpellCheck = konf->readBoolEntry(CFG_WANT_KSPELL, true);

    /* Connect signals which disable the fields and store the configuration */
    connect( this, SIGNAL( user1Clicked()), this, SLOT( writeConfig()));
    connect( this, SIGNAL( user1Clicked()), this, SLOT( startOCR() ));
    connect( this, SIGNAL( user2Clicked()), this, SLOT( stopOCR() ));
    m_previewSize.setWidth(200);
    m_previewSize.setHeight(300);

    enableButton( User1, true );   /* start ocr */
    enableButton( User2, false );  /* Cancel    */
    enableButton( Close, true );
}


KAnimWidget* KOCRBase::getAnimation(QWidget *parent)
{
   if( ! m_animation )
   {
      m_animation = new KAnimWidget( QString("kde"), 48, parent, "ANIMATION" );
   }
   return( m_animation );
}

EngineError KOCRBase::setupGui()
{
    ocrIntro();
    imgIntro();
    if( m_wantSpellCfg ) spellCheckIntro();

    return ENG_OK;
}

void KOCRBase::imgIntro()
{
    m_imgPage = addVBoxPage( i18n("Image") );
    (void) new QLabel( i18n("Image Information"), m_imgPage );

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
void KOCRBase::ocrIntro( )
{
    m_ocrPage = addVBoxPage( i18n("OCR") );

    // Caption - Label and image
    /* labelstring */
    (void) new QLabel( i18n("<b>Starting Optical Character Recognition with %1</b><p>").
                       arg( ocrEngineName() ), m_ocrPage );
    // Find the kadmos logo and display if available
    KStandardDirs stdDir;
    QString logo = stdDir.findResource( "data", "kooka/pics/" + ocrEngineLogo() );

    kdDebug(28000)<< "Reading logo " << logo << endl;
    QPixmap pix;
    QWidget *pa = m_ocrPage;

    if( pix.load( logo ))
    {
        QHBox *hb_cap = new QHBox( m_ocrPage );
        hb_cap->setSpacing( KDialog::spacingHint());

        QLabel *imgLab = new QLabel( hb_cap );
        imgLab->setAlignment( Qt::AlignHCenter | Qt::AlignTop  );
        imgLab->setPixmap( pix );
        pa = hb_cap;
    }

    (void) new KActiveLabel( ocrEngineDesc(), pa );
}


void KOCRBase::spellCheckIntro()
{
    m_spellchkPage = addVBoxPage( i18n("Spell-checking") );

    /* Want the spell checking at all? Checkbox here */
    QGroupBox *gb1 = new QGroupBox( 1, Qt::Horizontal, i18n("OCR Post Processing"), m_spellchkPage );
    m_cbWantCheck = new QCheckBox( i18n("Enable spell-checking for validation of the OCR result"),
                                   gb1 );
    /* Spellcheck options */
    m_gbSpellOpts = new QGroupBox( 1, Qt::Horizontal, i18n("Spell-Check Options"),
                                   m_spellchkPage );

    KSpellConfig *sCfg = new KSpellConfig( m_gbSpellOpts, "SPELLCHK", m_spellConfig, false );
    /* A space eater */
    QWidget *spaceEater = new QWidget(m_spellchkPage);
    spaceEater->setSizePolicy( QSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored ));

    /* connect toggle button */
    connect( m_cbWantCheck, SIGNAL(toggled(bool)), this, SLOT(slWantSpellcheck(bool)));
    m_cbWantCheck->setChecked( m_userWantsSpellCheck );
    m_gbSpellOpts->setEnabled( m_userWantsSpellCheck );
    m_spellConfig = sCfg;

    connect( sCfg, SIGNAL(configChanged()),
            this, SLOT(slSpellConfigChanged()));
}

void KOCRBase::slSpellConfigChanged()
{
    kdDebug(28000) << "Spellcheck config changed" << endl;
}



void KOCRBase::stopAnimation()
{
   if( m_animation )
      m_animation->stop();
}

void KOCRBase::startAnimation()
{
   if( m_animation )
      m_animation->start();
}

KOCRBase::~KOCRBase()
{

}

void KOCRBase::introduceImage( KookaImage* img)
{
    if( ! (img && img->isFileBound()) ) return;
    KFileMetaInfo info = img->fileMetaInfo();
    QStringList groups;
    if ( info.isValid() )
         groups = info.preferredGroups();

    delete m_metaBox;
    m_metaBox = new QVBox( m_imgHBox );

    /* Start to create a preview job for the thumb */
    KURL::List li(img->url());
    KIO::Job *m_job = KIO::filePreview(li, m_previewSize.width(),
                                       m_previewSize.height());

    if( m_job )
    {
        connect( m_job, SIGNAL( result( KIO::Job * )),
                 this, SLOT( slPreviewResult( KIO::Job * )));
        connect( m_job, SIGNAL( gotPreview( const KFileItem*, const QPixmap& )),
                 SLOT( slGotPreview( const KFileItem*, const QPixmap& ) ));
         /* KIO::Jo result is called in any way: Success, Failed, Error,
          * thus connecting the failed is not really necessary.
          */
    }

    for ( QStringList::Iterator it = groups.begin(); it != groups.end(); ++it )
    {
        QString theGroup(*it);

        kdDebug(29000) << "handling the group " << theGroup << endl;

        QStringList keys = info.group(theGroup).supportedKeys();

        if( keys.count() > 0 )
        {
            // info.groupInfo( theGroup )->translatedName()
            // FIXME: howto get the translated group name?
            QLabel *lGroup = new QLabel( theGroup, m_metaBox );
            lGroup->setBackgroundColor( QColor(gray));
            lGroup->setMargin( KDialog::spacingHint());

            QGrid *nGrid = new QGrid( 2,  m_metaBox );
            nGrid->setSpacing( KDialog::spacingHint());
            for ( QStringList::Iterator keyIt = keys.begin(); keyIt != keys.end(); ++keyIt )
            {
                KFileMetaInfoItem item = info.item(*keyIt);
                QString itKey = item.translatedKey();
                if( itKey.isEmpty() )
                    itKey = item.key();
                if( ! itKey.isEmpty() )
                {
                    (void) new QLabel( item.translatedKey() + ": ", nGrid );
                    (void) new QLabel( item.string(), nGrid );
                    kdDebug(29000) << "hasKey " << *keyIt << endl;
                }
            }
        }
    }
    QWidget *spaceEater = new QWidget( m_metaBox );
    spaceEater->setSizePolicy( QSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored ));
    m_metaBox->show();
}

void KOCRBase::slPreviewResult(KIO::Job *job )
{
    // nothing
    if( job && job->error() > 0 )
   {
      kdDebug(28000) << "Thumbnail Creation ERROR: " << job->errorString() << endl;
      job->showErrorDialog( 0 );
   }
}

void KOCRBase::slGotPreview( const KFileItem*, const QPixmap& newPix )
{
    kdDebug(28000) << "Got the preview" << endl;
    m_previewPix->setPixmap(newPix);

    if( m_previewPix && m_currImg )
    {
        m_previewPix->setPixmap(newPix);
    }
}


void KOCRBase::writeConfig()
{

}

bool KOCRBase::wantSpellCheck()
{
    return m_userWantsSpellCheck;
}

void KOCRBase::startOCR()
{
    /* en- and disable the buttons */
    kdDebug(28000) << "Base: Starting OCR" << endl;

    enableFields(false);
    enableButton( User1, false );   /* Start OCR */
    enableButton( User2, true );    /* Stop OCR */
    enableButton( Close, true );

    startAnimation();
}

void KOCRBase::stopOCR()
{
    enableFields(true);

    enableButton( User1, true );   /* start ocr */
    enableButton( User2, false );  /* Cancel    */
    enableButton( Close, true );

    stopAnimation();

}

void KOCRBase::enableFields(bool)
{

}

void KOCRBase::slWantSpellcheck( bool wantIt )
{
    if( m_gbSpellOpts )
    {
        m_gbSpellOpts->setEnabled( wantIt );
    }
    m_userWantsSpellCheck = wantIt;

    KConfig *konf = KGlobal::config ();
    KConfigGroupSaver gs( konf, CFG_OCR_KSPELL );
    konf->writeEntry( CFG_WANT_KSPELL, wantIt );
}

/* The End ;) */
#include "kocrbase.moc"
