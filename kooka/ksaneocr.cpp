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
#include <stdio.h>
#include <unistd.h>

#include <img_canvas.h>

#include "kadmosocr.h"
#include "kocrbase.h"
#include "kocrkadmos.h"
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


/*
 * Thread support is disabled here because the kadmos lib seems not to be
 * thread save unfortunately. See slotKadmosResult-comments for more information
 */

KSANEOCR::KSANEOCR( QWidget*, KConfig *cfg ):
    m_ocrProcessDia(0L),
    daemon(0L),
    visibleOCRRunning(false),
    m_imgCanvas(0L),
    m_spell(0L),
    m_wantKSpell(true),
    m_kspellVisible(false),
    m_hideDiaWhileSpellcheck(true),
    m_spellInitialConfig(0L),
    m_parent(0L),
    m_ocrCurrLine(0),
    m_currHighlight(-1)
{
    KConfig *konf = KGlobal::config ();
    m_ocrEngine = GOCR;
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
        konf->setGroup( CFG_OCR_KSPELL );

        int helpi = konf->readNumEntry(CFG_KS_NOROOTAFFIX, -256);
        if( helpi != -256 ) m_spellInitialConfig->setNoRootAffix( (bool) helpi );

        helpi = konf->readNumEntry(CFG_KS_RUNTOGETHER, -256 );
        if( helpi != -256 ) m_spellInitialConfig->setRunTogether( (bool) helpi );

        QString helps = konf->readEntry(CFG_KS_DICTIONARY, "noGlue" );
        if( helps != QString("noGlue") ) m_spellInitialConfig->setDictionary( helps );

        helpi = konf->readNumEntry(CFG_KS_DICTFROMLIST,  -256 );
        if( helpi != -256 ) m_spellInitialConfig->setDictFromList( helpi );

        helpi = konf->readNumEntry(CFG_KS_ENCODING, -256 );
        if( helpi != -256 ) m_spellInitialConfig->setEncoding( helpi );

        helpi = konf->readNumEntry(CFG_KS_CLIENT, -256 );
        if( helpi != 256 ) m_spellInitialConfig->setClient( helpi );

        /* -- ocr dialog information -- */
        konf->setGroup( CFG_GROUP_OCR_DIA );
        QString eng = konf->readEntry(CFG_OCR_ENGINE);
#ifdef HAVE_KADMOS
        if( eng == QString("kadmos") ) m_ocrEngine = KADMOS;
#endif
    }
    kdDebug(28000) << "OCR engine is " << ((m_ocrEngine==KADMOS)?"KADMOS":"GOCR") << endl;
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
   else if( m_ocrEngine == KADMOS )
   {
#ifdef HAVE_KADMOS
/*** Kadmos Engine OCR ***/
       m_ocrProcessDia = new KadmosDialog( parent, m_spellInitialConfig );
#else
       KMessageBox::sorry(0, i18n("This version of Kooka was not compiled with KADMOS support.\n"
           "Please select another OCR engine in Kookas options dialog"));
       kdDebug(28000) << "Sorry, this version of Kooka has no KADMOS support" << endl;
#endif /* HAVE_KADMOS */
   }
   else
   {
      kdDebug(28000) << "ERR Unknown OCR engine requested!" << endl;
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
   visibleOCRRunning =  false;
   cleanUpFiles();
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

       // emit newOCRResultPixmap( m_resPixmap );

       if( m_imgCanvas )
       {
           /* The image canvas is non-zero. Set it to our image */
           m_imgCanvas->newImage( m_img );
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
   }

   kdDebug(28000) << "# ocr finished #" << endl;
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
        }

        kdDebug(28000)<< "Wortliste: "<< m_checkStrings.join(", ") << endl;

        // if( list.count() > 0 )
        m_spell->checkList( &m_checkStrings, m_kspellVisible );

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
        kdDebug(28000) << k_funcinfo <<" -- no more lines !" << endl;
        m_spell->cleanUp();
    }
}



/* User Cancel is called when the user does not really start the
 * ocr but uses the cancel-Button to come out of the Dialog */
void KSANEOCR::slotClose()
{
    kdDebug(28000) << "closing ocr Dialog" << endl;
   if( daemon && daemon->isRunning() )
   {
      kdDebug(28000) << "Still running - Killing daemon with Sig. 9" << endl;
      daemon->kill(9);
   }
   finishedOCRVisible(false);
}

void KSANEOCR::slotStopOCR()
{
    kdDebug(28000) << "closing ocr Dialog" << endl;
    if( daemon && daemon->isRunning() )
    {
        kdDebug(28000) << "Killing daemon with Sig. 9" << endl;
        daemon->kill(9);
        // that leads to the process being destroyed.
        KMessageBox::error(0, i18n("The OCR-Process was stopped !") );
    }

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

       m_tmpFile = new KTempFile( QString(), "."+format.lower());
       m_tmpFile->setAutoDelete( false );
       m_tmpFile->close();

       QString tmpFile = m_tmpFile->name();

       m_img->save( tmpFile, format.latin1() );

       kdDebug(28000) <<  "Starting GOCR-Command: " << cmd << " on file " << tmpFile << ", format " << format << endl;

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
           kdDebug(28000) <<  "Error starting daemon!" << endl;
       }
       else
       {
           kdDebug(28000) << "Start OK" << endl;

       }
   }
#ifdef HAVE_KADMOS
   else
   {
       KadmosDialog *kadDia = static_cast<KadmosDialog*>(m_ocrProcessDia);

       kdDebug(28000)  << "Starting Kadmos OCR Engine" << endl;

       QString clasPath;  /* target where the clasPath is written in */
       if( ! kadDia->getSelClassifier( clasPath ) )
       {
           KMessageBox::error( m_parent,
                               i18n("The classifier file neccessary for OCR is can not be loaded: %1\n"
                                   "OCR with the KADMOS engine is not possible" ).
                               arg(clasPath), i18n("KADMOS Installation Problem"));
           finishedOCRVisible(false);
           return;
       }
       QCString c = clasPath.latin1();

       kdDebug(28000) << "Using classifier " << c << endl;
       m_rep.Init( c );
       m_rep.SetNoiseReduction( kadDia->getNoiseReduction() );
       m_rep.SetScaling( kadDia->getAutoScale() );
#if 0
       KookaImage img;
       img.load( tmpFile );

       kdDebug(28000) << "Filename of tmp: " << tmpFile << endl;
       if( img.depth() > 8 ) {
           QImage tmpImg( img.convertDepth(8));
           img = tmpImg;
           if( img.numColors() == 2 )
               img = tmpImg.convertDepth(1);
       }
#endif
       kdDebug(28000) << "Image size " << m_img->width() << " x " << m_img->height() << endl;
       kdDebug(28000) << "Image depth " << m_img->depth() << ", colors: " << m_img->numColors() << endl;
#define USE_KADMOS_FILEOP /* use a save-file for OCR instead of filling the reImage struct manually */
#ifdef USE_KADMOS_FILEOP
       m_tmpFile = new KTempFile( QString(), QString("bmp"));
       m_tmpFile->setAutoDelete( false );
       m_tmpFile->close();
       QString tmpFile = m_tmpFile->name();
       kdDebug() << "Saving to file " << tmpFile << endl;
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
        */

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
    kdDebug(28000) << "check for Recognition finished" << endl;


    if( m_rep.finished() )
    {
        /* The recognition thread is finished. */
        kdDebug(28000) << "kadmos is finished." << endl;

        m_ocrResultText = "";
        int lines = m_rep.GetMaxLine();
        kdDebug(28000) << "Count lines: " << lines << endl;
        m_ocrPage.clear();
        m_ocrPage.resize( lines );

        for( int line = 0; line < m_rep.GetMaxLine(); line++ )
        {
            // ocrWordList wordList = m_rep.getLineWords(line);
            /* call an ocr engine independent method to use the spellbook */
            ocrWordList words = m_rep.getLineWords(line);
            kdDebug(28000) << "Have " << words.count() << " entries in list" << endl;
            m_ocrPage[line]=words;
        }

        /* show results of ocr */
        finishedOCRVisible(true);

        m_rep.End();
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
   kdDebug(28000) << "daemonExited start !" << endl;

   /* Now all the text of gocr is in the member m_ocrResultText. This one must
    * be split up now to m_ocrPage. First break up the lines, resize m_ocrPage
    * accordingly and than go through every line and create ocrwords for every
    * word.
    */
   QStringList lines = QStringList::split( '\n', m_ocrResultText, true );

   m_ocrPage.clear();
   m_ocrPage.resize( lines.count() );

   kdDebug(28000) << "RESULT " << m_ocrResultText << " was splitted to lines " << lines.count() << endl;

   unsigned int lineCnt = 0;

   for ( QStringList::Iterator it = lines.begin(); it != lines.end(); ++it )
   {
       kdDebug(28000) << "Splitting up line " << *it << endl;
       ocrWordList ocrLine;

       QStringList words = QStringList::split( QRegExp( "\\s+" ), *it, false );
       for ( QStringList::Iterator itWord = words.begin(); itWord != words.end(); ++itWord )
       {
           kdDebug(28000) << "Appending to results: " << *itWord << endl;
           ocrLine.append( ocrWord( *itWord ));
       }
       m_ocrPage[lineCnt] = ocrLine;
       lineCnt++;
   }
   kdDebug(28000) << "Finished to split!" << endl;
   /* set the result pixmap to the result pix of gocr */
   if( ! m_resPixmap.load( m_ocrResultImage ) )
   {
       kdDebug(28000) << "Can not load result image!" << endl;
   }

   /* load the gocr result image */
   if( m_img ) delete m_img;
   m_img = new KookaImage();
   m_img->load( "out30.bmp" );

   finishedOCRVisible( d->normalExit() );
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
      kdDebug(28000) << "Unlinking OCR Result image file!" << endl;
      unlink(QFile::encodeName(m_ocrResultImage));
      m_ocrResultImage = "";
   }

   /* Delete the debug images of gocr ;) */
   unlink( "out20.bmp" );
}


void KSANEOCR::gocrStdErr(KProcess*, char* buffer, int buflen)
{
   QString errorBuffer = QString::fromLocal8Bit(buffer, buflen);
   kdDebug(28000) << "gocr says: " << errorBuffer << endl;

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
        kdDebug(28000) << "Emitting progress: " << progress << endl;
        emit ocrProgress( progress, subProgress );
    }
    else
    {
        m_ocrResultText += aux;
    }

    // kdDebug(28000) << aux << endl;

}

/*
 * Assemble the result text
 */
QString KSANEOCR::ocrResultText()
{
    QString res;
    const QString space(" ");

    /* start from the back and search the original word to replace it */
    QValueVector<ocrWordList>::iterator pageIt;

    for( pageIt = m_ocrPage.begin(); pageIt != m_ocrPage.end(); ++pageIt )
    {
        /* thats goes over all lines */
        QValueList<ocrWord>::iterator lineIt;
        for( lineIt = (*pageIt).begin(); lineIt != (*pageIt).end(); ++lineIt )
        {
            res += space + *lineIt;
        }
        res += "\n";
    }
    kdDebug(28000) << "Returning result String  " << res << endl;
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

    // if( m_imgCanvas && w == m_imgCanvas )
    {
        if( event->type() == QEvent::MouseButtonDblClick )
        {
            QMouseEvent *mev = static_cast<QMouseEvent*>(event);
            int x = mev->x();
            int y = mev->y();

            // kdDebug(28000) << "Clicked to " << x << "/" << y << endl;
            /* now search the word that was clicked on */
            QValueVector<ocrWordList>::iterator pageIt;

            for( pageIt = m_ocrPage.begin(); pageIt != m_ocrPage.end(); ++pageIt )
            {
                QRect r = (*pageIt).wordListRect();
                int line = 0;

                if( y > r.top() && y < r.bottom() )
                {
                    kdDebug(28000)<< "It is in between, line " << line << endl;
                    line++;
                }
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
    kdDebug(28000) << "Corrected: Original Word " << originalword << " was corrected to "
                   << newword << ", pos ist " << pos << endl;

    kdDebug(28000) << "Dialog state is " << m_spell->dlgResult() << endl;

    if( slUpdateWord( m_ocrCurrLine, pos, originalword, newword ) )
    {
        if( m_imgCanvas && m_currHighlight > -1 )
        {
            m_imgCanvas->removeHighlight( m_currHighlight );
        }
        else
        {
            kdDebug(28000) << "No highlighting to remove!" << endl;
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
            QPen pen( gray, 1 );
            QRect r = ignoreOCRWord.rect();
            r.moveBy(0,2);  // a bit offset to the top
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
    kdDebug(28000) << "Misspelled: " << originalword << " at position " << pos << endl;

    int line = m_ocrCurrLine;
    m_currHighlight = -1;

    ocrWord resWord = ocrWordFromKSpellWord( line, originalword );
    if( ! resWord.isEmpty() )
    {
        QBrush brush;
        brush.setColor( QColor(red)); // , "Dense4Pattern" );
        brush.setStyle( Qt::Dense4Pattern );
        QPen pen( red, 2 );
        QRect r = resWord.rect();
        r.moveBy(0,2);  // a bit offset to the top
        m_currHighlight = m_imgCanvas->highlight( r, pen, brush, true );

        kdDebug(28000) << "Position ist " << r.x() << ", " << r.y() << ", width: " << r.width() << ", height: " << r.height() << endl;

        /* draw a line under the word to check */

        /* copy the source */
        emit repaintOCRResImage();
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

    kdDebug(28000) << "Spellcheck available" << endl;

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
        kdDebug(28000) << "Spellcheck NOT available" << endl;
        /* Spellchecking has not yet been existing, thus there is a base problem with
         * spellcheck on this system.
         */
        KMessageBox::error( m_parent,
                            i18n("Spellchecking can not be started on this system.\n"
                                 "Please check the configuration" ),
                            i18n("Spellcheck") );

    }
    else
    {
        if( m_spell->status() == KSpell::Cleaning )
        {
            kdDebug(28000) << "KSpell cleans up" << endl;
        }
        else if( m_spell->status() == KSpell::Finished )
        {
            kdDebug(28000) << "KSpell finished" << endl;
        }
        else if( m_spell->status() == KSpell::Error )
        {
            kdDebug(28000) << "KSpell finished with Errors" << endl;
        }
        else if( m_spell->status() == KSpell::Crashed )
        {
            kdDebug(28000) << "KSpell Chrashed" << endl;
        }
        else
        {
            kdDebug(28000) << "KSpell finished with unknown state!" << endl;
        }

        /* save the current config */
        slSaveSpellCfg();
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
        startLineSpellCheck();
    }
}

/**
 * updates the word at position spellWordIndx in line line to the new word newWord.
 * The original word was origWord. This slot is called from slSpellCorrected
 *
 */
bool KSANEOCR::slUpdateWord( int line, int /* spellWordIndx */, const QString& origWord,
                             const QString& newWord )
{
    bool result = false;

    if( lineValid( line ))
    {
        ocrWordList words = m_ocrPage[line];
        kdDebug(28000) << "Updating word " << origWord << " to " << newWord << endl;

        if( words.updateOCRWord( origWord, newWord ) )  // searches for the word and updates
        {
            result = true;
            emit updateWord( line, origWord, newWord );
        }
        else
            kdDebug(28000) << "WRN: Update from " << origWord << " to " << newWord << " failed" << endl;

    }
    else
    {
        kdDebug(28000) << "WRN: Line " << line << " no not valid!" << endl;
    }
    return result;
}


void KSANEOCR::slSaveSpellCfg( )
{
    if( ! m_spell ) return;

    KSpellConfig ksCfg = m_spell->ksConfig();

    KConfig *konf = KGlobal::config ();
    KConfigGroupSaver gs( konf, CFG_OCR_KSPELL );

    konf->writeEntry (CFG_KS_NOROOTAFFIX,   (int) ksCfg.noRootAffix (), true, false);
    konf->writeEntry (CFG_KS_RUNTOGETHER,   (int) ksCfg.runTogether (), true, false);
    konf->writeEntry (CFG_KS_DICTIONARY,          ksCfg.dictionary (),  true, false);
    konf->writeEntry (CFG_KS_DICTFROMLIST,  (int) ksCfg.dictFromList(), true, false);
    konf->writeEntry (CFG_KS_ENCODING,      (int) ksCfg.encoding(),     true, false);
    konf->writeEntry (CFG_KS_CLIENT,              ksCfg.client(),       true, false);
    konf->sync();


}


/* -- */
#include "ksaneocr.moc"
