/***************************************************************************
                          kocrstartdia.cpp  -  description
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

KadmosDialog::KadmosDialog( QWidget *parent )
    :KOCRBase( parent, KDialogBase::Tabbed ),
     m_classifierCombo(0),
     m_cbNoise(0),
     m_cbAutoscale(0)
{
   kdDebug(28000) << "Starting KOCR-Start-Dialog!" << endl;
   // Layout-Boxes

   m_classifierTranslate["numplus.rec"]= i18n("Alphanumeric characters");
}

void KadmosDialog::setupGui()
{
   QVBox *page = addVBoxPage( i18n("OCR") );

   setupPreprocessing( addVBoxPage(   i18n("Preprocessing")));
   setupSegmentation( addVBoxPage(   i18n("Segmentation")));
   setupClassification( addVBoxPage( i18n("Classification")));

   KConfig *conf = KGlobal::config ();
   conf->setGroup( CFG_GROUP_KADMOS );

   // Caption - Label and image
   QHBox *hb_cap = new QHBox( page );
   hb_cap->setSpacing( KDialog::spacingHint());

   // Find the kadmos logo and display if available
   KStandardDirs stdDir;
   QString kadmosLogo = stdDir.findResource( "data", "kooka/pics/kadmoslogo.png" );

   kdDebug(28000)<< "Reading kadmos logo " << kadmosLogo << endl;
   QPixmap pix;
   if( pix.load( kadmosLogo ))
   {
       QLabel *imgLab = new QLabel( hb_cap );
       imgLab->setPixmap( pix );
   }

   QLabel *label = new QLabel( i18n( "<B>Starting Optical Character Recognition</B><P>"
				     "Kooka uses <I>the kadmos OCR engine</I>, a "
				     "commercial engine for optical character recognition.<P>"
				     "Kadmos is a product of <B>re Recognition AG</B><BR>"
				     "For more information about Kadmos OCR see "
				     "<A HREF=http://www.rerecognition.com>"
				     "http://www.rerecognition.com</A>"),
				     hb_cap, "captionImage" );
   Q_CHECK_PTR( label );


   // Horizontal line
   // KSeparator* f1 = new KSeparator( KSeparator::HLine, page);
   (void) new KSeparator( KSeparator::HLine, page);

   // Combo to select the classifiers
   addClassifierCombo( page, conf );

   QButtonGroup *cbGroup = new QButtonGroup( 1, Qt::Horizontal, i18n("OCR Modifier"), page );
   Q_CHECK_PTR(cbGroup);

   m_cbNoise = new QCheckBox( i18n( "Enable Automatic Noise Reduction" ), cbGroup );
   m_cbAutoscale = new QCheckBox( i18n( "Enable Automatic Scaling"), cbGroup );

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


void KadmosDialog::enableFields( bool b )
{
   kdDebug(28000) << "About to disable the entry fields" << endl;
   KOCRBase::enableFields(b);

   if( b )
       stopAnimation();
   else
       startAnimation();
}


/* The End ;) */

