/***************************************************************************
                   ocrresedit.cpp - ocr result editor widget
                             -------------------
    begin                : Tue 12 Feb 2003
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
#include <qcolor.h>

#include "ocrresedit.h"
#include "ocrword.h"
#include <kdebug.h>
#include <kfiledialog.h>
#include <klocale.h>

#include <qfile.h>
#include <qtextstream.h>

/* -------------------- ocrResEdit -------------------- */

ocrResEdit::ocrResEdit( QWidget *parent )
    : QTextEdit(parent)
{
    m_updateColor.setNamedColor( "SeaGreen");
    m_ignoreColor.setNamedColor( "CadetBlue4" );
    m_wrnColor.setNamedColor( "firebrick2" );
}


void ocrResEdit::slMarkWordWrong( int line, const ocrWord& word )
{
    // m_textEdit->setSelection( line,
    slReplaceWord( line, word, word, m_wrnColor );
}


void ocrResEdit::slUpdateOCRResult( int line, const QString& wordFrom,
                                   const QString& wordTo )
{
    /* the index is quite useless here, since  the text could have had been
     * changed by corrections before. Thus better search the word and update
     * it.
     */
    slReplaceWord( line, wordFrom, wordTo, m_updateColor );

}


void ocrResEdit::slIgnoreWrongWord( int line, const ocrWord& word )
{
    slReplaceWord( line, word, word, m_ignoreColor );
}


void ocrResEdit::slSelectWord( int line, const ocrWord& word )
{
   if( line < paragraphs() )
   {
      QString editLine = text(line);
      int cnt = editLine.contains( word);

      if( cnt > 0 )
      {
	 int pos = editLine.find(word);
	 setCursorPosition( line, pos );
	 setSelection( line, pos, line, pos + word.length());
      }
   }
}

void ocrResEdit::slReplaceWord( int line, const QString& wordFrom,
                               const QString& wordTo, const QColor& color )
{
    kdDebug(28000) << "Updating word " << wordFrom << " in line " << line << endl;

    bool isRO = isReadOnly();

    if( line < paragraphs() )
    {
        QString editLine = text(line);
        int cnt = editLine.contains( wordFrom );

        if( cnt > 0 )
        {
            int pos = editLine.find(wordFrom);
            setSelection( line, pos, line, pos+wordFrom.length());

            QColor saveCol = this->color();
            setColor( color );
            if( isRO ) {
                setReadOnly(false);
            }
            insert( wordTo, unsigned (4) );
            if( isRO ) {
                setReadOnly( true );
            }
            setColor(saveCol);
        }
        else
        {
            kdDebug(28000) << "WRN: Paragraph does not contain word " << wordFrom << endl;
        }

    }
    else
    {
        kdDebug(28000) << "WRN: editor does not have line " << line << endl;
    }
}


void ocrResEdit::slSaveText()
{
    QString fileName = KFileDialog::getSaveFileName( (QDir::home()).path(),
                                                 "*.txt",
                                                 this,
                                                 i18n("Save OCR Result Text") );
    if( fileName.isEmpty() )
      return;
    QFile file( fileName );
    if ( file.open( IO_WriteOnly ) )
    {
        QTextStream stream( &file );
        stream << text();
        file.close();
    }
}

#include "ocrresedit.moc"
/*   */
