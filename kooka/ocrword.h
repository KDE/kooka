/***************************************************************************
                      ocrword.h  - ocr-result word and wordlist
                             -------------------
    begin                : Fri 10 Jan 2003
    copyright            : (C) 2003 by Klaas Freitag
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

#ifndef OCRWORD_H
#define OCRWORD_H

#include <qstringlist.h>
#include <qvector.h>
#include <qlist.h>
#include <qrect.h>



/* ==== OcrWord ====================================== */

class OcrWord : public QString
{
public:
    OcrWord(const QString& s);
    OcrWord();
    QStringList getAlternatives() const
        { return m_alternatives; }

    void setAlternatives( const QString& s )
        { m_alternatives.append(s); }

    // QRect boundingRect();

    void setKnode( int k )
        { m_startKnode = k; }
    void setLine( int l )
        { m_line = l; }

    int getLine() const { return m_line; }
    int getKnode() const { return m_startKnode; }

    void setRect( const QRect& r )
        { m_position = r; }
    QRect rect() const
        { return m_position; }

private:
    QStringList m_alternatives;
    int         m_startKnode;
    int         m_line;
    QRect       m_position;
};

/* ==== OcrWordList ====================================== */

/**
 * This represents a line of words in an ocr'ed document
 */
class OcrWordList : public QList<OcrWord>
{
public:
    OcrWordList();
    QStringList stringList();

    bool updateOCRWord( const QString& from, const QString& to );

    bool findFuzzyIndex( const QString& word, OcrWord& resWord );

    QRect wordListRect( ) const;

    void setBlock( int b );
    int block() const { return m_block; }

private:
    int m_block;
};

/**
 * All lines of a block: A value vector containing as much as entries
 * as lines are available in a block. Needs to be resized acordingly.
 */
typedef QVector<OcrWordList> ocrBlock;

/**
 * Blocks taken together form the page.
 * Attention: Needs to be resized to the amount of blocks !!
 */
typedef QVector<ocrBlock> ocrBlockPage;

typedef QVector<QRect> rectList;

#endif							// OCRWORD_H
