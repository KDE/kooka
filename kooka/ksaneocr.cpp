/***************************************************************************
                          ksaneocr.cpp  -  description                              
                             -------------------                                         
    begin                : Fri Jun 30 2000                                           
    copyright            : (C) 2000 by Klaas Freitag                         
    email                : kooka@suse.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   * 
 *                                                                         *
 ***************************************************************************/
#include <kdebug.h>
#include <kmessagebox.h>
#include <stdlib.h>
#include <qcolor.h>


#include "kocrstartdia.h"
#include "kocrfindia.h"
#include "config.h"

#ifdef HAVE_LIBPGM2ASC
#include "gocr/gocr.h"
#include "gocr/pnm.h"

#include "gocr/pgm2asc.h"
#endif /* libPgm2asc */

#include "ksaneocr.h"


KSANEOCR::KSANEOCR( const QImage *img)
{
    if( !img ) return;
    ocrProcessDia = 0;
       
    bw_img = new QImage();

    if( img->depth()==32 )
        *bw_img = img->convertDepth( 8, MonoOnly );
    else if (img->depth() == 1)
        *bw_img = img->convertDepth( 8, MonoOnly );
    else
        *bw_img = *img;

    daemon = 0L;
    // Sorry, very early construction...
    tmpFile = "/tmp/img.pnm";
    bw_img->save( tmpFile, "PBM" );
    
    
}

KSANEOCR::~KSANEOCR()
{
   delete bw_img;
}

void KSANEOCR::startExternOcr( void )
{
   ocrProcessDia = new KOCRStartDialog ( 0L, bw_img );
   CHECK_PTR( ocrProcessDia );
   connect( ocrProcessDia, SIGNAL( user1Clicked()), this, SLOT( startOCRProcess() ));
   connect( ocrProcessDia, SIGNAL( cancelClicked()), this, SLOT( userCancel() ));

   ocrProcessDia->exec();
}


void KSANEOCR::userCancel( void )
{
   debug( "+++++++++++++++++++ *************************" );
   if( daemon && daemon->isRunning() )
   {
      daemon->normalExit();
      // that leads to the process being destroyed.
      KMessageBox::error(0, "The OCR-Process was killed !" );
   }
}


void KSANEOCR::startOCRProcess( void )
{
   if( ! ocrProcessDia ) return;

   const QString cmd = ocrProcessDia->getOCRCmd();
   kdDebug () <<  "Starting OCR-Command: " << cmd.latin1() << " " << tmpFile.latin1()  << endl;

   daemon = new KProcess;
   CHECK_PTR(daemon);
   ocrResultText = "";
   
   connect(daemon, SIGNAL(processExited(KProcess *)),
	   this, SLOT(daemonExited(KProcess*)));
   connect(daemon, SIGNAL(receivedStdout(KProcess *, char*, int)),
	   this, SLOT(msgRcvd(KProcess*, char*, int)));
   connect(daemon, SIGNAL(receivedStderr(KProcess *, char*, int)),
	   this, SLOT(errMsgRcvd(KProcess*, char*, int)));

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
   if( d )
   {
      delete d;
   }

   if( ocrProcessDia )
   {
      delete( ocrProcessDia );
      ocrProcessDia = 0L;
   }

   KOCRFinalDialog fin( 0L, ocrResultImage );

   fin.fillText( ocrResultText );
   fin.exec();
   /* Now ocr is finished, open the result */
   
   kdDebug () << "ocr exited" << endl;
}


void KSANEOCR::errMsgRcvd(KProcess*, char* buffer, int buflen)
{
   QString errorBuffer = QString::fromLocal8Bit(buffer, buflen);
   kdDebug () << "ERR: " << errorBuffer << endl;

   // KMessageBox::error(0, "Kooka detected an error after starting ocr!" );
}


void KSANEOCR::msgRcvd(KProcess*, char* buffer, int buflen)
{
   QString aux = QString::fromLocal8Bit(buffer, buflen);
   ocrResultText += aux;
   
   kdDebug()  << aux;

}

/* -- */
