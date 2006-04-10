/***************************************************************************
                          ksaneocr.cpp  -  generic ocr
                             -------------------
    begin                : Fri Jun 30 2000
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

#include <kdebug.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <kapplication.h>
#include <ktempfile.h>
#include <kprocess.h>
#include <stdlib.h>
#include <kspell.h>
#include <kspelldlg.h>
#include <qfile.h>
#include <qcolor.h>
//Added by qt3to4:
#include <Q3CString>
#include <QTextStream>
#include <QEvent>
#include <Q3ValueList>
#include <QMouseEvent>
#include <stdio.h>
#include <unistd.h>

#include <img_canvas.h>

#include "img_saver.h"
#include "kadmosocr.h"
#include "kocrbase.h"
#include "kocrkadmos.h"
#include "kocrocrad.h"
#include "config.h"
#include "ksaneocr.h"
#include "kocrgocr.h"
#include "kookaimage.h"
#include "kookapref.h"
#include "ocrword.h"

#include <qtimer.h>
#include <qregexp.h>
#include <klocale.h>
#include <qpaintdevice.h>
#include <qpainter.h>
#include <qpen.h>
#include <qbrush.h>
#include <qfileinfo.h>

/*
 * Thread support is disabled here because the kadmos lib seems not to be
 * thread save unfortunately. See slotKadmosResult-comments for more information
 */

KSANEOCR::KSANEOCR( QWidget*, KConfig *cfg ):
    m_ocrProcessDia(0L),
    daemon(0L),
    visibleOCRRunning(false),
    m_resultImage(0),
    m_imgCanvas(0L),
    m_spell(0L),
    m_wantKSpell(true),
    m_kspellVisible(true),
    m_hideDiaWhileSpellcheck(true),
    m_spellInitialConfig(0L),
    m_parent(0L),
    m_ocrCurrLine(0),
    m_currHighlight(-1),
    m_applyFilter(false),
    m_unlinkORF(true)
{
    KConfig *konf = KGlobal::config ();
    m_ocrEngine = OCRAD;
    m_img = 0L;
    m_tmpFile = 0L;

    if( cfg )
        m_hideDiaWhileSpellcheck = cfg->readBoolEntry( HIDE_BASE_DIALOG, true );
    /*
     * a initial config is needed as a starting point for the config dialog
     * but also for ocr without visible dialog.
     */
    m_spellInitialConfig = new KSpellConfig( 0L, 0L ,0L, false );

    if( konf )
    {
        /* -- ocr dialog information -- */
        konf->setGroup( CFG_GROUP_OCR_DIA );
        QString eng = konf->readEntry(CFG_OCR_ENGINE, "ocrad");

        if( eng == "ocrad" )
        {
            m_ocrEngine = OCRAD;
        }
        else if( eng == "gocr" )
        {
            m_ocrEngine = GOCR;
        }
#ifdef HAVE_KADMOS
        else if( eng == QString("kadmos") ) m_ocrEngine = KADMOS;
#endif
        kDebug(28000) << "OCR engine is " << eng << endl;

        m_unlinkORF = konf->readBoolEntry( CFG_OCR_CLEANUP, true );
    }

    /* resize m_blocks to size 1 since there is at least one block */
    m_blocks.resize(1);

}


KSANEOCR::~KSANEOCR()
{
   if( daemon  ) {
      delete( daemon );
      daemon = 0;
   }
   if ( m_tmpFile )
   {
       m_tmpFile->setAutoDelete( true );
       delete m_tmpFile;
   }

   if( m_resultImage )
   {
       delete m_resultImage;
       m_resultImage = 0;
   }

   if( m_img ) delete m_img;
   if( m_spellInitialConfig ) delete m_spellInitialConfig;
}

/*
 * This slot is called to introduce a new image, usually if the user clicks on a
 * new image either in the gallery or on the thumbnailview.
 */
void KSANEOCR::slSetImage(KookaImage *img )
{
   if( ! img ) return           ;

   if( m_img )
       delete m_img;

   // FIXME: copy all the image is bad.
   m_img = new KookaImage(*img);

   if( m_ocrProcessDia )
   {
       m_ocrProcessDia->introduceImage( m_img );
   }

   m_applyFilter = false;
}

/*
 * Request to visualise a line-box in the source image, KADMOS Engine
 */
void KSANEOCR::slLineBox( const QRect& )
{
    if( ! m_img ) return;
}


/*
 * starts visual ocr process. Depending on the ocr engine, this function creates
 * a new dialog, and shows it.
 */
bool KSANEOCR::startOCRVisible( QWidget *parent )
{
   if( visibleOCRRunning ) return( false );
   bool res = true;

   m_parent = parent;

   if( m_ocrEngine == GOCR )
   {
       m_ocrProcessDia = new KGOCRDialog ( parent, m_spellInitialConfig );
   }
   else if( m_ocrEngine == OCRAD )
   {
       m_ocrProcessDia = new ocradDialog( parent, m_spellInitialConfig );
   }
   else if( m_ocrEngine == KADMOS )
   {
#ifdef HAVE_KADMOS
/*** Kadmos Engine OCR ***/
       m_ocrProcessDia = new KadmosDialog( parent, m_spellInitialConfig );
#else
       KMessageBox::sorry(0, i18n("This version of Kooka was not compiled with KADMOS support.\n"
           "Please select another OCR engine in Kooka's options dialog."));
       kDebug(28000) << "Sorry, this version of Kooka has no KADMOS support" << endl;
#endif /* HAVE_KADMOS */
   }
   else
   {
      kDebug(28000) << "ERR Unknown OCR engine requested!" << endl;
   }

   /*
    * this part is independant from the engine again
    */
   if( m_ocrProcessDia )
   {
       m_ocrProcessDia->setupGui();

       m_ocrProcessDia->introduceImage( m_img );
       visibleOCRRunning = true;

      connect( m_ocrProcessDia, SIGNAL( user1Clicked()), this, SLOT( startOCRProcess() ));
      connect( m_ocrProcessDia, SIGNAL( closeClicked()), this, SLOT( slotClose() ));
      connect( m_ocrProcessDia, SIGNAL( user2Clicked()), this, SLOT( slotStopOCR() ));
      m_ocrProcessDia->show();

   }
   return( res );
}

/**
 * This method should be called by the engine specific finish slots.
 * It does the not engine dependant cleanups like re-enabling buttons etc.
 */

void KSANEOCR::finishedOCRVisible( bool success )
{
   bool doSpellcheck = m_wantKSpell;

   if( m_ocrProcessDia )
   {
       m_ocrProcessDia->stopOCR();
       doSpellcheck = m_ocrProcessDia->wantSpellCheck();
   }

   if( success )
   {
       QString goof = ocrResultText();

       emit newOCRResultText(goof);

       if( m_imgCanvas )
       {
	   if( m_resultImage != 0 ) delete m_resultImage;
	   kDebug(28000) << "Result image name: " << m_ocrResultImage << endl;
	   m_resultImage = new QImage( m_ocrResultImage, "BMP" );
	   kDebug(28000) << "New result image has dimensions: " << m_resultImage->width() << "x" << m_resultImage->height()<< endl;
           /* The image canvas is non-zero. Set it to our image */
           m_imgCanvas->newImageHoldZoom( m_resultImage );
	   m_imgCanvas->setReadOnly(true);

           /* now handle double clicks to jump to the word */
           m_applyFilter=true;
       }

       /** now it is time to invoke the dictionary if required **/
       emit readOnlyEditor( false );

       if( doSpellcheck )
       {
           m_ocrCurrLine = 0;
           /*
            * create a new kspell object, based on the config of the base dialog
            */

           connect( new KSpell( m_parent, i18n("Kooka OCR Dictionary Check"),
                                this, SLOT( slSpellReady(KSpell*)),
                                m_ocrProcessDia->spellConfig() ),
                    SIGNAL( death()), this, SLOT(slSpellDead()));
       }

       delete m_ocrProcessDia;
       m_ocrProcessDia = 0L;

   }

   visibleOCRRunning =  false;
   cleanUpFiles();


   kDebug(28000) << "# ocr finished #" << endl;
}

/*
 * starting the spell check on line m_ocrCurrLine if the line exists.
 * If not, the function returns.
 */
void KSANEOCR::startLineSpellCheck()
{
    if( m_ocrCurrLine < m_ocrPage.size() )
    {
        m_checkStrings = (m_ocrPage[m_ocrCurrLine]).stringList();

        /* In case the checklist is empty, call the result slot immediately */
        if( m_checkStrings.count() == 0 )
        {
            slCheckListDone(false);
	    return;
        }

        kDebug(28000)<< "Wordlist (size " << m_ocrPage[m_ocrCurrLine].count() << ", line " << m_ocrCurrLine << "):" << m_checkStrings.join(", ") << endl;

        // if( list.count() > 0 )

        m_spell->checkList( &m_checkStrings, m_kspellVisible );
	kDebug(28000)<< "Started!" << endl;
        /**
         * This call ends in three slots:
         * 1. slMisspelling:    Hit _before_ the dialog (if any) appears. Time to
         *    mark the wrong word.
         * 2. slSpellCorrected: Hit if the user decided which word to use.
         * 3. slCheckListDone:  The line is finished. The global counter needs to be
         *    increased and this function needs to be called again.
         **/

    }
    else
    {
        kDebug(28000) << k_funcinfo <<" -- no more lines !" << endl;
        m_spell->cleanUp();
    }
}



/* User Cancel is called when the user does not really start the
 * ocr but uses the cancel-Button to come out of the Dialog */
void KSANEOCR::slotClose()
{
    kDebug(28000) << "closing ocr Dialog" << endl;
   if( daemon && daemon->isRunning() )
   {
      kDebug(28000) << "Still running - Killing daemon with Sig. 9" << endl;
      daemon->kill(9);
   }
   finishedOCRVisible(false);
}

void KSANEOCR::slotStopOCR()
{
    kDebug(28000) << "closing ocr Dialog" << endl;
    if( daemon && daemon->isRunning() )
    {
        kDebug(28000) << "Killing daemon with Sig. 9" << endl;
        daemon->kill(9);
        // that leads to the process being destroyed.
        KMessageBox::error(0, i18n("The OCR-process was stopped.") );
    }

}

void KSANEOCR::startOCRAD( )
{
    ocradDialog *ocrDia = static_cast<ocradDialog*>(m_ocrProcessDia);

    m_ocrResultImage = ocrDia->orfUrl();
    const QString cmd = ocrDia->getOCRCmd();

    // if( m_ocrResultImage.isEmpty() )
    {
	/* The url is empty. Start the program to fill up a temp file */
	m_ocrResultImage = ImgSaver::tempSaveImage( m_img, "BMP", 8 ); // m_tmpFile->name();
	kDebug(28000) << "The new image name is <" << m_ocrResultImage << ">" << endl;
    }

    m_ocrImagePBM = ImgSaver::tempSaveImage( m_img, "PBM", 1 );

    /* temporar file for orf result */
    KTempFile *tmpOrf = new KTempFile( QString(), ".orf" );
    tmpOrf->setAutoDelete( false );
    tmpOrf->close();
    m_tmpOrfName = QFile::encodeName(tmpOrf->name());


    if( daemon )
    {
	delete( daemon );
	daemon = 0;
    }

    daemon = new KProcess;
    Q_CHECK_PTR(daemon);

    *daemon << cmd;
    *daemon << QString("-x");
    *daemon <<  m_tmpOrfName;                   // the orf result file
    *daemon << QFile::encodeName( m_ocrImagePBM );      // The name of the image
    *daemon << QString("-l");
    *daemon << QString::number( ocrDia->layoutDetectionMode());

    KConfig *konf = KGlobal::config ();
    KConfigGroupSaver( konf, CFG_GROUP_OCRAD );

    QString format = konf->readEntry( CFG_OCRAD_FORMAT, "utf8");
    *daemon << QString("-F");
    *daemon << format;

    QString charset = konf->readEntry( CFG_OCRAD_CHARSET, "iso-8859-15");
    *daemon << QString("-c");
    *daemon << charset;


    QString addArgs = konf->readEntry( CFG_OCRAD_EXTRA_ARGUMENTS, QString() );

    if( !addArgs.isEmpty() )
    {
	kDebug(28000) << "Setting additional args from config for ocrad: " << addArgs << endl;
	*daemon << addArgs;
    }

    m_ocrResultText = "";

    connect(daemon, SIGNAL(processExited(KProcess *)),
	    this,   SLOT(  ocradExited(KProcess*)));
    connect(daemon, SIGNAL(receivedStdout(KProcess *, char*, int)),
	    this,   SLOT(  ocradStdIn(KProcess*, char*, int)));
    connect(daemon, SIGNAL(receivedStderr(KProcess *, char*, int)),
	    this,   SLOT(  ocradStdErr(KProcess*, char*, int)));

    if (!daemon->start(KProcess::NotifyOnExit, KProcess::All))
    {
	kDebug(28000) <<  "Error starting ocrad-daemon!" << endl;
    }
    else
    {
	kDebug(28000) << "Start OK" << endl;

    }
    delete tmpOrf;
}


void KSANEOCR::ocradExited(KProcess* )
{
    kDebug(28000) << "ocrad exit " << endl;
    QString err;
    bool parseRes = true;

    if( ! readORF(m_tmpOrfName, err) )
    {
        KMessageBox::error( m_parent,
                            i18n("Parsing of the OCR Result File failed:") + err,
                            i18n("Parse Problem"));
        parseRes = false;
    }
    finishedOCRVisible( parseRes );

}

void KSANEOCR::ocradStdErr(KProcess*, char* buffer, int buflen)
{
   QString errorBuffer = QString::fromLocal8Bit(buffer, buflen);
   kDebug(28000) << "ocrad says on stderr: " << errorBuffer << endl;

}

void KSANEOCR::ocradStdIn(KProcess*, char* buffer, int buflen)
{
   QString errorBuffer = QString::fromLocal8Bit(buffer, buflen);
   kDebug(28000) << "ocrad says on stdin: " << errorBuffer << endl;
}




/*
 * This slot is fired if the user clicks on the 'Start' button of the GUI, no
 * difference which engine is active.
 */
void KSANEOCR::startOCRProcess( void )
{
   if( ! m_ocrProcessDia ) return;

   /* starting the animation, setting fields disabled */
   m_ocrProcessDia->startOCR();

   kapp->processEvents();
   if( m_ocrEngine == OCRAD )
   {
       startOCRAD();
   }

   if( m_ocrEngine == GOCR )
   {
       /*
        * Starting a gocr process
        */

       KGOCRDialog *gocrDia = static_cast<KGOCRDialog*>(m_ocrProcessDia);

       const QString cmd = gocrDia->getOCRCmd();

       /* Save the image to a temp file */

       /**
        * Save images formats:
        * Black&White: PBM
        * Gray:        PGM
        * Bunt:        PPM
        */
       QString format;
       if( m_img->depth() == 1 )
           format = "PBM";
       else if( m_img->isGrayscale() )
           format = "PGM";
       else
           format = "PPM";

       QString tmpFile = ImgSaver::tempSaveImage( m_img, format ); // m_tmpFile->name();

       kDebug(28000) <<  "Starting GOCR-Command: " << cmd << " on file " << tmpFile
		      << ", format " << format << endl;

       if( daemon ) {
           delete( daemon );
           daemon = 0;
       }

       daemon = new KProcess;
       Q_CHECK_PTR(daemon);
       m_ocrResultText = "";

       connect(daemon, SIGNAL(processExited(KProcess *)),
               this,   SLOT(  gocrExited(KProcess*)));
       connect(daemon, SIGNAL(receivedStdout(KProcess *, char*, int)),
               this,   SLOT(  gocrStdIn(KProcess*, char*, int)));
       connect(daemon, SIGNAL(receivedStderr(KProcess *, char*, int)),
               this,   SLOT(  gocrStdErr(KProcess*, char*, int)));

       QString opt;
       *daemon << QFile::encodeName(cmd);
       *daemon << "-x";
       *daemon << "-";
       if( !( m_img->numColors() > 0 && m_img->numColors() <3 ))   /* not a bw-image */
       {
           *daemon << "-l";
           opt.setNum(gocrDia->getGraylevel());
           *daemon << opt;
       }
       *daemon << "-s";
       opt.setNum(gocrDia->getSpaceWidth());
       *daemon << opt;
       *daemon << "-d";
       opt.setNum(gocrDia->getDustsize());
       *daemon << opt;

       // Write an result image
       *daemon << "-v";
       *daemon << "32";

       // Unfortunately this is fixed by gocr.
       m_ocrResultImage = "out30.bmp";

       *daemon << QFile::encodeName(tmpFile);

       m_ocrCurrLine = 0;  // Important in gocrStdIn to store the results

       if (!daemon->start(KProcess::NotifyOnExit, KProcess::All))
       {
           kDebug(28000) <<  "Error starting daemon!" << endl;
       }
       else
       {
           kDebug(28000) << "Start OK" << endl;

       }
   }
#ifdef HAVE_KADMOS
   if( m_ocrEngine == KADMOS )
   {
       KadmosDialog *kadDia = static_cast<KadmosDialog*>(m_ocrProcessDia);

       kDebug(28000)  << "Starting Kadmos OCR Engine" << endl;

       QString clasPath;  /* target where the clasPath is written in */
       if( ! kadDia->getSelClassifier( clasPath ) )
       {
           KMessageBox::error( m_parent,
                               i18n("The classifier file necessary for OCR cannot be loaded: %1;\n"
                                   "OCR with the KADMOS engine is not possible." , 
                               clasPath), i18n("KADMOS Installation Problem"));
           finishedOCRVisible(false);
           return;
       }
       Q3CString c = clasPath.latin1();

       kDebug(28000) << "Using classifier " << c << endl;
       m_rep.Init( c );
       if( m_rep.kadmosError() ) /* check if kadmos initialised OK */
       {
	   KMessageBox::error( m_parent,
			       i18n("The KADMOS OCR system could not be started:\n") +
			       m_rep.getErrorText()+
			       i18n("\nPlease check the configuration." ),
			       i18n("KADMOS Failure") );
       }
       else
       {
	   /** Since initialising succeeded, we start the ocr here **/
	   m_rep.SetNoiseReduction( kadDia->getNoiseReduction() );
	   m_rep.SetScaling( kadDia->getAutoScale() );
	   kDebug(28000) << "Image size " << m_img->width() << " x " << m_img->height() << endl;
	   kDebug(28000) << "Image depth " << m_img->depth() << ", colors: " << m_img->numColors() << endl;
#define USE_KADMOS_FILEOP /* use a save-file for OCR instead of filling the reImage struct manually */
#ifdef USE_KADMOS_FILEOP
	   m_tmpFile = new KTempFile( QString(), QString("bmp"));
	   m_tmpFile->setAutoDelete( false );
	   m_tmpFile->close();
	   QString tmpFile = m_tmpFile->name();
	   kDebug() << "Saving to file " << tmpFile << endl;
	   m_img->save( tmpFile, "BMP" );
	   m_rep.SetImage(tmpFile);
#else
	   m_rep.SetImage(m_img);
#endif
	   // rep.Recognize();
	   m_rep.run();

	   /* Dealing with threads or no threads (using QT_THREAD_SUPPORT to distinguish)
	    * If threads are here, the recognition task is started in its own thread. The gui thread
	    * needs to wait until the recognition thread is finished. Therefore, a timer is fired once
	    * that calls slotKadmosResult and checks if the recognition task is finished. If it is not,
	    * a new one-shot-timer is fired in slotKadmosResult. If it is, the OCR result can be
	    * processed.
	    * In case the system has no threads, the method start of the recognition engine does not
	    * return until it is ready, the user has to live with a non responsive gui while
	    * recognition is performed. The start()-method is implemented as a wrapper to the run()
	    * method of CRep, which does the recognition job. Instead of pulling up a timer, simply
	    * the result slot is called if start()=run() has finished. In the result slot, finished()
	    * is only a dummy always returning true to avoid more preprocessor tags here.
	    * Hope that works ...
	    * It does not :( That is why it is not used here. Maybe some day...
	    */
       }
#ifdef QT_THREAD_SUPPORT
       /* start a timer and wait until it fires. */
       QTimer::singleShot( 500, this, SLOT( slotKadmosResult() ));
#else
       slotKadmosResult();
#endif

   }
#endif /* HAVE_KADMOS */
}

/*
 * This method is called to check if the kadmos process was already finished, if
 * thread support is enabled (check for preprocessor variable QT_THREAD_SUPPORT)
 * The problem is that the kadmos library seems not to be thread stable so thread
 * support should not be enabled by default. In case threads are enabled, this slot
 * checks if the KADMOS engine is finished already and if not it fires a timer.
 */

void KSANEOCR::slotKadmosResult()
{
#ifdef HAVE_KADMOS
    kDebug(28000) << "check for Recognition finished" << endl;


    if( m_rep.finished() )
    {
        /* The recognition thread is finished. */
        kDebug(28000) << "kadmos is finished." << endl;

        m_ocrResultText = "";
	if( ! m_rep.kadmosError() )
	{
	    int lines = m_rep.GetMaxLine();
	    kDebug(28000) << "Count lines: " << lines << endl;
	    m_ocrPage.clear();
	    m_ocrPage.resize( lines );

	    for( int line = 0; line < m_rep.GetMaxLine(); line++ )
	    {
		// ocrWordList wordList = m_rep.getLineWords(line);
		/* call an ocr engine independent method to use the spellbook */
		ocrWordList words = m_rep.getLineWords(line);
		kDebug(28000) << "Have " << words.count() << " entries in list" << endl;
		m_ocrPage[line]=words;
	    }

	    /* show results of ocr */
	    m_rep.End();
	}
	finishedOCRVisible( !m_rep.kadmosError() );

    }
    else
    {
        /* recognition thread is not yet finished. Wait another half a second. */
        QTimer::singleShot( 500, this, SLOT( slotKadmosResult() ));
        /* Never comes here if no threads exist on the system */
    }
#endif /* HAVE_KADMOS */
}




/*
 *
 */
void KSANEOCR::gocrExited(KProcess* d)
{
   kDebug(28000) << "daemonExited start !" << endl;

   /* Now all the text of gocr is in the member m_ocrResultText. This one must
    * be split up now to m_ocrPage. First break up the lines, resize m_ocrPage
    * accordingly and than go through every line and create ocrwords for every
    * word.
    */
   QStringList lines = QStringList::split( '\n', m_ocrResultText, true );

   m_ocrPage.clear();
   m_ocrPage.resize( lines.count() );

   kDebug(28000) << "RESULT " << m_ocrResultText << " was splitted to lines " << lines.count() << endl;

   unsigned int lineCnt = 0;

   for ( QStringList::Iterator it = lines.begin(); it != lines.end(); ++it )
   {
       kDebug(28000) << "Splitting up line " << *it << endl;
       ocrWordList ocrLine;

       QStringList words = QStringList::split( QRegExp( "\\s+" ), *it, false );
       for ( QStringList::Iterator itWord = words.begin(); itWord != words.end(); ++itWord )
       {
           kDebug(28000) << "Appending to results: " << *itWord << endl;
           ocrLine.append( ocrWord( *itWord ));
       }
       m_ocrPage[lineCnt] = ocrLine;
       lineCnt++;
   }
   kDebug(28000) << "Finished to split!" << endl;
   /* set the result pixmap to the result pix of gocr */
   if( ! m_resPixmap.load( m_ocrResultImage ) )
   {
       kDebug(28000) << "Can not load result image!" << endl;
   }

   /* load the gocr result image */
   if( m_img ) delete m_img;
   m_img = new KookaImage();
   m_img->load( "out30.bmp" );

   finishedOCRVisible( d->normalExit() );
}

/*
 *  A sample orf snippet:
 *
 *  # Ocr Results File. Created by GNU ocrad version 0.3pre1
 *  total blocks 2
 *  block 1 0 0 560 344
 *  lines 5
 *  line 1 chars 10 height 26
 *  71 109 17 26;2,'0'1,'o'0
 *  93 109 15 26;2,'1'1,'l'0
 *  110 109 18 26;1,'2'0
 *  131 109 18 26;1,'3'0
 *  151 109 19 26;1,'4'0
 *  172 109 17 26;1,'5'0
 *  193 109 17 26;1,'6'0
 *  213 108 17 27;1,'7'0
 *  232 109 18 26;1,'8'0
 *  253 109 17 26;1,'9'0
 *  line 2 chars 14 height 27
 *
 */

bool KSANEOCR::readORF( const QString& fileName, QString& errStr )
{
    QFile file( fileName );
    QRegExp rx;
    bool error = false;

    /* use a global line number counter here, not the one from the orf. The orf one
     * starts at 0 for every block, but we want line-no counting page global here.
     */
    unsigned int lineNo = 0;
    int	blockCnt = 0;
    int currBlock = -1;


    /* Fetch the numeric version of ocrad */
    ocradDialog *ocrDia = static_cast<ocradDialog*>(m_ocrProcessDia);
    int ocradVersion = 0;
    if( ocrDia )
    {
        ocradVersion = ocrDia->getNumVersion();
    }

    /* clear the ocr result page */
    m_ocrPage.clear();
    kDebug(28000) << "***** starting to analyse orf at " << fileName << " *****" << endl;

    /* some  checks on the orf */
    QFileInfo fi( fileName );
    if( ! fi.exists() ) {
        error = true;
        errStr = i18n("The orf %1 does not exist.", fileName);
    }
    if( ! error && ! fi.isReadable() ) {
        error = true;
        errStr = i18n("Permission denied on file %1.", fileName);
    }


    if ( !error && file.open( QIODevice::ReadOnly ) )
    {
        QTextStream stream( &file );
        QString line;
	QString recLine; // recognised line

        while ( !stream.atEnd() )
	{
            line = stream.readLine().trimmed(); // line of text excluding '\n'
	    int len = line.length();

	    if( ! line.startsWith( "#" ))  // Comments
	    {
		kDebug(28000) << "# Line check |" << line << "|" << endl;
		if( line.startsWith( "total blocks " ) )  // total count fo blocks, must be first line
		{
		    blockCnt = line.right( len - 13 /* QString("total blocks ").length() */ ).toInt();
		    kDebug(28000) << "Amount of blocks: " << blockCnt << endl;
		    m_blocks.resize(blockCnt);
		}
                else if( line.startsWith( "total text blocks " ))
                {
		    blockCnt = line.right( len - 18 /* QString("total text blocks ").length() */ ).toInt();
		    kDebug(28000) << "Amount of blocks (V. 10): " << blockCnt << endl;
		    m_blocks.resize(blockCnt);
                }
		else if( line.startsWith( "block ") || line.startsWith( "text block ") )
		{
		    rx.setPattern("^.*block\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)");
		    if( rx.search( line ) > -1)
		    {
			currBlock = (rx.cap(1).toInt())-1;
			kDebug(28000) << "Setting current block " << currBlock << endl;
			QRect r( rx.cap(2).toInt(), rx.cap(3).toInt(), rx.cap(4).toInt(), rx.cap(5).toInt());
			m_blocks[currBlock] = r;
		    }
		    else
		    {
			kDebug(28000) << "WRN: Unknown block line: " << line << endl;
                        // Not a killing bug
		    }
		}
		else if( line.startsWith( "lines " ))
		{
		    int lineCnt = line.right( len - 6 /* QString("lines ").length() */).toInt();
		    m_ocrPage.resize(m_ocrPage.size()+lineCnt);
		    kDebug(28000) << "Resized ocrPage to linecount " << lineCnt << endl;
		}
		else if( line.startsWith( "line" ))
		{
		    // line 5 chars 13 height 20
		    rx.setPattern("^line\\s+(\\d+)\\s+chars\\s+(\\d+)\\s+height\\s+\\d+" );
		    if( rx.search( line )>-1 )
		    {
			kDebug(28000) << "RegExp-Result: " << rx.cap(1) << " : " << rx.cap(2) << endl;
			int charCount = rx.cap(2).toInt();
			ocrWord word;
			QRect   brect;
			ocrWordList ocrLine;
                        ocrLine.setBlock(currBlock);
			/* Loop over all characters in the line. Every char has it's own line
			 * defined in the orf file */
			kDebug(28000) << "Found " << charCount << " chars for line " << lineNo << endl;

			for( int c=0; c < charCount && !stream.atEnd(); c++ )
			{
			    /* Read one line per character */
			    QString charLine = stream.readLine();
			    int semiPos = charLine.find(';');
			    if( semiPos == -1 )
			    {
				kDebug(28000) << "invalid line: " << charLine << endl;
			    }
			    else
			    {
 				QString rectStr = charLine.left( semiPos );
				QString results = charLine.remove(0, semiPos+1 );
                                bool lineErr = false;

				// rectStr contains the rectangle info of for the character
				// results contains the real result caracter

                                // find the amount of alternatives.
                                int altCount = 0;
                                int h = results.find(',');  // search the first comma
                                if( h > -1 ) {
                                    // kDebug(28000) << "Results of count search: " << results.left(h) << endl;
                                    altCount = results.left(h).toInt();
                                    results = results.remove( 0, h+1 ).trimmed();
                                } else {
                                    lineErr = true;
                                }
                                // kDebug(28000) << "Results-line after cutting the alter: " << results << endl;
                                QChar detectedChar = UndetectedChar;
                                if( !lineErr )
                                {
                                    /* take the first alternative only FIXME */
                                    if( altCount > 0 )
                                        detectedChar = results[1];
                                    // kDebug(28000) << "Found " << altCount << " alternatives for "
                                    //	       << QString(detectedChar) << endl;
                                }

                                /* Analyse the rectangle */
                                if( ! lineErr && detectedChar != ' ' )
                                {
                                    // kDebug(28000) << "STRING: " << rectStr << "<" << endl;
                                    rx.setPattern( "(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)");
                                    if( rx.search( rectStr ) != -1 )
                                    {
                                        /* unite the rectangles */
                                        QRect privRect( rx.cap(1).toInt(), rx.cap(2).toInt(),
                                                        rx.cap(3).toInt(), rx.cap(4).toInt() );
                                        word.setRect( word.rect() | privRect );
                                    }
                                    else
                                    {
                                        kDebug(28000) << "ERR: Unable to read rect info for char!" << endl;
                                        lineErr = true;
                                    }
                                }

                                if( ! lineErr )
                                {
                                    /* store word if finished by a space */
                                    if( detectedChar == ' ' )
                                    {
					/* add the block offset to the rect of the word */
					QRect r = word.rect();
                                        if( ocradVersion < 10 )
                                        {
                                            QRect blockRect = m_blocks[currBlock];
                                            r.moveBy( blockRect.x(), blockRect.y());
                                        }

					word.setRect( r );
                                        ocrLine.append( word );
                                        word = ocrWord();
                                    }
                                    else
                                    {
                                        word.append( detectedChar );
                                    }
                                }
			    }
			}
			if( !word.isEmpty() )
			{
			    /* add the block offset to the rect of the word */
			    QRect r = word.rect();
                            if( ocradVersion < 10 )
                            {
                                QRect blockRect = m_blocks[currBlock];
                                r.moveBy( blockRect.x(), blockRect.y());
                            }
			    word.setRect( r );

			    ocrLine.append( word );
			}
			if( lineNo < m_ocrPage.size() )
			{
			    kDebug(29000) << "Store result line no " << lineNo << "=\"" <<
                                ocrLine.first() << "..." << endl;
			    m_ocrPage[lineNo] = ocrLine;
			    lineNo++;
			}
			else
			{
			    kDebug(28000) << "ERR: line index overflow: " << lineNo << endl;
			}
		    }
                    else
                    {
                        kDebug(28000) << "ERR: Unknown line found: " << line << endl;
                    }
		}
		else
		{
		    kDebug(29000) << "Unknown line: " << line << endl;
		}
	    }  /* is a comment? */

        }
        file.close();
    }
    return !error;
}


void KSANEOCR::cleanUpFiles( void )
{
    if( m_tmpFile )
    {
        delete m_tmpFile;
        m_tmpFile = 0L;
    }

   if( ! m_ocrResultImage.isEmpty())
   {
      kDebug(28000) << "Unlinking OCR Result image file!" << endl;
      unlink(QFile::encodeName(m_ocrResultImage));
      m_ocrResultImage = QString();
   }

   if( ! m_ocrImagePBM.isEmpty())
   {
       kDebug(28000) << "Unlinking OCR PBM file!" << endl;
       unlink( QFile::encodeName(m_ocrImagePBM));
       m_ocrImagePBM = QString();
   }

   if( ! m_tmpOrfName.isEmpty() )
   {
       if( m_unlinkORF )
   {
      unlink(QFile::encodeName(m_tmpOrfName));
      m_tmpOrfName = QString();
   }
       else
       {
           kDebug(28000)  << "Do NOT unlink temp orf file " << m_tmpOrfName << endl;
       }
   }

   /* Delete the debug images of gocr ;) */
   unlink( "out20.bmp" );
}


void KSANEOCR::gocrStdErr(KProcess*, char* buffer, int buflen)
{
   QString errorBuffer = QString::fromLocal8Bit(buffer, buflen);
   kDebug(28000) << "gocr says: " << errorBuffer << endl;

}


void KSANEOCR::gocrStdIn(KProcess*, char* buffer, int buflen)
{
    QString aux = QString::fromLocal8Bit(buffer, buflen);

    QRegExp rx( "^\\s*\\d+\\s+\\d+");
    if( rx.search( aux ) > -1 )
    {
        /* calculate ocr progress for gocr */
        int progress = rx.capturedTexts()[0].toInt();
        int subProgress = rx.capturedTexts()[1].toInt();
        // kDebug(28000) << "Emitting progress: " << progress << endl;
        emit ocrProgress( progress, subProgress );
    }
    else
    {
        m_ocrResultText += aux;
    }

    // kDebug(28000) << aux << endl;

}

/*
 * Assemble the result text
 */
QString KSANEOCR::ocrResultText()
{
    QString res;
    const QString space(" ");

    /* start from the back and search the original word to replace it */
    Q3ValueVector<ocrWordList>::iterator pageIt;

    for( pageIt = m_ocrPage.begin(); pageIt != m_ocrPage.end(); ++pageIt )
    {
        /* thats goes over all lines */
        Q3ValueList<ocrWord>::iterator lineIt;
        for( lineIt = (*pageIt).begin(); lineIt != (*pageIt).end(); ++lineIt )
        {
            res += space + *lineIt;
        }
        res += "\n";
    }
    kDebug(28000) << "Returning result String  " << res << endl;
    return res;
}


/* --------------------------------------------------------------------------------
 * event filter to filter the mouse events to the image viewer
 */

void KSANEOCR::setImageCanvas( ImageCanvas *canvas )
{
    m_imgCanvas = canvas;

    m_imgCanvas->installEventFilter( this );
}


bool KSANEOCR::eventFilter( QObject *object, QEvent *event )
{
    QWidget *w = (QWidget*) object;

    if( m_applyFilter && m_imgCanvas && w == m_imgCanvas )
    {
        if( event->type() == QEvent::MouseButtonDblClick )
        {
            QMouseEvent *mev = static_cast<QMouseEvent*>(event);

            int x = mev->x();
            int y = mev->y();
            int scale = m_imgCanvas->getScaleFactor();

	    m_imgCanvas->viewportToContents( mev->x(), mev->y(),
					     x, y );

            kDebug(28000) << "Clicked to " << x << "/" << y << ", scale " << scale << endl;
            if( scale != 100 )
            {
                // Scale is e.g. 50 that means tha the image is only half of size.
                // thus the clicked coords must be multiplied with 2
                y = int(double(y)*100/scale);
                x = int(double(x)*100/scale);
            }
            /* now search the word that was clicked on */
            Q3ValueVector<ocrWordList>::iterator pageIt;

	    int line = 0;
	    bool valid = false;
	    ocrWord wordToFind;

            for( pageIt = m_ocrPage.begin(); pageIt != m_ocrPage.end(); ++pageIt )
            {
                QRect r = (*pageIt).wordListRect();

                if( y > r.top() && y < r.bottom() )
                {
		   kDebug(28000)<< "It is in between " << r.top() << "/" << r.bottom()
				 << ", line " << line << endl;
		   valid = true;
		   break;
                }
		line++;
            }

	    /*
	     * If valid, we have the line into which the user clicked. Now we
	     * have to find out the actual word
	     */
	    if( valid )
	    {
	       valid = false;
	       /* find the word in the line and mark it */
	       ocrWordList words = *pageIt;
	       ocrWordList::iterator wordIt;

	       for( wordIt = words.begin(); wordIt != words.end() && !valid; ++wordIt )
	       {
		  QRect r = (*wordIt).rect();
		  if( x > r.left() && x < r.right() )
		  {
		     wordToFind = *wordIt;
		     valid = true;
		  }
	       }

	    }

	    /*
	     * if valid, the wordToFind contains the correct word now.
	     */
	    if( valid )
	    {
	       kDebug(28000) << "Found the clicked word " << wordToFind << endl;
	       emit selectWord( line, wordToFind );
	    }

            return true;
        }
    }
    return false;
};



/* --------------------------------------------------------------------------------
 * Spellbook support
 */


/*
 * This slot is hit when the checkWord method of KSpell thinks a word is wrong.
 * KSpell detects the correction by itself and delivers it in newword here.
 * To see all alternatives KSpell proposes, slMissspelling must be used.
 */
void KSANEOCR::slSpellCorrected( const QString& originalword,
                                 const QString& newword,
                                 unsigned int pos )
{
    kDebug(28000) << "Corrected: Original Word " << originalword << " was corrected to "
                   << newword << ", pos ist " << pos << endl;

    kDebug(28000) << "Dialog state is " << m_spell->dlgResult() << endl;

    if( slUpdateWord( m_ocrCurrLine, pos, originalword, newword ) )
    {
        if( m_imgCanvas && m_currHighlight > -1 )
        {
            if( m_applyFilter )
                m_imgCanvas->removeHighlight( m_currHighlight );
        }
        else
        {
            kDebug(28000) << "No highlighting to remove!" << endl;
        }
    }

}


void KSANEOCR::slSpellIgnoreWord( const QString& word )
{
    ocrWord ignoreOCRWord;

    ignoreOCRWord = ocrWordFromKSpellWord( m_ocrCurrLine, word );
    if( ! ignoreOCRWord.isEmpty() )
    {
        emit ignoreWord( m_ocrCurrLine, ignoreOCRWord );

        if( m_imgCanvas && m_currHighlight > -1 )
        {
            m_imgCanvas->removeHighlight( m_currHighlight );

            /* create a new highlight. That will never be removed */
            QBrush brush;
            QPen pen( Qt::gray, 1 );
            QRect r = ignoreOCRWord.rect();
            r.moveBy(0,2);  // a bit offset to the top

            if( m_applyFilter )
                m_imgCanvas->highlight( r, pen, brush );
        }
    }
}

ocrWord KSANEOCR::ocrWordFromKSpellWord( int line, const QString& word )
{
    ocrWord resWord;
    if( lineValid(line) )
    {
        ocrWordList words = m_ocrPage[line];

        words.findFuzzyIndex( word, resWord );
    }

    return resWord;
}


bool KSANEOCR::lineValid( int line )
{
    bool ret = false;

    if( line >= 0 && (uint)line < m_ocrPage.count() )
        ret = true;

    return ret;
}

void KSANEOCR::slMisspelling( const QString& originalword, const QStringList& suggestions,
                              unsigned int pos )
{
    /* for the first try, use the first suggestion */
    ocrWord s( suggestions.first());
    kDebug(28000) << "Misspelled: " << originalword << " at position " << pos << endl;

    int line = m_ocrCurrLine;
    m_currHighlight = -1;

    // ocrWord resWord = ocrWordFromKSpellWord( line, originalword );
    ocrWordList words = m_ocrPage[line];
    ocrWord resWord;
    kDebug(28000) << "Size of wordlist (line " << line << "): " << words.count() << endl;

    if( pos < words.count() )
    {
        resWord = words[pos];
    }

    if( ! resWord.isEmpty() )
    {
        QBrush brush;
        brush.setColor( QColor(Qt::red)); // , "Dense4Pattern" );
        brush.setStyle( Qt::Dense4Pattern );
        QPen pen( Qt::red, 2 );
        QRect r = resWord.rect();

        r.moveBy(0,2);  // a bit offset to the top

        if( m_applyFilter )
            m_currHighlight = m_imgCanvas->highlight( r, pen, brush, true );

        kDebug(28000) << "Position ist " << r.x() << ", " << r.y() << ", width: "
		       << r.width() << ", height: " << r.height() << endl;

        /* draw a line under the word to check */

        /* copy the source */
        emit repaintOCRResImage();
    }
    else
    {
        kDebug(28000) << "Could not find the ocrword for " << originalword << endl;
    }

    emit markWordWrong( line, resWord );
}

/*
 * This is the global starting point for spell checking of the ocr result.
 * After the KSpell object was created in method finishedOCRVisible, this
 * slot is called if the KSpell-object feels itself ready for operation.
 * Coming into this slot, the spelling starts in a line by line manner
 */
void KSANEOCR::slSpellReady( KSpell *spell )
{
    m_spell = spell;
    connect ( m_spell, SIGNAL( misspelling( const QString&, const QStringList&,
                                            unsigned int )),
              this, SLOT( slMisspelling(const QString& ,
                                        const QStringList& ,
                                        unsigned int  )));
    connect( m_spell, SIGNAL( corrected ( const QString&, const QString&, unsigned int )),
             this, SLOT( slSpellCorrected( const QString&, const QString&, unsigned int )));

    connect( m_spell, SIGNAL( ignoreword( const QString& )),
             this, SLOT( slSpellIgnoreWord( const QString& )));

    connect( m_spell, SIGNAL( done(bool)), this, SLOT(slCheckListDone(bool)));

    kDebug(28000) << "Spellcheck available" << endl;

    if( m_ocrProcessDia && m_hideDiaWhileSpellcheck )
        m_ocrProcessDia->hide();
    emit readOnlyEditor( true );
    startLineSpellCheck();
}

/**
 * slot called after either the spellcheck finished or the KSpell object found
 * out that it does not want to run because of whatever problems came up.
 * If it is an KSpell-init problem, the m_spell variable is still zero and
 * Kooka pops up a warning.
 */
void KSANEOCR::slSpellDead()
{
    if( ! m_spell )
    {
        kDebug(28000) << "Spellcheck NOT available" << endl;
        /* Spellchecking has not yet been existing, thus there is a base problem with
         * spellcheck on this system.
         */
        KMessageBox::error( m_parent,
                            i18n("Spell-checking cannot be started on this system.\n"
                                 "Please check the configuration" ),
                            i18n("Spell-Check") );

    }
    else
    {
        if( m_spell->status() == KSpell::Cleaning )
        {
            kDebug(28000) << "KSpell cleans up" << endl;
        }
        else if( m_spell->status() == KSpell::Finished )
        {
            kDebug(28000) << "KSpell finished" << endl;
        }
        else if( m_spell->status() == KSpell::Error )
        {
            kDebug(28000) << "KSpell finished with Errors" << endl;
        }
        else if( m_spell->status() == KSpell::Crashed )
        {
            kDebug(28000) << "KSpell Chrashed" << endl;
        }
        else
        {
            kDebug(28000) << "KSpell finished with unknown state!" << endl;
        }

        /* save the current config */
        delete m_spell;
        m_spell = 0L;

        /* reset values */
        m_checkStrings.clear();
        m_ocrCurrLine = 0;
        if( m_imgCanvas && m_currHighlight > -1 )
            m_imgCanvas->removeHighlight( m_currHighlight );

    }
    if( m_ocrProcessDia )
        m_ocrProcessDia->show();
    emit readOnlyEditor( false );
}


/**
 * This slot reads the current line from the member m_ocrCurrLine and
 * writes the corrected wordlist to the member page word lists
 */
void KSANEOCR::slCheckListDone(bool)
{

    /*
     * nothing needs to be updated here in the texts, because it is already done
     * in the slSpellCorrected  slot
     */

    /* Check the dialog state here */
    if( m_spell->dlgResult() == KS_CANCEL ||
        m_spell->dlgResult() == KS_STOP )
    {
        /* stop processing */
        m_spell->cleanUp();
    }
    else
    {
        m_ocrCurrLine++;
	kDebug(28000) << "Starting spellcheck from CheckListDone" << endl;
        startLineSpellCheck();
    }
}

/**
 * updates the word at position spellWordIndx in line line to the new word newWord.
 * The original word was origWord. This slot is called from slSpellCorrected
 *
 */
bool KSANEOCR::slUpdateWord( int line, int spellWordIndx, const QString& origWord,
                             const QString& newWord )
{
    bool result = false;

    if( lineValid( line ))
    {
        ocrWordList words = m_ocrPage[line];
        kDebug(28000) << "Updating word " << origWord << " to " << newWord << endl;

        if( words.updateOCRWord( words[spellWordIndx] /* origWord */, newWord ) )  // searches for the word and updates
        {
            result = true;
            emit updateWord( line, origWord, newWord );
        }
        else
            kDebug(28000) << "WRN: Update from " << origWord << " to " << newWord << " failed" << endl;

    }
    else
    {
        kDebug(28000) << "WRN: Line " << line << " no not valid!" << endl;
    }
    return result;
}


char KSANEOCR::UndetectedChar = '_';

/* -- */
#include "ksaneocr.moc"
