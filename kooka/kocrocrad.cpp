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
#include <qregexp.h>

#include <kapplication.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kdebug.h>
#include <klocale.h>
#include <kanimwidget.h>
#include <kseparator.h>
#include <kmessagebox.h>
#include <kurlrequester.h>
#include <kprocess.h>

#include "resource.h"
#include "kocrocrad.h"
#include <kscanslider.h>
#include "kookaimage.h"
#include "kookapref.h"


#include <qcombobox.h>
#include <kvbox.h>



ocradDialog::ocradDialog( QWidget *parent, K3SpellConfig *spellConfig )
    :KOCRBase( parent, spellConfig, KPageDialog::Tabbed ),
     m_ocrCmd( QString()),
     m_orfUrlRequester(0L),
     m_layoutMode(0),
     m_binaryLabel(0),
     m_proc(0),
     m_version(0)
{
   kDebug(28000) << "Starting ocrad-Start-Dialog!" << endl;
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


int ocradDialog::layoutDetectionMode() const
{
    return m_layoutMode->currentItem();
}

EngineError ocradDialog::setupGui()
{
    KOCRBase::setupGui();

    KVBox *page = ocrPage();
    Q_CHECK_PTR( page );

    KSharedConfig::Ptr conf = KGlobal::config();
    conf->setGroup( CFG_GROUP_OCR_DIA );

    // Horizontal line
    // (void) new  KSeparator( Qt::Horizontal, page);

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

    /** layout detection button **/
    conf->setGroup( CFG_GROUP_OCRAD );
    int layoutDetect = conf->readNumEntry( CFG_OCRAD_LAYOUT_DETECTION, 0 );
    kDebug(28000) << "Layout detection from config: " << layoutDetect << endl;

    (void) new KSeparator( Qt::Horizontal, page);
    KHBox *hb1 = new KHBox(page);
    hb1->setSpacing( KDialog::spacingHint() );
    (void) new QLabel( i18n("OCRAD layout analysis mode: "), hb1);
    m_layoutMode = new QComboBox(hb1);
    m_layoutMode->insertItem(i18n("No Layout Detection"), 0 );
    m_layoutMode->insertItem(i18n("Column Detection"), 1 );
    m_layoutMode->insertItem(i18n("Full Layout Detection"), 2);
    m_layoutMode->setCurrentItem(layoutDetect);

    /** stating the ocrad binary **/
    (void) new KSeparator( Qt::Horizontal, page);
    KHBox *hb = new KHBox(page);
    hb->setSpacing( KDialog::spacingHint());

    m_binaryLabel = new QLabel( i18n("Using ocrad binary: ") + res, hb );

    // retrieve Program version and display
    version(res);

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
    if( m_proc )
        delete m_proc;
}

void ocradDialog::writeConfig( void )
{
   KSharedConfig::Ptr conf = KGlobal::config();
   conf->setGroup( CFG_GROUP_OCR_DIA );

   conf->writeEntry( CFG_OCRAD_BINARY, QString(getOCRCmd()));

   conf->setGroup( CFG_GROUP_OCRAD );
   conf->writeEntry( CFG_OCRAD_LAYOUT_DETECTION, m_layoutMode->currentItem());
}


void ocradDialog::enableFields(bool )
{
    kDebug(28000) << "About to disable the entry fields" << endl;
}

/* Later: Allow interactive loading of orf files
 *  for now, return empty string
 */
QString ocradDialog::orfUrl() const
{
    if( m_orfUrlRequester )
	return m_orfUrlRequester->url();
    else
	return QString();
}

void ocradDialog::version( const QString& exe )
{
    if( m_proc ) delete m_proc;

    m_proc = new KProcess;

    kDebug(28000) << "Using " << exe << " as command" << endl;
    *m_proc << exe;
    *m_proc << QString("-V");

    connect( m_proc, SIGNAL(receivedStdout(KProcess *, char *, int )),
             this,     SLOT(slReceiveStdIn(KProcess *, char *, int )));

    if( ! m_proc->start( KProcess::NotifyOnExit, KProcess::Stdout ) )
    {
        slReceiveStdIn( 0, (char*) "unknown", 7 );
    }
}

void ocradDialog::slReceiveStdIn( KProcess*, char *buffer, int buflen)
{
    QString vstr = QString::fromUtf8(buffer, buflen);

    kDebug(28000) << "Got input: "<< buffer << endl;

    QRegExp rx;
    rx.setPattern("GNU Ocrad version ([\\d\\.]+)");
    if( rx.search( vstr ) > -1 )
    {
        QString vStr = rx.cap(1);
        vStr.remove(0,2);

        m_version = vStr.toInt();
        QString v = i18n("Version: ") + rx.cap(1);

        if( m_binaryLabel )
        {
            m_binaryLabel->setText(m_binaryLabel->text() + '\n' + v );
            m_binaryLabel->update();
        }
    }
}

/*
 * returns the numeric version of the ocrad program. It is queried in the slot
 * slReceiveStdIn, which parses the output of the ocrad -V call.
 *
 * Attention: This method returns 10 for ocrad v. 0.10 and 8 for ocrad-0.8
 */
int ocradDialog::getNumVersion()
{
    return m_version;
}

#include "kocrocrad.moc"

/* The End ;) */

