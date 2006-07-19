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

/* $Id$ */

#include <qlayout.h>
#include <qlabel.h>
#include <qfileinfo.h>
#include <qtooltip.h>
#include <kpagedialog.h>
#include <kapplication.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kdebug.h>
#include <klocale.h>
#include <kanimwidget.h>
#include <kseparator.h>
#include <kmessagebox.h>

#include "resource.h"
#include "ksaneocr.h"  // TODO: Really needed?
#include "kocrgocr.h"
#include "kocrgocr.moc"
#include <kscanslider.h>
#include <kvbox.h>
#include "kookaimage.h"
#include "kookapref.h"



/* defines for konfig-reading */

#define CFG_GOCR_DUSTSIZE "gocrDustSize"
#define CFG_GOCR_GRAYLEVEL "gocrGrayLevel"
#define CFG_GOCR_SPACEWIDTH "gocrSpaceWidth"



KGOCRDialog::KGOCRDialog( QWidget *parent, K3SpellConfig *spellConfig )
    :KOCRBase( parent, spellConfig, KPageDialog::Tabbed ),
     m_ocrCmd( QString())
{
   kDebug(28000) << "Starting KOCR-Start-Dialog!" << endl;
   // Layout-Boxes
}

QString KGOCRDialog::ocrEngineLogo() const
{
    return "gocr.png";
}

QString KGOCRDialog::ocrEngineName() const
{
    return i18n("GOCR" );
}

QString KGOCRDialog::ocrEngineDesc() const
{
    return i18n("GOCR is an Open Source project "
                "for optical character recognition.<P>"
                "The author of gocr is <B>Joerg Schulenburg</B><BR>"
                "For more information about gocr see "
                "<A HREF=http://jocr.sourceforge.net>"
                "http://jocr.sourceforge.net</A>");
}

EngineError KGOCRDialog::setupGui()
{
    KOCRBase::setupGui();

    KVBox *page = ocrPage();
    Q_CHECK_PTR( page );

    KConfig *conf = KGlobal::config ();
    conf->setGroup( CFG_GROUP_OCR_DIA );

    // Horizontal line
    // (void) new  KSeparator( Qt::Horizontal, page);

    // Entry-Field.
    QString res = conf->readPathEntry( CFG_GOCR_BINARY, "notFound" );
    if( res == "notFound" )
    {
        res = KookaPreferences::tryFindGocr();
        if( res.isEmpty() )
        {
            /* Popup here telling that the config needs to be called */
            KMessageBox::sorry( this, i18n( "The path to the gocr binary is not configured yet.\n"
                                            "Please go to the Kooka configuration and enter the path manually."),
                                i18n("OCR Software Not Found") );
        }
    }

    if( res.isEmpty() )
        res = i18n("Not found");
    else
        m_ocrCmd = res;

    (void) new QLabel( i18n("Using GOCR binary: ") + res, page );
    (void) new KSeparator( Qt::Horizontal, page);

    KHBox *hb = new KHBox(page);
    hb->setSpacing( KDialog::spacingHint());
    KVBox *innerBox = new KVBox( hb );
    innerBox->setSpacing( KDialog::spacingHint());
    /* This is for a 'work-in-progress'-Animation */
    getAnimation(hb);

    /* Slider for OCR-Options */
    sliderGrayLevel = new KScanSlider( innerBox , i18n("&Gray level"), 0, 254, true, 160 );
    int numdefault = conf->readEntry( CFG_GOCR_GRAYLEVEL, 160 );
    sliderGrayLevel->slSetSlider( numdefault );
    sliderGrayLevel->setToolTip(
                   i18n( "The numeric value gray pixels are \nconsidered to be black.\n\nDefault is 160"));

    sliderDustSize = new KScanSlider( innerBox, i18n("&Dust size" ), 0, 60, true, 10 );
    numdefault = conf->readEntry( CFG_GOCR_DUSTSIZE, 10 );
    sliderDustSize->slSetSlider( numdefault );
    sliderDustSize->setToolTip(
                   i18n( "Clusters smaller than this value\nwill be considered to be dust and \nremoved from the image.\n\nDefault is 10"));

    sliderSpace = new KScanSlider( innerBox, i18n( "&Space width" ), 0, 60, true, 0 );
    numdefault = conf->readEntry( CFG_GOCR_SPACEWIDTH, 0 );
    sliderSpace->slSetSlider( numdefault );
    sliderSpace->setToolTip( i18n("Spacing between characters.\n\nDefault is 0 what means autodetection"));

    return ENG_OK;
}

void KGOCRDialog::introduceImage( KookaImage *img )
{
    if( !img ) return;

    KOCRBase::introduceImage( img );


    bool isOn = true;

    if( img->numColors() > 0 && img->numColors() <3 )
    {
        kDebug(29000) << "introduceImage: Have " << img->numColors() << " colors on depth " << img->depth() << endl;

        /* that means it is a black-and-white image. Thus we do not need the GrayLevel slider */
        isOn = false;
    }

    if( sliderGrayLevel )
        sliderGrayLevel->setEnabled( isOn );

}


KGOCRDialog::~KGOCRDialog()
{

}

void KGOCRDialog::writeConfig( void )
{
   KConfig *conf = KGlobal::config ();
   conf->setGroup( CFG_GROUP_OCR_DIA );

   conf->writeEntry( CFG_GOCR_BINARY, QString(getOCRCmd()));
   conf->writeEntry( CFG_GOCR_GRAYLEVEL, getGraylevel());
   conf->writeEntry( CFG_GOCR_DUSTSIZE, getDustsize());
   conf->writeEntry( CFG_GOCR_SPACEWIDTH, getSpaceWidth());
}


void KGOCRDialog::enableFields(bool b)
{
    kDebug(28000) << "About to disable the entry fields" << endl;
    sliderGrayLevel->setEnabled( b );
    sliderDustSize->setEnabled( b );
    sliderSpace->setEnabled( b );
}

/* The End ;) */

