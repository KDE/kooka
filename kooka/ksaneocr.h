/***************************************************************************
                          ksaneocr.h  - ocr-engine class
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


#ifndef KSANEOCR_H
#define KSANEOCR_H
#include <qwidget.h>
#include <qobject.h>
#include <qimage.h>

#include <ktempfile.h>
#include <kprocess.h>

/**
  *@author Klaas Freitag
  */

class KOCRStartDialog;


class KSANEOCR : public QObject
{
   Q_OBJECT
public: 
   KSANEOCR( QWidget* );
   ~KSANEOCR();

   bool startExternOcrVisible( void );
   void setImage( const QImage* );

private slots:
   void startOCRProcess( void );
   
   void msgRcvd(KProcess*, char* buffer, int buflen);
   void errMsgRcvd(KProcess*, char* buffer, int buflen);
   void daemonExited(KProcess*);
   void userCancel( void );

private:
   void     cleanUpFiles( void );
   
   bool     visibleOCRRunning;
   KOCRStartDialog *ocrProcessDia;
   KProcess *daemon;
   QString  tmpFile;
   KTempFile *ktmpFile;
   
   QImage   *bw_img;
   QString  ocrResultText;
   QString  ocrResultImage;
   QWidget  *parent;
};

#endif
