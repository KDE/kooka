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
     m_classifierCombo(0),
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
    return i18n("KADMOS OCR Engine");
}

QString KadmosDialog::ocrEngineDesc() const
{
    return i18n("<B>Starting Optical Character Recognition</B><P>"
                "Kooka uses <I>the KADMOS OCR engine</I>, a "
                "commercial engine for optical character recognition.<P>"
                "Kadmos is a product of <B>re Recognition AG</B><BR>"
                "For more information about Kadmos OCR see "
                "<A HREF=http://www.rerecognition.com>"
                "http://www.rerecognition.com</A>");
}

void KadmosDialog::setupGui()
{

    KOCRBase::setupGui();
    setupPreprocessing( addVBoxPage(   i18n("Preprocessing")));
    setupSegmentation(  addVBoxPage(   i18n("Segmentation")));
    setupClassification( addVBoxPage( i18n("Classification")));

    /* continue page setup on the first page */
    QVBox *page = ocrPage();

    // Horizontal line
    (void) new KSeparator( KSeparator::HLine, page);

    KConfig *conf = KGlobal::config ();
    conf->setGroup( CFG_GROUP_KADMOS );

    // Combo to select the classifiers
    addClassifierCombo( page, conf );

    QHBox *innerBox = new QHBox( page );
    innerBox->setSpacing( KDialog::spacingHint());

    QButtonGroup *cbGroup = new QButtonGroup( 1, Qt::Horizontal, i18n("OCR Modifier"), innerBox );
    Q_CHECK_PTR(cbGroup);

    m_cbNoise = new QCheckBox( i18n( "Enable Automatic Noise Reduction" ), cbGroup );
    m_cbAutoscale = new QCheckBox( i18n( "Enable Automatic Scaling"), cbGroup );

    getAnimation(innerBox);
    // (void) new QWidget ( page );
}

/*
 * glob the classifier directory for *.rec or *.REC files. The directory must
 * be specified in app config under CFG_KADMOS_CLASSIFIER_PATH or it defaults
 * to the apps data dir.
 * For each classifier entry, a translated description is searched in the QMap
 * holding them. The translations are shown in the combobox.
 */
void KadmosDialog::addClassifierCombo( QWidget *addTo, KConfig *conf)
{
   QStrList classi;
   m_classifierPath = conf->readEntry( CFG_KADMOS_CLASSIFIER_PATH, "notyetset" );
   QString preSel = conf->readEntry( CFG_KADMOS_CLASSIFIER );

   if( m_classifierPath == "notyetset" )
   {
       // TODO
       kdDebug(28000) << "Classifierpath not yet set !" << endl;

   }

   QDir dir( m_classifierPath );
   QStringList lst = dir.entryList("*.rec; *.REC");
   for ( QStringList::Iterator it = lst.begin(); it != lst.end(); ++it )
   {
       if ( m_classifierTranslate.contains( *it ) )
       {
           const QString tr( m_classifierTranslate[*it] );
           classi.append( tr.latin1() ); // FIXME: Use proper QStrings here !
        }
       else
       {
           kdDebug(28000) << "Found Unknonw classifier " << (*it) << endl;
           classi.append( (*it).latin1());
       }
   }

   m_classifierCombo = new KScanCombo( addTo, i18n("Select a classifier: "), classi );

   if( ! preSel.isEmpty() )
       m_classifierCombo->slSetEntry( preSel );

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

QString KadmosDialog::getSelClassifier() const
{
    return m_classifierPath + "/" + getSelClassifierName();
}

QString KadmosDialog::getSelClassifierName() const
{
    QString res;
    if( ! m_classifierCombo ) return QString();
    QString trans = m_classifierCombo->currentText();

     StrMap::ConstIterator it;
     for ( it = m_classifierTranslate.begin(); it != m_classifierTranslate.end(); ++it )
     {
         if( it.data() == trans )
             return( it.key() );
     }
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
   KConfig *conf = KGlobal::config ();
   conf->setGroup( CFG_GROUP_KADMOS );

   conf->writeEntry( CFG_KADMOS_CLASSIFIER, getSelClassifierName());

}


void KadmosDialog::enableFields( bool )
{
   kdDebug(28000) << "About to disable the entry fields" << endl;
}


/* The End ;) */

