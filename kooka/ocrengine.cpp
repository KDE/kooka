/***************************************************************************
                             -------------------
    begin                : Fri Jun 30 2000
    copyright            : (C) 2000 by Klaas Freitag
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

#include "ocrengine.h"

#include <qpen.h>
#include <qvector.h>
#include <qevent.h>
#include <qfileinfo.h>

#include <qtextdocument.h>
#include <qtextcursor.h>

#include <QDebug>
#include <kmessagebox.h>
#include <kapplication.h>
#include <kprocess.h>
#include <KLocalizedString>
#include <kdialog.h>
#include <kcolorscheme.h>
#include <KGlobal>

#include <kio/deletejob.h>

#include "imagecanvas.h"

#include "kookaimage.h"
#include "kookapref.h"

#include "ocrbasedialog.h"
#include "ocrocraddialog.h"
#include "ocrgocrdialog.h"
#include "ocrgocrengine.h"
#include "ocrocradengine.h"
#ifdef HAVE_KADMOS
#include "kadmosocr.h"
#include "ocrkadmosengine.h"
#include "ocrkadmosdialog.h"
#endif

//  Constructor/destructor and external engine creation
//  ---------------------------------------------------

OcrEngine::OcrEngine(QWidget *parent)
    : m_ocrProcess(NULL),
      m_ocrDialog(NULL),
      m_ocrRunning(false),
      m_resultImage(0),
      m_imgCanvas(NULL),
      m_currHighlight(-1),
      m_trackingActive(false),
      m_document(NULL),
      m_cursor(NULL)
{
    m_introducedImage = KookaImage();
    m_ocrResultFile = QString::null;
    m_parent = NULL;
}

OcrEngine::~OcrEngine()
{
    if (m_ocrProcess != NULL) {
        delete m_ocrProcess;
    }
    if (m_ocrDialog != NULL) {
        delete m_ocrDialog;
    }
}

OcrEngine *OcrEngine::createEngine(QWidget *parent, bool *gotoPrefs)
{
    KConfigGroup grp = KSharedConfig::openConfig()->group(CFG_GROUP_OCR_DIA);

    OcrEngine::EngineType eng = static_cast<OcrEngine::EngineType>(grp.readEntry(CFG_OCR_ENGINE2,
                                static_cast<int>(OcrEngine::EngineNone)));
    //qDebug() << "configured OCR engine is" << eng << "=" << engineName(eng);

    QString msg = QString::null;
    switch (eng) {
    case OcrEngine::EngineGocr: return (new OcrGocrEngine(parent));
    case OcrEngine::EngineOcrad:    return (new OcrOcradEngine(parent));

    case OcrEngine::EngineKadmos:
#ifdef HAVE_KADMOS
        return (new OcrKadmosEngine(parent));
#else                           // HAVE_KADMOS
        msg = i18n("This version of Kooka is not compiled with KADMOS support.\n"
                   "Please select another OCR engine in the configuration dialog.");
        break;
#endif                          // HAVE_KADMOS

    case OcrEngine::EngineNone:     msg = i18n("No OCR engine is configured.\n"
                                              "Please select and configure one in the OCR configuration dialog.");
        break;

    default:            //qDebug() << "Cannot create engine of type" << eng;
        return (NULL);
    }

    if (!msg.isNull()) {                // failed, tell the user
        int result = KMessageBox::warningContinueCancel(parent, msg, QString::null,
                     KGuiItem(i18n("Configure OCR...")));
        if (gotoPrefs != NULL) {
            *gotoPrefs = (result == KMessageBox::Continue);
        }
    }

    return (NULL);
}

bool OcrEngine::engineValid() const
{
    OcrEngine::EngineType curEngine = engineType();

    KConfigGroup grp = KSharedConfig::openConfig()->group(CFG_GROUP_OCR_DIA);
    OcrEngine::EngineType confEngine = static_cast<OcrEngine::EngineType>(grp.readEntry(CFG_OCR_ENGINE2,
                                       static_cast<int>(OcrEngine::EngineNone)));

    //qDebug() << "cur" << curEngine << "conf" << confEngine;
    return (curEngine == confEngine);
}

const QString OcrEngine::engineName(OcrEngine::EngineType eng)
{
    switch (eng) {
    case OcrEngine::EngineNone: return (i18n("None"));
    case OcrEngine::EngineGocr: return (i18n("GOCR"));
    case OcrEngine::EngineOcrad:    return (i18n("OCRAD"));
    case OcrEngine::EngineKadmos:   return (i18n("Kadmos"));
    default:                return (i18n("Unknown"));
    }
}

/*
 * This is called to introduce a new image, usually if the user clicks on a
 * new image either in the gallery or on the thumbnailview.
 */
void OcrEngine::setImage(const KookaImage img)
{
    m_introducedImage = img;             // shallow copy of original

    if (m_ocrDialog != NULL) {
        m_ocrDialog->introduceImage(&m_introducedImage);
    }
    m_trackingActive = false;
}

/*
 * starts visual ocr process. Depending on the ocr engine, this function creates
 * a new dialog, and shows it.
 */
bool OcrEngine::startOCRVisible(QWidget *parent)
{
    if (m_ocrRunning) {
        KMessageBox::sorry(parent, i18n("OCR is already running"));
        return (false);
    }

    m_parent = parent;

    m_ocrDialog = createOCRDialog(parent);
    if (m_ocrDialog == NULL) {
        return (false);
    }

    m_ocrDialog->setupGui();
    m_ocrDialog->introduceImage(&m_introducedImage);

    connect(m_ocrDialog, &OcrBaseDialog::signalOcrStart, this, &OcrEngine::startOCRProcess);
    connect(m_ocrDialog, &OcrBaseDialog::signalOcrStop, this, &OcrEngine::slotStopOCR);
    connect(m_ocrDialog, &OcrBaseDialog::signalOcrClose, this, &OcrEngine::slotClose);
    m_ocrDialog->show();

    m_ocrRunning = true;
    return (true);
}

/**
 * This method should be called by the engine specific finish slots.
 * It does the engine independent cleanups like re-enabling buttons etc.
 */

void OcrEngine::finishedOCRVisible(bool success)
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
            //qDebug() << "Result image" << m_ocrResultFile
                     //<< "size" << m_resultImage->size();

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

    if (m_ocrDialog != NULL) {
        m_ocrDialog->hide();    // close the dialogue
    }

    m_ocrRunning = false;
    removeTempFiles();

    //qDebug() << "OCR finished";
}

void OcrEngine::removeTempFiles()
{
    bool retain = m_ocrDialog->keepTempFiles();
    //qDebug() << "retain=" << retain;

    const QStringList temps = tempFiles(retain);
    if (temps.isEmpty()) {
        return;
    }

    bool haveSome = false;
    if (retain) {
        QString s = i18n("<qt>The following OCR temporary files are retained for debugging:<p>");
        for (QStringList::const_iterator it = temps.constBegin(); it != temps.constEnd(); ++it) {
            if ((*it).isEmpty()) {
                continue;
            }

            KUrl u(*it);
            s += i18n("<filename><a href=\"%1\">%2</a></filename><br>", u.url(), u.pathOrUrl());
            haveSome = true;
        }

        if (haveSome) {
            if (KMessageBox::questionYesNo(NULL, s,
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
            if (!fi.exists()) {             // what happened?
                //qDebug() << "does not exist:" << tf;
            } else if (fi.isDir()) {
                //qDebug() << "temp dir:" << tf;
                //QT5 KIO::del(tf, KIO::HideProgressInfo);  // for recursive deletion
            } else {
                //qDebug() << "temp file:" << tf;
                QFile::remove(tf);          // just a simple file
            }
        }
    }
}

void OcrEngine::stopOCRProcess(bool tellUser)
{
    if (m_ocrProcess != NULL && m_ocrProcess->state() == QProcess::Running) {
        //qDebug() << "Killing daemon";
        m_ocrProcess->kill();
        if (tellUser) {
            KMessageBox::error(m_parent, i18n("The OCR process was stopped"));
        }
    }

    finishedOCRVisible(false);
}

/* Called by "Close" used while OCR is not in progress */
void OcrEngine::slotClose()
{
    //qDebug();
    stopOCRProcess(false);
}

/* Called by "Stop" used while OCR is in progress */
void OcrEngine::slotStopOCR()
{
    //qDebug();
    stopOCRProcess(true);
}

/* Called by "Start" used while OCR is not in progress */
void OcrEngine::startOCRProcess()
{
    if (m_ocrDialog == NULL) {
        return;
    }

    m_ocrResultText = "";
    startProcess(m_ocrDialog, &m_introducedImage);
}

//  Filtering mouse events on the image viewer
//  ------------------------------------------

void OcrEngine::setImageCanvas(ImageCanvas *canvas)
{
    m_imgCanvas = canvas;
    connect(m_imgCanvas, &ImageCanvas::doubleClicked, this, &OcrEngine::slotImagePosition);
}

void OcrEngine::slotImagePosition(const QPoint &p)
{
    if (!m_trackingActive) {
        return;    // not interested
    }
    // ImageCanvas did the coordinate conversion.
    // OcrResEdit does all of the rest of the work.
    emit selectWord(p);
}

//  Highlighting/scrolling the result text
//  --------------------------------------

void OcrEngine::slotHighlightWord(const QRect &r)
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

void OcrEngine::slotScrollToWord(const QRect &r)
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

void OcrEngine::setTextDocument(QTextDocument *doc)
{
    m_document = doc;
}

QTextDocument *OcrEngine::startResultDocument()
{
    //qDebug();

    m_document->setUndoRedoEnabled(false);
    m_document->clear();
    m_wordCount = 0;

    m_cursor = new QTextCursor(m_document);

    emit readOnlyEditor(true);              // read only while updating

    return (m_document);
}

void OcrEngine::finishResultDocument()
{
    //qDebug() << "words" << m_wordCount
             //<< "lines" << m_document->blockCount()
             //<< "chars" << m_document->characterCount();

    if (m_cursor != NULL) {
        delete m_cursor;
    }

    emit readOnlyEditor(false);             // now let user edit it
}

void OcrEngine::startLine()
{
    if (m_ocrDialog->verboseDebug()) {
        //qDebug();
    }
    if (!m_cursor->atStart()) {
        m_cursor->insertBlock(QTextBlockFormat(), QTextCharFormat());
    }
}

void OcrEngine::finishLine()
{
}

void OcrEngine::addWord(const QString &word, const OcrWordData &data)
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
