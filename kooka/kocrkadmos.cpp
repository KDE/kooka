/***************************************************************************
                          kocrstartdia.cpp  -  description
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
#include <qvbox.h>
#include <qdict.h>
#include <qdir.h>
#include <qmap.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>

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
#include "kocrkadmos.h"
#include "kocrkadmos.moc"

#include <kscanslider.h>
#include <qcheckbox.h>
#include <qbuttongroup.h>
#include <kstandarddirs.h>

/* defines for konfig-reading */
#define CFG_GROUP_KADMOS "Kadmos"
#define CFG_KADMOS_CLASSIFIER_PATH "classifierPath"
#define CFG_KADMOS_CLASSIFIER      "classifier"

KadmosDialog::KadmosDialog( QWidget *parent, KSpellConfig *spellConfig )
    :KOCRBase( parent, spellConfig, KDialogBase::Tabbed ),
     m_cbNoise(0),
     m_cbAutoscale(0)
{
   kdDebug(28000) << "Starting KOCR-Start-Dialog!" << endl;
   // Layout-Boxes

   m_classifierTranslate["numplus.rec"]= i18n("Alphanumeric characters");
}

QString KadmosDialog::ocrEngineLogo() const
{
    return "kadmoslogo.png";
}

QString KadmosDialog::ocrEngineName() const
{
    return i18n("KADMOS OCR/ICR");
}

QString KadmosDialog::ocrEngineDesc() const
{
    return i18n("This version of Kooka was linked with the <I>KADMOS OCR/ICR engine</I>, a "
                "commercial engine for optical character recognition.<P>"
                "Kadmos is a product of <B>re Recognition AG</B><BR>"
                "For more information about Kadmos OCR see "
                "<A HREF=http://www.rerecognition.com>"
                "http://www.rerecognition.com</A>");
}

void KadmosDialog::setupGui()
{

    KOCRBase::setupGui();
    // setupPreprocessing( addVBoxPage(   i18n("Preprocessing")));
    // setupSegmentation(  addVBoxPage(   i18n("Segmentation")));
    // setupClassification( addVBoxPage( i18n("Classification")));

    /* continue page setup on the first page */
    QVBox *page = ocrPage();

    // Horizontal line
    (void) new KSeparator( KSeparator::HLine, page);

    KConfig *conf = KGlobal::config ();
    conf->setGroup( CFG_GROUP_KADMOS );

    // FIXME: dynamic classifier reading.
    KStandardDirs stdDir;
    const QString stdPath = stdDir.findResourceDir( "data", "kooka/classifiers/ttfus.rec" );
    kdDebug(28000) << "Standard-Classifier-Path: " << stdPath << endl;

    m_classifierPath = conf->readEntry( CFG_KADMOS_CLASSIFIER_PATH, stdPath );

    (void) new QLabel( i18n("Please classify the Font Type and Language of the text on the image:"),
		       page );
    QHBox *locBox = new QHBox( page );
    m_bbFont = new QButtonGroup(1, Qt::Horizontal, i18n("Font Type Selection"), locBox);
    (void) (new QRadioButton( i18n("Machine Print"), m_bbFont ))->setChecked(true);
    (void) new QRadioButton( i18n("Hand Writing"), m_bbFont );
    (void) new QRadioButton( i18n("Norm Font"), m_bbFont );

    m_bbRegion= new QButtonGroup(1, Qt::Horizontal, i18n("Language"), locBox);

    (void) new QRadioButton( i18n("US English"), m_bbRegion );
    (new QRadioButton( i18n("West European Languages"), m_bbRegion ))->setChecked(true);
    (void) new QRadioButton( i18n("East European Languages"), m_bbRegion );


    QHBox *innerBox = new QHBox( page );
    innerBox->setSpacing( KDialog::spacingHint());

    QButtonGroup *cbGroup = new QButtonGroup( 1, Qt::Horizontal, i18n("OCR Modifier"), innerBox );
    Q_CHECK_PTR(cbGroup);

    m_cbNoise = new QCheckBox( i18n( "Enable Automatic Noise Reduction" ), cbGroup );
    m_cbAutoscale = new QCheckBox( i18n( "Enable Automatic Scaling"), cbGroup );

    getAnimation(innerBox);
    // (void) new QWidget ( page );
}


void KadmosDialog::setupPreprocessing( QVBox*  )
{

}

void KadmosDialog::setupSegmentation(  QVBox* )
{

}

void KadmosDialog::setupClassification( QVBox* )
{

}

bool KadmosDialog::getSelClassifier( QString& path ) const
{
    QString classifier = getSelClassifierName();
    QString cmplPath = m_classifierPath + "kooka/classifiers/" + classifier;
    bool res = true;
    /* Check if the classifier exists on the HD. If not, return an empty string */
    QFileInfo fi(cmplPath);

    if( res && ! fi.exists() )
    {
        kdDebug(28000) << "Classifier file does not exist" << endl;
        path = i18n("Classifier file %1 does not exist").arg(classifier);
        res = false;
    }

    if( res && ! fi.isReadable() )
    {
        kdDebug(28000) << "Classifier file could not be read" << endl;
        path = i18n("Classifier file %1 is not readable").arg(classifier);
        res = false;
    }

    if( res )
        path = cmplPath;

    return res;
}

QString KadmosDialog::getSelClassifierName() const
{
     QButton *butt = m_bbFont->selected();

     QString fType, rType;

     if( butt )
     {
	int fontTypeID = m_bbFont->id(butt);
	if( fontTypeID == 0 )
	   fType = "ttf";
	else if( fontTypeID == 1 )
	   fType = "hand";
	else if( fontTypeID == 2 )
	   fType = "norm";
	else
	   kdDebug(28000) << "ERR: Wrong Font Type ID" << endl;
     }

     butt = m_bbRegion->selected();
     if( butt)
     {
	int regID = m_bbRegion->id( butt );
	if( regID == 0 )
	   rType = "us";   // american
	else if( regID == 1 )
	   rType = "eu";      //
	else if( regID == 2 )
	   rType = "ee";
	else
	   kdDebug(28000) << "ERR: Wrong Region Type ID" << endl;
     }
     QString trans = fType+rType+".rec";
     return QString(trans);
}

bool KadmosDialog::getAutoScale()
{
    return( m_cbAutoscale ? m_cbAutoscale->isChecked() : false );
}

bool KadmosDialog::getNoiseReduction()
{
    return( m_cbNoise ? m_cbNoise->isChecked() : false );

}

KadmosDialog::~KadmosDialog()
{

}

void KadmosDialog::writeConfig( void )
{

}


void KadmosDialog::enableFields( bool state )
{
   kdDebug(28000) << "About to disable the entry fields" << endl;
   m_cbNoise->setEnabled( state );
   m_cbAutoscale->setEnabled( state );

   m_bbFont->setEnabled( state );
   m_bbRegion->setEnabled( state );
}


/* The End ;) */

