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
#include <kstandarddirs.h>
#include <qstringlist.h>


/* defines for konfig-reading */
#define CFG_GROUP_KADMOS "Kadmos"
#define CFG_KADMOS_CLASSIFIER_PATH "classifierPath"
#define CFG_KADMOS_CLASSIFIER      "classifier"


#define CNTRY_CZ i18n( "Czech Republic, Slovakia")
#define CNTRY_GB i18n( "Great Britain, USA" )

KadmosDialog::KadmosDialog( QWidget *parent, KSpellConfig *spellConfig )
    :KOCRBase( parent, spellConfig, KDialogBase::Tabbed ),
     m_cbNoise(0),
     m_cbAutoscale(0),
     m_haveNorm(false)
{
   kdDebug(28000) << "Starting KOCR-Start-Dialog!" << endl;
   // Layout-Boxes
   findClassifiers();
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


EngineError KadmosDialog::findClassifiers()
{
    findClassifierPath();

    KLocale *locale = KGlobal::locale();
    QStringList allCountries = locale->allLanguagesTwoAlpha ();
    for ( QStringList::Iterator it = allCountries.begin();
          it != allCountries.end(); ++it )
    {
        m_longCountry2short[locale->twoAlphaToCountryName(*it)] = *it;
    }
    m_longCountry2short[i18n("European Countries")] = "eu";
    m_longCountry2short[ CNTRY_CZ ] = "cz";
    m_longCountry2short[ CNTRY_GB ] = "us";

    QStringList lst;

    /* custom Path */
    if( ! m_customClassifierPath.isEmpty() )
    {
        QDir dir( m_customClassifierPath );

        QStringList lst1 = dir.entryList( "ttf*.rec" );

        for ( QStringList::Iterator it = lst1.begin(); it != lst1.end(); ++it )
        {
            lst << m_customClassifierPath + *it;
        }

        lst1 = dir.entryList( "hand*.rec" );

        for ( QStringList::Iterator it = lst1.begin(); it != lst1.end(); ++it )
        {
            lst << m_customClassifierPath + *it;
        }

        lst1 = dir.entryList( "norm*.rec" );

        for ( QStringList::Iterator it = lst1.begin(); it != lst1.end(); ++it )
        {
            lst << m_customClassifierPath + *it;
        }
    }
    else
    {
        /* standard location */
        KStandardDirs stdDir;
        kdDebug(28000) << "Starting to read resource" << endl;

        lst = stdDir.findAllResources( "data",
                                       "kooka/classifiers/*.rec",
                                       true,   /* recursive */
                                       true ); /* uniqu */
    }


    /* no go through lst and sort out hand-, ttf- and norm classifier */
    for ( QStringList::Iterator it = lst.begin(); it != lst.end(); ++it )
    {
        QFileInfo fi( *it);
        QString name = fi.fileName().lower();

        kdDebug(28000) << "Checking file " << *it << endl;

        if( name.startsWith( "ttf" ) )
        {
            QString lang = name.mid(3,2);
            if( allCountries.contains(lang) )
            {
                QString lngCountry = locale->twoAlphaToCountryName(lang);
                if( lngCountry.isEmpty() )
                    lngCountry = name;
                m_ttfClassifier << lngCountry;
                kdDebug(28000) << "ttf: Insert country " << lngCountry << endl;
            }
            else if( lang == "cz" )
            {
                m_ttfClassifier << CNTRY_CZ;
            }
            else if( lang == "us" )
            {
                m_ttfClassifier << CNTRY_GB;
            }
            else
            {
                m_ttfClassifier << name;
                kdDebug(28000) << "ttf: Unknown country" << endl;
            }
        }
        else if( name.startsWith( "hand" ) )
        {
            QString lang = name.mid(4,2);
            if( allCountries.contains(lang) )
            {
                QString lngCountry = locale->twoAlphaToCountryName(lang);
                if( lngCountry.isEmpty() )
                    lngCountry = name;
                m_handClassifier << lngCountry;
            }
            else if( lang == "cz" )
            {
                m_handClassifier << i18n( "Czech Republic, Slovakia");
            }
            else if( lang == "us" )
            {
                m_handClassifier << i18n( "Great Britain, USA" );
            }
            else
            {
                kdDebug(28000) << "Hand: Unknown country " << lang << endl;
                m_handClassifier << name;
            }
        }
        else if( name.startsWith( "norm" ))
        {
            m_haveNorm = true;
        }

        kdDebug(28000) << "Found classifier: " << *it << endl;
        m_classifierPath << *it;
    }

    if( m_handClassifier.count()+m_ttfClassifier.count()>0 )
    {
        /* There are classifiers */
        return ENG_OK;
    }
    else
    {
        /* Classifier are missing */
        return ENG_DATA_MISSING;
    }
}


EngineError KadmosDialog::findClassifierPath()
{
    KStandardDirs stdDir;
    EngineError err = ENG_OK;

    KConfig *conf = KGlobal::config ();
    KConfigGroupSaver gs( conf, CFG_GROUP_KADMOS );

    m_customClassifierPath = conf->readPathEntry( CFG_KADMOS_CLASSIFIER_PATH );
#if 0
    if( m_customClassifierPath == "NotFound" )
    {
        /* Wants the classifiers from the standard kde paths */
       KMessageBox::error(0, i18n("The classifier files for KADMOS could not be found.\n"
				  "OCR with KADMOS will not be possible!\n\n"
				  "Change the OCR engine in the preferences dialog."),
			  i18n("Installation Error") );
    }
    else
    {
        m_classifierPath = customPath;
    }
#endif
    return err;

}


EngineError KadmosDialog::setupGui()
{

    EngineError err = KOCRBase::setupGui();

    // setupPreprocessing( addVBoxPage(   i18n("Preprocessing")));
    // setupSegmentation(  addVBoxPage(   i18n("Segmentation")));
    // setupClassification( addVBoxPage( i18n("Classification")));

    /* continue page setup on the first page */
    QVBox *page = ocrPage();

    // Horizontal line
    (void) new KSeparator( KSeparator::HLine, page);

    // FIXME: dynamic classifier reading.

    (void) new QLabel( i18n("Please classify the font type and language of the text on the image:"),
		       page );
    QHBox *locBox = new QHBox( page );
    m_bbFont = new QButtonGroup(1, Qt::Horizontal, i18n("Font Type Selection"), locBox);

    m_rbMachine = new QRadioButton( i18n("Machine print"), m_bbFont );
    m_rbHand    = new QRadioButton( i18n("Hand writing"),  m_bbFont );
    m_rbNorm    = new QRadioButton( i18n("Norm font"),     m_bbFont );

    m_gbLang = new QGroupBox(1, Qt::Horizontal, i18n("Country"), locBox);


    m_cbLang = new QComboBox( m_gbLang );
    m_cbLang->setCurrentText( KLocale::defaultCountry() );

    connect( m_bbFont, SIGNAL(clicked(int)), this, SLOT(slFontChanged(int) ));
    m_rbMachine->setChecked(true);

    /* --- */
    QHBox *innerBox = new QHBox( page );
    innerBox->setSpacing( KDialog::spacingHint());

    QButtonGroup *cbGroup = new QButtonGroup( 1, Qt::Horizontal, i18n("OCR Modifier"), innerBox );
    Q_CHECK_PTR(cbGroup);

    m_cbNoise = new QCheckBox( i18n( "Enable automatic noise reduction" ), cbGroup );
    m_cbAutoscale = new QCheckBox( i18n( "Enable automatic scaling"), cbGroup );

    getAnimation(innerBox);
    // (void) new QWidget ( page );

    if( err != ENG_OK )
    {
       enableFields(false);
       enableButton(User1, false );
    }

    if( m_ttfClassifier.count() == 0 )
    {
        m_rbMachine->setEnabled(false);
    }
    if( m_handClassifier.count() == 0 )
    {
        m_rbHand->setEnabled(false);
    }
    if( !m_haveNorm )
        m_rbNorm->setEnabled(false);

    if( (m_ttfClassifier.count() + m_handClassifier.count()) == 0 && ! m_haveNorm )
    {
        KMessageBox::error(0, i18n("The classifier files for KADMOS could not be found.\n"
                                   "OCR with KADMOS will not be possible!\n\n"
                                   "Change the OCR engine in the preferences dialog."),
                           i18n("Installation Error") );
        err = ENG_BAD_SETUP;
    }
    else
        slFontChanged( 0 ); // Load machine print font language list
    return err;
}

void KadmosDialog::slFontChanged( int id )
{
    m_cbLang->clear();

    KConfig *conf = KGlobal::config ();
    KConfigGroupSaver gs( conf, CFG_GROUP_KADMOS );



    m_customClassifierPath = conf->readPathEntry( CFG_KADMOS_CLASSIFIER_PATH );

    bool enable = true;

    if( id == 0 )  /* Machine Print */
    {
        m_cbLang->insertStringList( m_ttfClassifier );
    }
    else if( id == 1 ) /* Hand Writing */
    {
        m_cbLang->insertStringList( m_handClassifier );
    }
    else if( id == 2 ) /* Norm Font */
    {
        enable = false;
    }
    m_cbLang->setEnabled( enable );
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
/*
 * returns the complete path of the classifier selected in the
 * GUI in the parameter path. The result value indicates if there
 * was one found.
 */

bool KadmosDialog::getSelClassifier( QString& path ) const
{
    QString classifier = getSelClassifierName();

    QString cmplPath;
    /*
     * Search the complete path for the classifier file name
     * returned from the getSelClassifierName method
     */
    for ( QStringList::ConstIterator it = m_classifierPath.begin();
          it != m_classifierPath.end(); ++it )
    {
        QFileInfo fi( *it );
        if( fi.fileName() == classifier )
        {
            cmplPath = *it;
            break;
        }
    }

    bool res = true;

    if( cmplPath.isEmpty() )
    {
        /* hm, no path was found */
	kdDebug(28000) << "ERR; The entire path is empty, joking?" << endl;
        res = false;
    }
    else
    {
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
    }
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

     /* Get the long text from the combo box */
     QString selLang = m_cbLang->currentText();
     QString trans;
     if( fType != "norm" && m_longCountry2short.contains( selLang ))
     {
         QString langType = m_longCountry2short[selLang];
         trans = fType+langType+".rec";
     }
     else
     {
         if( selLang.endsWith( ".rec" ))
         {
             /* can be a undetected */
             trans = selLang;
         }
	 else if( fType == "norm" )
	 {
	     trans = "norm.rec";
	 }
         else
             kdDebug(28000) << "ERROR: Not a valid classifier" << endl;
     }
     kdDebug(28000) << "Returning trans. "<< trans << endl;
     return( trans );
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
   m_gbLang->setEnabled( state );
}


/* The End ;) */

