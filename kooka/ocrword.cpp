/***************************************************************************
                   ocrword.cpp  - ocr-result word and wordlist
                             -------------------
    begin                : Fri Jan 10 2003
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

#include "ocrword.h"

#include <qstring.h>
#include <qrect.h>
#include <qlist.h>
#include <qregexp.h>

#include <kdebug.h>

/* -------------------- OcrWord -------------------- */

OcrWord::OcrWord(const QString &s)
    : QString(s)
{
}

OcrWord::OcrWord()
    : QString()
{
}

#if 0
QRect OcrWord::boundingRect()
{
    QRect r;

    return r;
}
#endif

/* -------------------- OcrWordList ------------------ */
OcrWordList::OcrWordList()
    : QList<OcrWord>(),
      m_block(0)
{
    // setAutoDelete( true );
}


QStringList OcrWordList::stringList()
{
    QStringList res;
    QRegExp rx("[,\\.-]");
    OcrWordList::iterator it;

    for ( it = begin(); it != end(); ++it )
    {
#if 0
        /* Uncommented this to prevent an error that occurs if the lenght of the
         * spellchecked stringlist and the ocr_page wordlist are not the same length.
         * For the ocrpage words connected with a dash are one word while the code
         * below parts them into two. That confuses the replacement code if the user
         * decided. Solution:  KSpell should treat dash-linked words correctly.
         * We live with the problem here that dashes bring confusion ;-)
         */
        if( (*it).contains( rx ) )
            res += QStringList::split( rx, (*it) );
        else
#endif
            res << *it;
    }
    return res;

}

bool OcrWordList::updateOCRWord( const QString& from, const QString& to )
{
    OcrWordList::iterator it;
    bool res = false;

    for( it = begin(); it != end(); ++it )
    {
        QString word = (*it);
        kDebug() <<  "comparing word" << word;
        if (word.contains(from))			// case sensitive search
        {
            word.replace( from, to );
            *it = OcrWord( word );
            res = true;
            break;
        }
    }
    return res;
}

QRect OcrWordList::wordListRect() const
{
    QRect rect;
    for (OcrWordList::const_iterator it = constBegin(); it!=constEnd(); ++it)
    {
        rect = rect.unite( (*it).rect() );
    }
    return rect;
}


/*
 * since kspell removes , - | / etc. from words while they remain in the words
 * in the ocr wordlist.
 * This search goes through the wordlist and tries to find the words without caring
 * for special chars. It simply removes all chars from the words that are not alphanumeric.
 */
bool OcrWordList::findFuzzyIndex( const QString& word, OcrWord& resWord )
{
    OcrWordList::iterator it;
    bool res = false;

    for( it = begin(); it != end() && !res; ++it )
    {
        QString fuzzyword = (*it);
        fuzzyword.remove( QRegExp( "\\W" ));  // Remove all non-word characters.
        fuzzyword.remove( '_' );

        kDebug() <<  "comparing word" << fuzzyword
                 << "which was" << (*it)
                 << " with" <<  word;
        if( fuzzyword == word )
        {
            resWord = *it;
            res = true;
        }
    }
    return res;

}

void OcrWordList::setBlock( int b )
{
    m_block = b;
}
