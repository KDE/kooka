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

/* $Id$ */

#include <qlayout.h>
#include <qlabel.h>
#include <qfileinfo.h>
#include <qtooltip.h>

#include <kapplication.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kdebug.h>
#include <klocale.h>
#include <kanimwidget.h>
#include <kseparator.h>
#include <kmessagebox.h>
#include <kurlrequester.h>

#include "resource.h"
#include "kocrocrad.h"
#include <kscanslider.h>
#include "kookaimage.h"
#include "kookapref.h"
#include <qvbox.h>
#include <qhbox.h>


ocradDialog::ocradDialog( QWidget *parent, KSpellConfig *spellConfig )
    :KOCRBase( parent, spellConfig, KDialogBase::Tabbed ),
     m_ocrCmd( QString()),
     m_orfUrlRequester(0L)
{
   kdDebug(28000) << "Starting ocrad-Start-Dialog!" << endl;
   // Layout-Boxes
}

QString ocradDialog::ocrEngineLogo() const
{
    return "ocrad.png";
}

QString ocradDialog::ocrEngineName() const
{
    return i18n("ocrad" );
}

QString ocradDialog::ocrEngineDesc() const
{
    return i18n("ocrad is a Free Software project "
                "for optical character recognition.<p>"
                "The author of ocrad is <b>Antonio Diaz</b><br>"
                "For more information about ocrad see "
                "<A HREF=\"http://www.gnu.org/software/ocrad/ocrad.html\">"
                "http://www.gnu.org/software/ocrad/ocrad.html</A><p>"
        "Images should be scanned in black/white mode for ocrad.<br>"
        "Best results are achieved if the characters are at least 20 pixels high.<p>"
        "Problems arise, as usual, with very bold or very light or broken characters, "
        "the same with merged character groups.");
}

EngineError ocradDialog::setupGui()
{
    KOCRBase::setupGui();

    QVBox *page = ocrPage();
    Q_CHECK_PTR( page );

    KConfig *conf = KGlobal::config ();
    conf->setGroup( CFG_GROUP_OCR_DIA );

    // Horizontal line
    // (void) new  KSeparator( KSeparator::HLine, page);

    // Entry-Field.
    QString res = conf->readPathEntry( CFG_OCRAD_BINARY, "notFound" );
    if( res == "notFound" )
    {
        res = KookaPreferences::tryFindBinary("ocrad", CFG_OCRAD_BINARY);
        if( res.isEmpty() )
        {
            /* Popup here telling that the config needs to be called */
            KMessageBox::sorry( this, i18n( "The path to the ocrad binary is not configured yet.\n"
                                            "Please go to the Kooka configuration and enter the path manually."),
                                i18n("OCR Software Not Found") );
        }
    }

    if( res.isEmpty() )
        res = i18n("Not found");
    else
        m_ocrCmd = res;

    (void) new KSeparator( KSeparator::HLine, page);
    QHBox *hb = new QHBox(page);
    hb->setSpacing( KDialog::spacingHint());
    (void) new QLabel( i18n("Using ocrad binary: ") + res, hb );
    getAnimation(hb);

    /* This is for a 'work-in-progress'-Animation */

    return ENG_OK;
}

void ocradDialog::introduceImage( KookaImage *img )
{
    if( !img ) return;

    KOCRBase::introduceImage( img );
}


ocradDialog::~ocradDialog()
{

}

void ocradDialog::writeConfig( void )
{
   KConfig *conf = KGlobal::config ();
   conf->setGroup( CFG_GROUP_OCR_DIA );

   conf->writeEntry( CFG_OCRAD_BINARY, QString(getOCRCmd()));
}


void ocradDialog::enableFields(bool )
{
    kdDebug(28000) << "About to disable the entry fields" << endl;
}

/* Later: Allow interactive loading of orf files
 *  for now, return emty string
 */
QString ocradDialog::orfUrl() const
{
    if( m_orfUrlRequester )
	return m_orfUrlRequester->url();
    else
	return QString();
}

#include "kocrocrad.moc"

/* The End ;) */

