/***************************************************************************
                     kocrbase.cpp - base dialog for ocr
                             -------------------
    begin                : Fri Now 10 2000
    copyright            : (C) 2000 by Klaas Freitag
    email                : freitag@suse.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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
#include <qhbox.h>
#include <qvbox.h>

#include "resource.h"
#include "kocrbase.h"
#include "ksaneocr.h"
#include "kookaimage.h"

#include <kscanslider.h>
#include <kstandarddirs.h>
#include <kfilemetainfo.h>
#include <qstringlist.h>
#include <qcolor.h>
#include <qgrid.h>
#include <qsizepolicy.h>

KOCRBase::KOCRBase( QWidget *parent, KDialogBase::DialogType face )
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
    m_currImg(0L)
{
    kdDebug(28000) << "OCR Base Dialog!" << endl;
    // Layout-Boxes

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

void KOCRBase::setupGui()
{
    ocrIntro();
    imgIntro();
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
    (void) new QLabel( ocrEngineName(), m_ocrPage );
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

    (void) new QLabel( ocrEngineDesc(), pa );
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
          * thus connecting the failed is not really neccessary.
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


void KOCRBase::startOCR()
{
    /* en- and disable the buttons */
    kdDebug(28000) << "Base: Starting OCR" << endl;
    enableButton( User1, false );   /* Start OCR */
    enableButton( User2, true );    /* Stop OCR */
    enableButton( Close, true );

    startAnimation();
}

void KOCRBase::stopOCR()
{
    enableButton( User1, true );   /* start ocr */
    enableButton( User2, false );  /* Cancel    */
    enableButton( Close, true );

    stopAnimation();

}

/* The End ;) */
#include "kocrbase.moc"
