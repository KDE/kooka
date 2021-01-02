/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2003-2016 Klaas Freitag <freitag@suse.de>		*
 *                          Jonathan Marten <jjm@keelhaul.me.uk>	*
 *									*
 *  Kooka is free software; you can redistribute it and/or modify it	*
 *  under the terms of the GNU Library General Public License as	*
 *  published by the Free Software Foundation and appearing in the	*
 *  file COPYING included in the packaging of this file;  either	*
 *  version 2 of the License, or (at your option) any later version.	*
 *									*
 *  As a special exception, permission is given to link this program	*
 *  with any version of the KADMOS OCR/ICR engine (a product of		*
 *  reRecognition GmbH, Kreuzlingen), and distribute the resulting	*
 *  executable without including the source code for KADMOS in the	*
 *  source distribution.						*
 *									*
 *  This program is distributed in the hope that it will be useful,	*
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of	*
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*
 *  GNU General Public License for more details.			*
 *									*
 *  You should have received a copy of the GNU General Public		*
 *  License along with this program;  see the file COPYING.  If		*
 *  not, see <http://www.gnu.org/licenses/>.				*
 *									*
 ************************************************************************/

#include "ocrresedit.h"

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_STRERROR
#include <string.h>
#endif

#include <qcolor.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qfiledialog.h>

#include <klocalizedstring.h>
#include <kmessagebox.h>

#include "abstractocrengine.h"
#include "recentsaver.h"
#include "kooka_logging.h"

//  The OCR results are stored in our text document.  Each OCR'ed word has
//  properties stored in its QTextCharFormat recording the word rectangle
//  (if the OCR engine provides this information) and possibly other details
//  also.  We can read out those properties again to highlight the relevant
//  part of the result image when a cursor move or selection is made.
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

    setTabChangesFocus(true);               // will never OCR these
    slotSetReadOnly(true);              // initially, anyway

    connect(this, &OcrResEdit::cursorPositionChanged, this, &OcrResEdit::slotUpdateHighlight);

// TODO: monitor textChanged() signal, if document emptied (cleared)
// then tell OCR engine to stop tracking and double clicks
// then ImageCanvas can disable selection if tracking active (because it
// doesn't paint properly).
}

static void moveForward(QTextCursor &curs, bool once = true)
{
    if (once) {
        curs.movePosition(QTextCursor::NextCharacter);
    }
    while (curs.atBlockStart()) {
        curs.movePosition(QTextCursor::NextCharacter);
    }
}

void OcrResEdit::slotSelectWord(const QPoint &pos)
{
    if (document()->isEmpty()) {
        return;    // nothing to search
    }

    //qCDebug(KOOKA_LOG) << pos;

    QTextCursor curs(document());           // start of document
    QRect wordRect;

    // First find the start of the word corresponding to the clicked point

    moveForward(curs, false);
    while (!curs.atEnd()) {
        QTextCharFormat fmt = curs.charFormat();
        QRect rect = fmt.property(OcrWordData::Rectangle).toRect();
        ////qCDebug(KOOKA_LOG) << "at" << curs.position() << "rect" << rect;
        if (rect.isValid() && rect.contains(pos, true)) {
            wordRect = rect;
            break;
        }
        moveForward(curs);
    }

    //qCDebug(KOOKA_LOG) << "found rect" << wordRect << "at" << curs.position();

    if (!wordRect.isValid()) {
        return;    // no word found
    }

    // Then find the end of the word.  That is an OCR result word, i.e. a
    // span with the same character format, not a text word ended by whitespace.

    QTextCursor wordStart = curs;
    QTextCharFormat ref = wordStart.charFormat();

    moveForward(curs);
    while (!curs.atEnd()) {
        QTextCharFormat fmt = curs.charFormat();
        ////qCDebug(KOOKA_LOG) << "at" << curs.position() << "rect" << fmt.property(OcrWordData::Rectangle).toRect();
        if (fmt != ref) {
            ////qCDebug(KOOKA_LOG) << "mismatch at" << curs.position();
            break;
        }
        moveForward(curs);
    }

    curs.movePosition(QTextCursor::PreviousCharacter);
    //qCDebug(KOOKA_LOG) << "word start" << wordStart.position() << "end" << curs.position();
    int pos1 = wordStart.position();
    int pos2 = curs.position();
    if (pos1 == pos2) {
        return;    // no word found
    }

    QTextCursor wc(document());
    wc.setPosition(wordStart.position() - 1, QTextCursor::MoveAnchor);
    wc.setPosition(curs.position(), QTextCursor::KeepAnchor);
    setTextCursor(wc);
    ensureCursorVisible();
}

void OcrResEdit::slotSaveText()
{
    RecentSaver saver("saveOCR");
    QString fileName = QFileDialog::getSaveFileName(this, i18n("Save OCR Result Text"),
                                                    saver.recentPath(), i18n("Text File (*.txt)"));
    if (fileName.isEmpty()) return;
    saver.save(fileName);

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QString msg = xi18nc("@info", "Unable to save the OCR results file<nl/><filename>%1</filename>", fileName);
#ifdef HAVE_STRERROR
        msg += xi18nc("@info", "<nl/>%1", strerror(errno));
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
    if (isReadOnly()) {
        return;
    }
    ////qCDebug(KOOKA_LOG) << "pos" << textCursor().position() << "hassel" << textCursor().hasSelection()
    //         << "start" << textCursor().selectionStart() << "end" << textCursor().selectionEnd();

    QTextCursor curs = textCursor();			// will not move cursor, see
							// QTextEdit::textCursor() doc
    if (curs.hasSelection()) {
        ////qCDebug(KOOKA_LOG) << "sel start" << curs.selectionStart() << "end" << curs.selectionEnd();

        int send = curs.selectionEnd();
        curs.setPosition(curs.selectionStart());
        curs.movePosition(QTextCursor::NextCharacter);
        QTextCharFormat ref = curs.charFormat();
        ////qCDebug(KOOKA_LOG) << "at" << curs.position() << "format rect" << ref.property(OcrWordData::Rectangle).toRect();
        bool same = true;

        while (curs.position() != send) {
            curs.movePosition(QTextCursor::NextCharacter);
            QTextCharFormat fmt = curs.charFormat();
            ////qCDebug(KOOKA_LOG) << "at" << curs.position() << "format rect" << fmt.property(OcrWordData::Rectangle).toRect();
            if (fmt != ref) {
                ////qCDebug(KOOKA_LOG) << "mismatch at" << curs.position();
                same = false;
                break;
            }
        }

        ////qCDebug(KOOKA_LOG) << "range same format?" << same;
        if (same) {                 // valid word selection
            QRect r = ref.property(OcrWordData::Rectangle).toRect();
            ////qCDebug(KOOKA_LOG) << "rect" << r;
            emit highlightWord(r);
            return;
        }
    }

    emit highlightWord(QRect());            // no valid word selection,
    // clear highlight
    QTextCharFormat fmt = textCursor().charFormat();
    QRect r = fmt.property(OcrWordData::Rectangle).toRect();
    if (r.isValid()) {
        emit scrollToWord(r);    // scroll to cursor position
    }
}

// QTextEdit::setReadOnly() is no longer a slot in Qt4!
void OcrResEdit::slotSetReadOnly(bool isRO)
{
    setReadOnly(isRO);
    if (isRO) setCheckSpellingEnabled(false);
}
