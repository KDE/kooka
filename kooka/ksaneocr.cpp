/***************************************************************************
                          ksaneocr.cpp  -  generic ocr
                             -------------------
    begin                : Fri Jun 30 2000
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

/* $Id$ */

#include <kdebug.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <kapplication.h>
#include <ktempfile.h>
#include <stdlib.h>
#include <qfile.h>
#include <qcolor.h>
#include <stdio.h>
#include <unistd.h>

#include "kadmosocr.h"
#include "kocrbase.h"
#include "kocrkadmos.h"
#include "config.h"
#include "ksaneocr.h"
#include "kocrgocr.h"
#include "kookaimage.h"
#include "kookapref.h"
#include <qtimer.h>
#include <qregexp.h>

/*
 * Thread support is disabled here because the kadmos lib seems not to be
 * thread save unfortunately. See slotKadmosResult-comments for more information
 */
#ifdef HAVE_KADMOS
#undef QT_THREAD_SUPPORT
#endif

KSANEOCR::KSANEOCR( QWidget* ):
   m_ocrProcessDia(0),
   daemon(0),
   visibleOCRRunning(false)
{
   KConfig *konf = KGlobal::config ();
   m_ocrEngine = GOCR;
   m_img = 0L;
   m_tmpFile = 0L;

   if( konf )
   {
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

   if( m_ocrEngine == GOCR )
   {
       m_ocrProcessDia = new KGOCRDialog ( parent );
   }
   else if( m_ocrEngine == KADMOS )
   {
#ifdef HAVE_KADMOS
/*** Kadmos Engine OCR ***/
       m_ocrProcessDia = new KadmosDialog( parent );
#else
       KMessageBox::sorry(0, "This version of Kooka was not compiled with KADMOS support.\n"
           "Please select another OCR engine in Kookas options dialog");
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

   if( m_ocrProcessDia )
   {
       m_ocrProcessDia->stopOCR();
   }

   if( success )
   {
       emit newOCRResultText( m_ocrResultText );
       emit newOCRResultPixmap( m_resPixmap );
   }

   kdDebug(28000) << "# ocr finished #" << endl;

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
        KMessageBox::error(0, "The OCR-Process was stopped !" );
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
       m_tmpFile = new KTempFile( QString(), QString(".ppm"));
       m_tmpFile->setAutoDelete( false );
       m_tmpFile->close();
       QString tmpFile = m_tmpFile->name();

       kdDebug(28000) <<  "Starting GOCR-Command: " << cmd << " " << tmpFile << endl;
       m_img->save( tmpFile, "PPM" );

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

       QCString c = kadDia->getSelClassifier().latin1();
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
       m_rep.start();

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
        m_resPixmap.convertFromImage( *m_img );
        /* The recognition thread is finished. */
        kdDebug(28000) << "kadmos is finished." << endl;

        m_ocrResultText = "";

        for( int i = 0; i < m_rep.GetMaxLine(); i++ )
        {
            QString l = QString::fromLocal8Bit( m_rep.RepTextLine( i, 256, '_', TEXT_FORMAT_ANSI ));
            kdDebug(28000) << "| " << l << endl;
            m_rep.analyseLine( i, &m_resPixmap );
            m_ocrResultText += l+"\n";
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

   /* set the result pixmap to the result pix of gocr */
   if( ! m_resPixmap.load( m_ocrResultImage ) )
   {
       kdDebug(28000) << "Can not load result image!" << endl;
   }
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

    QRegExp rx( "^\\s*\\d+\\s+\\d+$");
    if( m_ocrEngine == GOCR && rx.search( aux ) > -1 )
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



/* -- */
#include "ksaneocr.moc"
