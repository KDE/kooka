/***************************************************************************
                      kadmosocr.h - Kadmos cpp interface
                             -------------------
    begin                : Fri Jun 30 2000

    (c) 2002 re Recognition AG      Hafenstrasse 50b  CH-8280 Kreuzlingen
    Switzerland          Phone: +41 (0)71 6780000  Fax: +41 (0)71 6780099
    Website: www.reRecognition.com         E-mail: info@reRecognition.com

    Author: Tamas Nagy (nagy@rerecognition.com)
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

#ifndef __KADMOS_OCR_
#define __KADMOS_OCR_

#include <qobject.h>
#include <qstring.h>

#include "config.h"

#ifdef HAVE_KADMOS
/* class declarations */
class QImage;
class QPixmap;
class QColor;
class QStringList;
class QRect;


class ocrWord;
class ocrWordList;

namespace Kadmos {

/* include files */

#include "kadmos.h"
#include <qptrlist.h>

/* ---------------------------------------- REP ---------------------------------------- */
//! Maximum number of lines in a paragraph
    const int LINE_MAX_LEN  = 100;
    const int GRID_MAX_LEN  =  50;   //!< Maximum number of grid elements in a line
    const int GRAPH_MAX_LEN = 500;   //!< Maximum number of graph elements in a line
    const int CHAR_MAX_LEN  = 500;   //!< Maximum number of characters in a line

    /* Error handling */
    const char CPP_ERROR[] = "Kadmos CPP interface error";

    /* ==== CRep ========================================= */
    class CRep : public QObject
    {
        Q_OBJECT
    public:
        CRep();
        virtual ~CRep();

        RepResult*  getRepResult(int line=0);
        RelGraph*   getGraphKnode(int line, int offset=0);
        RelResult*  getRelResult(int line, Kadmos::RelGraph* graph, int alternative=0);


        /**
           @param ClassifierFilename is a name of a classifier file (*.rec)
        */
        KADMOS_ERROR Init(const char* ClassifierFile);
	
        virtual void run();
        virtual bool finished() { return true; }
        // KADMOS_ERROR Recognize();
        KADMOS_ERROR End();

        /**
          @param Image is an image object
        */
        KADMOS_ERROR SetImage(QImage* Image);
        KADMOS_ERROR SetImage( const QString );
        int GetMaxLine();

        ocrWordList getLineWords( int line );

        const char* RepTextLine(int Line, unsigned char RejectLevel=128,
                                int RejectChar='~', long Format=TEXT_FORMAT_ANSI);

        void analyseLine(int, QPixmap* );
        /** Enable/disable noise reduction
          @param TRUE(enable)/FALSE(disable) noise reduction
        */
        void SetNoiseReduction(bool bNoiseReduction);

        /** Enable/disable scaling (size normalization)
             @param TRUE(enable)/FALSE(disable) scaling (size normalization)
        */
        void SetScaling(bool bScaling);

        /* draw graphic visualiser into the pixmap pointed to */
        virtual void drawLineBox( QPixmap*, const QRect& );
        virtual void drawCharBox( QPixmap*, const QRect& );
        virtual void drawBox( QPixmap*, const QRect&, const QColor& );

        int nextBestWord( int line, int knode, QString& theWord, QRect& brect );

	/* Error text in QString */
	QString getErrorText() const;
	bool    kadmosError();
    private:
        void partStrings( int line, int graphKnode, QString soFar );
        void CheckError();
        
        RepData       m_RepData;
        KADMOS_ERROR  m_Error;
        char m_Line[2*CHAR_MAX_LEN];
        int  m_currLine;
        QStringList m_parts;
        QString m_theWord;
        int m_recurse;

        QChar m_undetectChar;
    };

} /* End of Kadmos namespace */

#endif  /*  HAVE KADMOS */

#endif /* header tagging */
