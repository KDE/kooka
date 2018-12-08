/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2000-2016 Klaas Freitag <freitag@suse.de>		*
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

#include "abstractocrengine.h"

#include <qdir.h>
#include <qfileinfo.h>
#include <qprocess.h>
#include <qdebug.h>
#include <qtemporaryfile.h>

#include <qtextdocument.h>
#include <qtextcursor.h>

#include <kmessagebox.h>
#include <klocalizedstring.h>
#include <kcolorscheme.h>

#include "imagecanvas.h"
#include "imageformat.h"
#include "kookaimage.h"

#include "abstractocrdialogue.h"


//  Constructor/destructor and external engine creation
//  ---------------------------------------------------

AbstractOcrEngine::AbstractOcrEngine(QObject *pnt)
    : AbstractPlugin(pnt),
      m_ocrProcess(nullptr),
      m_ocrRunning(false),
      m_ocrDialog(nullptr),
      m_resultImage(nullptr),
      m_imgCanvas(nullptr),
      m_document(nullptr),
      m_cursor(nullptr),
      m_currHighlight(-1),
      m_trackingActive(false)
{
    m_introducedImage = KookaImage();
    m_ocrResultFile = QString();
    m_parent = nullptr;
}


AbstractOcrEngine::~AbstractOcrEngine()
{
    if (m_ocrProcess!=nullptr) delete m_ocrProcess;
    if (m_ocrDialog!=nullptr) delete m_ocrDialog;
}


/*
 * This is called to introduce a new image, usually if the user clicks on a
 * new image either in the gallery or on the thumbnailview.
 */
void AbstractOcrEngine::setImage(const KookaImage img)
{
    m_introducedImage = img;				// shallow copy of original

    if (m_ocrDialog!=nullptr) m_ocrDialog->introduceImage(&m_introducedImage);
    m_trackingActive = false;
}


/*
 * Starts the visual OCR process. Depending on the OCR engine, this function creates
 * a new dialog, and shows it.
 */
bool AbstractOcrEngine::openOcrDialogue(QWidget *pnt)
{
    if (m_ocrRunning) {
        KMessageBox::sorry(pnt, i18n("OCR is already in progress"));
        return (false);
    }

    m_parent = pnt;

    m_ocrDialog = createOcrDialogue(this, pnt);
    if (m_ocrDialog==nullptr) return (false);

    m_ocrDialog->setupGui();
    m_ocrDialog->introduceImage(&m_introducedImage);

    connect(m_ocrDialog, &AbstractOcrDialogue::signalOcrStart, this, &AbstractOcrEngine::slotStartOCR);
    connect(m_ocrDialog, &AbstractOcrDialogue::signalOcrStop, this, &AbstractOcrEngine::slotStopOCR);
    connect(m_ocrDialog, &AbstractOcrDialogue::signalOcrClose, this, &AbstractOcrEngine::slotClose);
    m_ocrDialog->show();

    // TODO: m_ocrActive would better reflect the function (if indeed useful at all)
    m_ocrRunning = true;
    return (true);
}


/* Called by "Close" used while OCR is not in progress */
void AbstractOcrEngine::slotClose()
{
    stopOcrProcess(false);
}


/* Called by "Stop" used while OCR is in progress */
void AbstractOcrEngine::slotStopOCR()
{
    stopOcrProcess(true);
}


/* Called by "Start" used while OCR is not in progress */
void AbstractOcrEngine::slotStartOCR()
{
    if (m_ocrDialog==nullptr) return;
    m_ocrDialog->show();

    m_ocrResultText.clear();
    startOcrProcess(m_ocrDialog, &m_introducedImage);
}


void AbstractOcrEngine::stopOcrProcess(bool tellUser)
{
    if (m_ocrProcess!=nullptr && m_ocrProcess->state()==QProcess::Running)
    {
        qDebug() << "Killing OCR process" << m_ocrProcess->pid();
        m_ocrProcess->kill();
        if (tellUser) KMessageBox::error(m_parent, i18n("The OCR process was stopped"));
    }

    finishedOcr(false);
}


/**
 * This method should be called by the engine specific finish slots.
 * It does the engine independent cleanups like re-enabling buttons etc.
 */
void AbstractOcrEngine::finishedOcr(bool success)
{
    if (m_ocrDialog != NULL) {
        m_ocrDialog->enableGUI(false);
    }

    if (success) {
        emit newOCRResultText();

        if (m_imgCanvas != NULL) {
            if (m_resultImage != NULL) {
                delete m_resultImage;
            }

            m_resultImage = new QImage(m_ocrResultFile);
            qDebug() << "Result image" << m_ocrResultFile << "size" << m_resultImage->size();

            /* The image canvas is present. Set it to our image */
            m_imgCanvas->newImage(m_resultImage, true);
            m_imgCanvas->setReadOnly(true);

            /* now handle double clicks to jump to the word */
            m_trackingActive = true;
        }

        /* now it is time to invoke the dictionary if required */
        // TODO: readOnlyEditor needed here? Also done in finishResultDocument()
        emit readOnlyEditor(false);         // user can now edit
        if (m_ocrDialog != NULL) {
            emit setSpellCheckConfig(m_ocrDialog->customSpellConfigFile());

            bool doSpellcheck = m_ocrDialog->wantInteractiveSpellCheck();
            bool bgSpellcheck = m_ocrDialog->wantBackgroundSpellCheck();
            emit startSpellCheck(doSpellcheck, bgSpellcheck);
        }
    }

    if (m_ocrDialog!=nullptr) m_ocrDialog->hide();	// close the dialogue
    m_ocrRunning = false;
    removeTempFiles();

    qDebug() << "OCR finished";
}


void AbstractOcrEngine::removeTempFiles()
{
    bool retain = m_ocrDialog->keepTempFiles();
    qDebug() << "retain=" << retain;

    const QStringList temps = tempFiles(retain);
    if (temps.isEmpty()) {
        return;
    }

    bool haveSome = false;
    if (retain) {
        QString s = xi18nc("@info", "The following OCR temporary files are retained for debugging:<nl/><nl/>");
        for (QStringList::const_iterator it = temps.constBegin(); it != temps.constEnd(); ++it) {
            if ((*it).isEmpty()) {
                continue;
            }

            QUrl u(*it);
            s += xi18nc("@info", "<filename><link url=\"%1\">%2</link></filename><nl/>", u.url(), u.url(QUrl::PreferLocalFile));
            haveSome = true;
        }

        if (haveSome) {
            if (KMessageBox::questionYesNo(nullptr, s,
                                           i18n("OCR Temporary Files"),
                                           KStandardGuiItem::del(),
                                           KStandardGuiItem::close(),
                                           QString::null,
                                           KMessageBox::AllowLink) == KMessageBox::Yes) {
                retain = false;
            }
        }
    }

    if (!retain) {
        for (QStringList::const_iterator it = temps.constBegin(); it != temps.constEnd(); ++it) {
            if ((*it).isEmpty()) {
                continue;
            }

            QString tf = (*it);
            QFileInfo fi(tf);
            if (!fi.exists()) {				// what happened?
                //qDebug() << "does not exist:" << tf;
            } else if (fi.isDir()) {
                //qDebug() << "temp dir" << tf;
                QDir(tf).removeRecursively();		// recursive deletion
            } else {
                //qDebug() << "temp file" << tf;
                QFile::remove(tf);			// just a simple file
            }
        }
    }
}


//  Filtering mouse events on the image viewer
//  ------------------------------------------
void AbstractOcrEngine::setImageCanvas(ImageCanvas *canvas)
{
    m_imgCanvas = canvas;
    connect(m_imgCanvas, &ImageCanvas::doubleClicked, this, &AbstractOcrEngine::slotImagePosition);
}


void AbstractOcrEngine::slotImagePosition(const QPoint &p)
{
    if (!m_trackingActive) return;			// not interested

    // ImageCanvas did the coordinate conversion.
    // OcrResEdit does all of the rest of the work.
    emit selectWord(p);
}


//  Highlighting/scrolling the result text
//  --------------------------------------
void AbstractOcrEngine::slotHighlightWord(const QRect &r)
{
    if (m_imgCanvas == NULL) {
        return;
    }

    if (m_currHighlight > -1) {
        m_imgCanvas->removeHighlight(m_currHighlight);
    }
    m_currHighlight = -1;

    if (!m_trackingActive) {
        return;    // not highlighting
    }
    if (!r.isValid()) {
        return;    // word rectangle invalid
    }

    KColorScheme sch(QPalette::Active, KColorScheme::Selection);
    QColor col = sch.background(KColorScheme::NegativeBackground).color();
    m_imgCanvas->setHighlightStyle(ImageCanvas::HighlightBox, QPen(col, 2));
    m_currHighlight = m_imgCanvas->addHighlight(r, true);
}

void AbstractOcrEngine::slotScrollToWord(const QRect &r)
{
    if (m_imgCanvas == NULL) {
        return;
    }

    if (m_currHighlight > -1) {
        m_imgCanvas->removeHighlight(m_currHighlight);
    }
    m_currHighlight = -1;

    if (!m_trackingActive) {
        return;    // not highlighting
    }

    KColorScheme sch(QPalette::Active, KColorScheme::Selection);
    QColor col = sch.background(KColorScheme::NeutralBackground).color();
    m_imgCanvas->setHighlightStyle(ImageCanvas::HighlightUnderline, QPen(col, 2));
    m_currHighlight = m_imgCanvas->addHighlight(r, true);
}


//  Assembling the OCR results
//  --------------------------
void AbstractOcrEngine::setTextDocument(QTextDocument *doc)
{
    m_document = doc;
}


QTextDocument *AbstractOcrEngine::startResultDocument()
{
    m_document->setUndoRedoEnabled(false);
    m_document->clear();
    m_wordCount = 0;

    m_cursor = new QTextCursor(m_document);

    emit readOnlyEditor(true);				// read only while updating
    return (m_document);
}

void AbstractOcrEngine::finishResultDocument()
{
    qDebug() << "words" << m_wordCount << "lines" << m_document->blockCount() << "chars" << m_document->characterCount();

    if (m_cursor != NULL) delete m_cursor;
    emit readOnlyEditor(false);				// now let user edit it
}

void AbstractOcrEngine::startLine()
{
    if (m_ocrDialog->verboseDebug()) {
        //qDebug();
    }
    if (!m_cursor->atStart()) {
        m_cursor->insertBlock(QTextBlockFormat(), QTextCharFormat());
    }
}

void AbstractOcrEngine::finishLine()
{
}

void AbstractOcrEngine::addWord(const QString &word, const OcrWordData &data)
{
    if (m_ocrDialog->verboseDebug()) {
        //qDebug() << "word" << word << "len" << word.length()
        //<< "rect" << data.property(OcrWordData::Rectangle)
        //<< "alts" << data.property(OcrWordData::Alternatives);
    }

    if (!m_cursor->atBlockStart()) {
        m_cursor->insertText(" ", QTextCharFormat());
    }
    m_cursor->insertText(word, data);
    ++m_wordCount;
}


QString AbstractOcrEngine::tempSaveImage(const KookaImage *img, const ImageFormat &format, int colors)
{
    if (img==nullptr) return (QString::null);

    const QString tempTemplate = QDir::tempPath()+'/'+"imgsaverXXXXXX."+format.extension();
    QTemporaryFile tmpFile(tempTemplate);
    tmpFile.setAutoRemove(false);

    if (!tmpFile.open())
    {
        qDebug() << "Error opening temp file" << tmpFile.fileName();
        tmpFile.setAutoRemove(true);
        return (QString::null);
    }
    QString name = tmpFile.fileName();
    tmpFile.close();

    const KookaImage *tmpImg = NULL;
    if (colors != -1 && img->depth() != colors) { // Need to convert image
        QImage::Format newfmt;
        switch (colors) {
        case 1:     newfmt = QImage::Format_Mono;
            break;

        case 8:     newfmt = QImage::Format_Indexed8;
            break;

        case 24:    newfmt = QImage::Format_RGB888;
            break;

        case 32:    newfmt = QImage::Format_RGB32;
            break;

        default:    //qDebug() << "Error: Bad colour depth requested" << colors;
            tmpFile.setAutoRemove(true);
            return (QString::null);
        }

        tmpImg = new KookaImage(img->convertToFormat(newfmt));
        img = tmpImg;
    }

    qDebug() << "Saving to" << name << "in format" << format;
    if (!img->save(name, format.name())) {
        qDebug() << "Error saving to" << name;
        tmpFile.setAutoRemove(true);
        name = QString::null;
    }

    if (tmpImg != NULL) delete tmpImg;
    return (name);
}
