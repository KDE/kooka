/***************************************************************************
                          ksaneocr.cpp  -  description
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
#include <qtimer.h>


#undef QT_THREAD_SUPPORT


KSANEOCR::KSANEOCR( QWidget* ):
   m_ocrProcessDia(0),
   daemon(0),
   visibleOCRRunning(false)
{
   KConfig *konf = KGlobal::config ();
   m_ocrEngine = GOCR;
   m_img = 0L;

   if( konf )
   {
      konf->setGroup( CFG_GROUP_OCR_DIA );
      QString eng = konf->readEntry(CFG_OCR_ENGINE);
      if( eng == QString("kadmos") ) m_ocrEngine = KADMOS;
   }
   kdDebug(28000) << "OCR engine is " << ((m_ocrEngine==KADMOS)?"KADMOS":"GOCR") << endl;
}


KSANEOCR::~KSANEOCR()
{
   if( daemon  ) {
      delete( daemon );
      daemon = 0;
   }
   if ( ktmpFile )
   {
       ktmpFile->setAutoDelete( true );
       delete ktmpFile;
   }

   if( m_img ) delete m_img;
}

void KSANEOCR::slSetImage(KookaImage *img )
{
   if( ! img ) return           ;
    // create temporar name for the saved file
   if( m_img )
       delete m_img;

   m_img = new KookaImage(*img);

}

bool KSANEOCR::startExternOcrVisible( QWidget *parent )
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
       kdDebug(28000) << "Have no kadmos support compiled in" << endl;
#endif /* HAVE_KADMOS */
   }
   else
   {
      kdDebug(28000) << "ERR Unknown OCR engine requested!" << endl;
   }

   if( m_ocrProcessDia )
   {
       m_ocrProcessDia->setupGui();
       m_ocrProcessDia->introduceImage( m_img );
       visibleOCRRunning = true;

      connect( m_ocrProcessDia, SIGNAL( user1Clicked()), this, SLOT( startOCRProcess() ));
      connect( m_ocrProcessDia, SIGNAL( cancelClicked()), this, SLOT( userCancel() ));
      connect( m_ocrProcessDia, SIGNAL( cancelClicked()),
	       m_ocrProcessDia, SLOT( stopAnimation() ));
      m_ocrProcessDia->show();
   }
   return( res );
}

/* User Cancel is called when the user does not really start the
 * ocr but uses the cancel-Button to come out of the Dialog */
void KSANEOCR::userCancel( void )
{
   kdDebug(28000) << "+++++++++++++++++++ *************************" << endl;
   if( daemon && daemon->isRunning() )
   {
      kdDebug(28000) << "Killing daemon with Sig. 9" << endl;
      daemon->kill(9);
      // that leads to the process being destroyed.
      KMessageBox::error(0, "The OCR-Process was killed !" );
   }
   visibleOCRRunning =  false;
   cleanUpFiles();
}


void KSANEOCR::startOCRProcess( void )
{
   if( ! m_ocrProcessDia ) return;

   m_ocrProcessDia->enableFields(false);
   kapp->processEvents();
   if( m_ocrEngine == GOCR )
   {
       KGOCRDialog *gocrDia = static_cast<KGOCRDialog*>(m_ocrProcessDia);

       const QCString cmd = gocrDia->getOCRCmd();

       /* Save the image to a temp file */
       ktmpFile = new KTempFile( QString(), QString(".bmp"));
       ktmpFile->setAutoDelete( false );
       ktmpFile->close();
       QString tmpFile = ktmpFile->name();

       kdDebug(28000) <<  "Starting GOCR-Command: " << cmd << " " << tmpFile << endl;
       m_img->save( tmpFile, "BMP" );

       if( daemon ) {
           delete( daemon );
           daemon = 0;
       }

       daemon = new KProcess;
       Q_CHECK_PTR(daemon);
       m_ocrResultText = "";

       connect(daemon, SIGNAL(processExited(KProcess *)),
               this, SLOT(daemonExited(KProcess*)));
       connect(daemon, SIGNAL(receivedStdout(KProcess *, char*, int)),
               this, SLOT(msgRcvd(KProcess*, char*, int)));
       connect(daemon, SIGNAL(receivedStderr(KProcess *, char*, int)),
               this, SLOT(errMsgRcvd(KProcess*, char*, int)));

       QString opt;
       *daemon << QFile::encodeName(cmd);
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
       KTempFile *ktmpFile = new KTempFile( QString(), QString("bmp"));
       ktmpFile->setAutoDelete( false );
       ktmpFile->close();
       QString tmpFile = ktmpFile->name();
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

       if( ktmpFile )
           delete ktmpFile;
#ifdef QT_THREAD_SUPPORT
       /* start a timer and wait until it fires. */
       QTimer::singleShot( 500, this, SLOT( slotKadmosResult() ));
#else
       slotKadmosResult();
#endif

   }
#endif /* HAVE_KADMOS */
}
void KSANEOCR::slotKadmosResult()
{
#ifdef HAVE_KADMOS
    kdDebug(28000) << "check for Recognition finished" << endl;

    if( m_rep.finished() )
    {
        /* The recognition thread is finished. */
        kdDebug(28000) << "YEAH - its finished" << endl;

        m_ocrResultText = "";

        for( int i = 0; i < m_rep.GetMaxLine(); i++ )
        {
            QString l = QString::fromLocal8Bit( m_rep.RepTextLine( i, 256, '_', TEXT_FORMAT_ANSI ));
            kdDebug(28000) << "| " << l << endl;
            m_rep.analyseLine(i);
            m_ocrResultText += l+"\n";
        }

        /* Text analyse */
        // m_rep.analyseGraph();

        m_rep.End();
        m_ocrProcessDia->enableFields(true);
        emit newOCRResultText( m_ocrResultText );
    }
    else
    {
        /* recognition thread is not yet finished. Wait another half a second. */
        QTimer::singleShot( 500, this, SLOT( slotKadmosResult() ));
        /* Never comes here if no threads exist on the system */
    }
#endif /* HAVE_KADMOS */
}



void KSANEOCR::daemonExited(KProcess* d)
{
   kdDebug(28000) << "daemonExited start !" << endl;


   if( d->normalExit() )
   {
      /* Only on normal Exit, bcause it is destroyed on
       * nonregular exit by itself or whomever... */
      if( m_ocrProcessDia )
      {
	 m_ocrProcessDia->stopAnimation();
	 delete m_ocrProcessDia;
	 m_ocrProcessDia = 0L;
      }


      /* Now ocr is finished, open the result */
      emit newOCRResultText( m_ocrResultText );
   }

   /* Clean up */

   visibleOCRRunning = false;
   cleanUpFiles();

   kdDebug(28000) << "# ocr exited #" << endl;
}


void KSANEOCR::cleanUpFiles( void )
{
    if( ktmpFile )
    {
        delete ktmpFile;
        ktmpFile = 0L;
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


void KSANEOCR::errMsgRcvd(KProcess*, char* buffer, int buflen)
{
   QString errorBuffer = QString::fromLocal8Bit(buffer, buflen);
   kdDebug(28000) << "gocr says: " << errorBuffer << endl;

}


void KSANEOCR::msgRcvd(KProcess*, char* buffer, int buflen)
{
   QString aux = QString::fromLocal8Bit(buffer, buflen);
   m_ocrResultText += aux;

   // kdDebug(28000) << aux << endl;

}



/* -- */
#include "ksaneocr.moc"
