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
#include <stdlib.h>
#include <qcolor.h>
#include <stdio.h>
#include <unistd.h>

#include "kocrstartdia.h"
#include "kocrfindia.h"
#include "config.h"
#include "ksaneocr.h"


KSANEOCR::KSANEOCR( QWidget *pa )
{
   parent = pa;
   ocrProcessDia = 0;
   daemon = 0L;
   tmpFile = "";
   visibleOCRRunning = false;
}


KSANEOCR::~KSANEOCR()
{
   if( daemon  ) delete( daemon );
}

void KSANEOCR::setImage( const QImage *img )
{
   if( ! img ) return;
    // create temporar name for the saved file
    tmpFile = QString( tmpnam( 0L ));
    debug( "save the image to %s", (const char*) tmpFile );

    // converting the incoming image
    if( img->depth()==32 )
       img->convertDepth( 8, MonoOnly ).save( tmpFile, "PBM" );
    else if (img->depth() == 1)
        img->convertDepth( 8, MonoOnly ).save( tmpFile, "PBM");
    else
       img->save( tmpFile, "PBM" );
}

bool KSANEOCR::startExternOcrVisible( void )
{
   if( visibleOCRRunning ) return( false );
   
   ocrProcessDia = new KOCRStartDialog ( parent );
   CHECK_PTR( ocrProcessDia );
   visibleOCRRunning = true;
   
   connect( ocrProcessDia, SIGNAL( user1Clicked()), this, SLOT( startOCRProcess() ));
   connect( ocrProcessDia, SIGNAL( cancelClicked()), this, SLOT( userCancel() ));
   connect( ocrProcessDia, SIGNAL( cancelClicked()),
	    ocrProcessDia, SLOT( stopAnimation() ));
   
   ocrProcessDia->exec();
   return( true );
}

/* User Cancel is called when the user does not really start the
 * ocr but uses the cancel-Button to come out of the Dialog */
void KSANEOCR::userCancel( void )
{
   debug( "+++++++++++++++++++ *************************" );
   if( daemon && daemon->isRunning() )
   {
      debug( "Killing daemon with Sig. 9" );
      daemon->kill(9);
      // that leads to the process being destroyed.
      KMessageBox::error(0, "The OCR-Process was killed !" );
   }
   visibleOCRRunning =  false;
   cleanUpFiles();
}


void KSANEOCR::startOCRProcess( void )
{
   if( ! ocrProcessDia ) return;

   const QString cmd = ocrProcessDia->getOCRCmd();
   kdDebug () <<  "Starting OCR-Command: " << cmd.latin1() << " " << tmpFile.latin1()  << endl;

   if( daemon ) delete( daemon );
   
   daemon = new KProcess;
   CHECK_PTR(daemon);
   ocrResultText = "";
   
   connect(daemon, SIGNAL(processExited(KProcess *)),
	   this, SLOT(daemonExited(KProcess*)));
   connect(daemon, SIGNAL(receivedStdout(KProcess *, char*, int)),
	   this, SLOT(msgRcvd(KProcess*, char*, int)));
#if 0
   connect(daemon, SIGNAL(receivedStderr(KProcess *, char*, int)),
	   this, SLOT(errMsgRcvd(KProcess*, char*, int)));
#endif
   QString opt;
   *daemon << cmd.latin1();
   *daemon << "-l";
   opt.setNum(ocrProcessDia->getGraylevel());
   *daemon << opt;
   *daemon << "-s";
   opt.setNum(ocrProcessDia->getSpaceWidth());
   *daemon << opt;
   *daemon << "-d";
   opt.setNum(ocrProcessDia->getDustsize());
   *daemon << opt;

   // Write an result image 
   *daemon << "-v";
   *daemon << "32";

   // Unfortunately this is fixed by gocr.
   ocrResultImage = "out30.bmp";
   
   *daemon << tmpFile.latin1();
   
   if (!daemon->start(KProcess::NotifyOnExit, KProcess::All))
   {
      kdDebug() <<  "Error starting daemon!" << endl;
   }
   else
   {
      kdDebug () << "Start OK" << endl;
   }
}



void KSANEOCR::daemonExited(KProcess* d)
{
   debug( "daemonExited start !" );


   if( d->normalExit() )
   {
      /* Only on normal Exit, bcause it is destroyed on
       * nonregular exit by itself or whomever... */
      if( ocrProcessDia )
      {
	 ocrProcessDia->stopAnimation();
	 delete ocrProcessDia;
	 ocrProcessDia = 0L;
      }
   

      /* Now ocr is finished, open the result */
      KOCRFinalDialog fin( parent, ocrResultImage );

      fin.fillText( ocrResultText );
      fin.exec();
   }

   /* Clean up */
   
   visibleOCRRunning = false;
   cleanUpFiles();
   
   kdDebug () << "# ocr exited #" << endl;
}


void KSANEOCR::cleanUpFiles( void )
{
   if( ! tmpFile.isEmpty())
   {
      debug("Unlinking file to OCR!");
      unlink( (const char*) tmpFile );
      tmpFile = "";
   }

   if( ! ocrResultImage.isEmpty())
   {
      debug("Unlinking OCR Result image file!");
      unlink( (const char*) ocrResultImage);
      ocrResultImage = "";
   }

   /* Delete the debug images of gocr ;) */
   unlink( "out20.bmp" );
}


void KSANEOCR::errMsgRcvd(KProcess*, char* buffer, int buflen)
{
   QString errorBuffer = QString::fromLocal8Bit(buffer, buflen);
   kdDebug () << "ERR: " << errorBuffer << endl;

}


void KSANEOCR::msgRcvd(KProcess*, char* buffer, int buflen)
{
   QString aux = QString::fromLocal8Bit(buffer, buflen);
   ocrResultText += aux;
   
   kdDebug()  << aux;

}

/* -- */
