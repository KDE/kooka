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

#include <kprocess.h>

#define CFG_GROUP_OCR_DIA "ocrDialog"
#define CFG_OCR_ENGINE    "ocrEngine"
#define CFG_OCRE_GOCR     "gocr"
#define CFG_OCRE_KADMOS   "kadmos"

/**
  *@author Klaas Freitag
  */

class KOCRBase;
class KookaImage;
class KTempFile;

#include "kadmosocr.h"

class KSANEOCR : public QObject
{
    Q_OBJECT
public:
    enum OCREngines{ GOCR, KADMOS };

    KSANEOCR( QWidget*);
    ~KSANEOCR();

    bool startExternOcrVisible( QWidget* parent=0);

#ifdef HAVE_KADMOS
    bool startKadmosOCR();
#endif

signals:
    void newOCRResultText( const QString& );
    void clearOCRResultText();

public slots:
    void slSetImage( KookaImage* );

private slots:

    void slotKadmosResult();
    void startOCRProcess( void );
    void msgRcvd(KProcess*, char* buffer, int buflen);
    void errMsgRcvd(KProcess*, char* buffer, int buflen);
    void daemonExited(KProcess*);
    void userCancel( void );

private:
    void     cleanUpFiles( void );


    KOCRBase        *m_ocrProcessDia;
    KProcess        *daemon;
    bool             visibleOCRRunning;
    KTempFile       *ktmpFile;

    KookaImage      *m_img;
    QString         m_ocrResultText;
    QString         m_ocrResultImage;

    OCREngines      m_ocrEngine;

#ifdef HAVE_KADMOS
    Kadmos::CRep   m_rep;
#endif
};

#endif
