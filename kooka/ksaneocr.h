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


#define CFG_OCR_ENGINE    "ocrEngine"
#define CFG_OCRE_GOCR     "gocr"
#define CFG_OCRE_KADMOS   "kadmos"

/**
  *@author Klaas Freitag
  */

class KOCRBase;
class KookaImage;
class KTempFile;
class QRect;
class QPixmap;

#include "kadmosocr.h"

class KSANEOCR : public QObject
{
    Q_OBJECT
public:
    enum OCREngines{ GOCR, KADMOS };

    KSANEOCR( QWidget*);
    ~KSANEOCR();

    bool startOCRVisible( QWidget* parent=0);

    void finishedOCRVisible( bool );

#ifdef HAVE_KADMOS
    bool startKadmosOCR();
#endif

signals:
    void newOCRResultText( const QString& );
    void clearOCRResultText();
    void newOCRResultPixmap( const QPixmap& );

    /**
     * progress of the ocr process. The first integer is the main progress,
     * the second the sub progress. If there is only on progress, it is the
     * first parameter, the second is always -1 than.
     * Both have a range from 0..100.
     * Note that this signal may not be emitted if the engine does not support
     * progress.
     */
    void ocrProgress(int, int);

public slots:
    void slSetImage( KookaImage* );

    void slLineBox( const QRect& );

protected slots:
    void slotClose ();
    void slotStopOCR();

private slots:

    void slotKadmosResult();
    void startOCRProcess( void );
    void gocrStdIn(KProcess*, char* buffer, int buflen);
    void gocrStdErr(KProcess*, char* buffer, int buflen);
    void gocrExited(KProcess*);


private:
    void     cleanUpFiles( void );


    KOCRBase        *m_ocrProcessDia;
    KProcess        *daemon;
    bool             visibleOCRRunning;
    KTempFile       *m_tmpFile;

    KookaImage      *m_img;
    QString         m_ocrResultText;
    QString         m_ocrResultImage;

    OCREngines      m_ocrEngine;
    QPixmap         m_resPixmap;

#ifdef HAVE_KADMOS
    Kadmos::CRep   m_rep;
#endif
};

#endif
