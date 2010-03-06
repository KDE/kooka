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

#include "ocrresedit.h"
#include "ocrresedit.moc"

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_STRERROR
#include <string.h>
#endif

#include <qcolor.h>
#include <qfile.h>
#include <qtextstream.h>

#include <kdebug.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "ocrengine.h"


//  The OCR results are stored in our text document.  Each OCR'ed word has
//  properties stored in is QTextCharFormat recording its word rectangle
//  (if the OCR engine has this information) and possibly other details also.
//  We can read out those properties again to highlight the relevant part of
//  the result image when a cursor move or selection is made.
//
//  Spell checking mostly uses KTextEdit's built in spell checking support
//  (which uses Sonnet).
//
//  Caution:  if the spell checking dialogue is cancelled, the text format
//  properties will be lost - the symptom of this is that the same place in
//  the result image will be highlighted no matter where in the text the
//  cursor or selection is.  This is bug 229150, hopefully fixed in KDE SC 4.5.


OcrResEdit::OcrResEdit(QWidget *parent)
    : KTextEdit(parent)
{
    setObjectName("OcrResEdit");

    setTabChangesFocus(true);				// will never OCR these
    slotSetReadOnly(true);				// initially, anyway

    connect(this, SIGNAL(cursorPositionChanged()), SLOT(slotUpdateHighlight()));
}


// TODO: still needs to be ported
#if 0
void OcrResEdit::slotSelectWord(int line, const OcrWord &word)
{
   if( line < paragraphs() )
   {
      QString editLine = text(line);
      int pos = editLine.indexOf(word);
      if (pos>-1)
      {
          QTextCursor curs(document());

	 setCursorPosition(line, pos);
	 setSelection( line, pos, line, pos + word.length());
         ensureCursorVisible();
      }
   }
}
#endif


void OcrResEdit::slotSaveText()
{
    QString fileName = KFileDialog::getSaveFileName(KUrl("kfiledialog:///saveOCR"),
                                                    "text/plain",
                                                    this,
                                                    i18n("Save OCR Result Text"));
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
    {
        QString msg = i18n("<qt>Unable to save the OCR results file<br><filename>%1</filename>", fileName);
#ifdef HAVE_STRERROR
        msg += i18n("<br>%1", strerror(errno));
#endif
        KMessageBox::error(this, msg, i18n("Error saving OCR results"));
        return;
    }

    QTextStream stream(&file);
    stream << toPlainText();
    file.close();
}


void OcrResEdit::slotUpdateHighlight()
{
    if (isReadOnly()) return;
    //kDebug() << "pos" << textCursor().position() << "hassel" << textCursor().hasSelection()
    //         << "start" << textCursor().selectionStart() << "end" << textCursor().selectionEnd();

    QTextCursor curs = textCursor();			// will not move cursor, see
							// QTextEdit::textCursor() doc
    if (curs.hasSelection())
    {
        //kDebug() << "sel start" << curs.selectionStart() << "end" << curs.selectionEnd();

        int send = curs.selectionEnd();
        curs.setPosition(curs.selectionStart());
        curs.movePosition(QTextCursor::NextCharacter);
        QTextCharFormat ref = curs.charFormat();
        //kDebug() << "at" << curs.position() << "format rect" << ref.property(OcrWordData::Rectangle).toRect();
        bool same = true;

        while (curs.position()!=send)
        {
            curs.movePosition(QTextCursor::NextCharacter);
            QTextCharFormat fmt = curs.charFormat();
            //kDebug() << "at" << curs.position() << "format rect" << fmt.property(OcrWordData::Rectangle).toRect();
            if (fmt!=ref)
            {
                //kDebug() << "mismatch at" << curs.position();
                same = false;
                break;
            }
        }

        //kDebug() << "range same format?" << same;
        if (same)					// valid word selection
        {
            QRect r = ref.property(OcrWordData::Rectangle).toRect();
            //kDebug() << "rect" << r;
            emit highlightWord(r);
            return;
        }
    }

    emit highlightWord(QRect());			// no valid word selection,
 							// clear highlight
    QTextCharFormat fmt = textCursor().charFormat();
    QRect r = fmt.property(OcrWordData::Rectangle).toRect();
    if (r.isValid()) emit scrollToWord(r);		// scroll to cursor position
}



// QTextEdit::setReadOnly() is no longer a slot in Qt4!
void OcrResEdit::slotSetReadOnly(bool isRO)
{
    setReadOnly(isRO);
    if (isRO) setCheckSpellingEnabled(false);
}
