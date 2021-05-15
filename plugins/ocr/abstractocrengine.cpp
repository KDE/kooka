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
#include <qtemporaryfile.h>

#include <qtextdocument.h>
#include <qtextcursor.h>

#include <kmessagebox.h>
#include <klocalizedstring.h>
#include <kcolorscheme.h>
#include <kconfigskeleton.h>

#include "imagecanvas.h"
#include "imageformat.h"

#include "abstractocrdialogue.h"
#include "ocr_logging.h"


//  Constructor/destructor and external engine creation
//  ---------------------------------------------------

AbstractOcrEngine::AbstractOcrEngine(QObject *pnt, const char *name)
    : AbstractPlugin(pnt),
      m_ocrProcess(nullptr),
      m_ocrRunning(false),
      m_ocrDialog(nullptr),
      m_imgCanvas(nullptr),
      m_document(nullptr),
      m_cursor(nullptr),
      m_currHighlight(-1),
      m_trackingActive(false)
{
    setObjectName(name);
    qCDebug(OCR_LOG) << objectName();

    m_parent = nullptr;
}


AbstractOcrEngine::~AbstractOcrEngine()
{
    qCDebug(OCR_LOG) << objectName();
    if (m_ocrProcess!=nullptr) delete m_ocrProcess;
    if (m_ocrDialog!=nullptr) delete m_ocrDialog;
}


/*
 * This is called to introduce a new image, usually if the user clicks on a
 * new image either in the gallery or on the thumbnailview.
 */
void AbstractOcrEngine::setImage(ScanImage::Ptr img)
{
    m_introducedImage = img;				// shared copy of original

    if (m_ocrDialog!=nullptr) m_ocrDialog->introduceImage(m_introducedImage);
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
    m_errorText.clear();				// ready for new messages

    m_ocrDialog = createOcrDialogue(this, pnt);
    Q_ASSERT(m_ocrDialog!=nullptr);

    if (!m_ocrDialog->setupGui())
    {
        const QString msg = collectErrorMessages(i18n("OCR could not be started."),
                                                 i18n("Check the OCR engine selection and settings."));
        int result = KMessageBox::warningContinueCancel(pnt, msg,
                                                        i18n("OCR Setup Error"),
                                                        KGuiItem(i18n("Configure OCR...")));
        if (result==KMessageBox::Continue) emit openOcrPrefs();
        return (false);					// with no OCR dialogue
    }

    connect(m_ocrDialog, &AbstractOcrDialogue::signalOcrStart, this, &AbstractOcrEngine::slotStartOCR);
    connect(m_ocrDialog, &AbstractOcrDialogue::signalOcrStop, this, &AbstractOcrEngine::slotStopOCR);
    connect(m_ocrDialog, &QDialog::rejected, this, &AbstractOcrEngine::slotClose);

    m_ocrDialog->introduceImage(m_introducedImage);
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
    Q_ASSERT(m_ocrDialog!=nullptr);

    stopOcrProcess(true);
    m_ocrDialog->enableGUI(false);			// enable controls again
}


/* Called by "Start" used while OCR is not in progress */
void AbstractOcrEngine::slotStartOCR()
{
    Q_ASSERT(m_ocrDialog!=nullptr);

    m_ocrDialog->enableGUI(true);			// disable controls while running
    m_ocrDialog->show();				// just in case it got closed

    createOcrProcess(m_ocrDialog, m_introducedImage);
}


void AbstractOcrEngine::stopOcrProcess(bool tellUser)
{
    if (m_ocrProcess!=nullptr && m_ocrProcess->state()==QProcess::Running)
    {
        qCDebug(OCR_LOG) << "Killing OCR process" << m_ocrProcess->pid();
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
    if (m_ocrDialog!=nullptr) m_ocrDialog->enableGUI(false);

    if (success)
    {
        if (!m_ocrResultFile.isEmpty() &&		// there is a result image
            m_imgCanvas!=nullptr)			// and we can display it
        {
            // Load the result image from its file.
            // The QSharedPointer passed to ImageCanvas::newImage() will
            // retain the image and delete it when it is no longer needed.
            ScanImage *resultImage = new ScanImage(QUrl::fromLocalFile(m_ocrResultFile));
            qCDebug(OCR_LOG) << "Result image from" << m_ocrResultFile << "size" << resultImage->size();
            m_imgCanvas->newImage(ScanImage::Ptr(resultImage), true);
            m_imgCanvas->setReadOnly(true);		// display on image canvas
            m_trackingActive = true;			// handle clicks on image
        }

        // Do this after loading the result image above.  Then when the signal
        // reaches KookaView::slotOcrResultAvailable() which then calls
        // updateSelectionState(), the image canvas has the image loaded and
        // the image view actions are correctly enabled.
        emit newOCRResultText();			// send out the text result

        /* now it is time to invoke the dictionary if required */
        // TODO: readOnlyEditor needed here? Also done in finishResultDocument()
        emit readOnlyEditor(false);			// user can now edit
        if (m_ocrDialog != nullptr)
        {
            emit setSpellCheckConfig(m_ocrDialog->customSpellConfigFile());

            bool doSpellcheck = m_ocrDialog->wantInteractiveSpellCheck();
            bool bgSpellcheck = m_ocrDialog->wantBackgroundSpellCheck();
            emit startSpellCheck(doSpellcheck, bgSpellcheck);
        }
    }

    if (m_ocrDialog!=nullptr) m_ocrDialog->hide();	// close the dialogue
    m_ocrRunning = false;
    removeTempFiles();

    qCDebug(OCR_LOG) << "OCR finished";
}


void AbstractOcrEngine::removeTempFiles()
{
    bool retain = m_ocrDialog->keepTempFiles();
    qCDebug(OCR_LOG) << "retain?" << retain;

    QStringList temps = tempFiles(retain);			// get files used by engine
    if (!m_ocrResultFile.isEmpty()) temps << m_ocrResultFile;	// plus our result image
    if (!m_ocrStderrLog.isEmpty()) temps << m_ocrStderrLog;	// and our standard error log
    if (temps.join("").isEmpty()) return;			// no temporary files to remove

    if (retain)
    {
        QString s = xi18nc("@info", "The following OCR temporary files are retained for debugging:<nl/><nl/>");
        for (QStringList::const_iterator it = temps.constBegin(); it != temps.constEnd(); ++it)
        {
            const QString file = (*it);
            if (file.isEmpty()) continue;

            QUrl u = QUrl::fromLocalFile(file);
            s += xi18nc("@info", "<filename><link url=\"%1\">%2</link></filename><nl/>", u.url(), file);
        }

        if (KMessageBox::questionYesNo(m_parent, s,
                                       i18n("OCR Temporary Files"),
                                       KStandardGuiItem::del(),
                                       KStandardGuiItem::close(),
                                       QString(),
                                       KMessageBox::AllowLink)==KMessageBox::Yes) retain = false;
    }

    if (!retain) {
        for (QStringList::const_iterator it = temps.constBegin(); it != temps.constEnd(); ++it) {
            if ((*it).isEmpty()) {
                continue;
            }

            QString tf = (*it);
            QFileInfo fi(tf);
            if (!fi.exists()) {				// what happened?
                //qCDebug(OCR_LOG) << "does not exist:" << tf;
            } else if (fi.isDir()) {
                //qCDebug(OCR_LOG) << "temp dir" << tf;
                QDir(tf).removeRecursively();		// recursive deletion
            } else {
                //qCDebug(OCR_LOG) << "temp file" << tf;
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
    if (m_imgCanvas == nullptr) {
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
    if (m_imgCanvas == nullptr) {
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
    qCDebug(OCR_LOG) << "words" << m_wordCount << "lines" << m_document->blockCount() << "chars" << m_document->characterCount();

    if (m_cursor != nullptr) delete m_cursor;
    emit readOnlyEditor(false);				// now let user edit it
}

void AbstractOcrEngine::startLine()
{
    if (verboseDebug()) {
        qCDebug(OCR_LOG);
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
    if (verboseDebug()) {
        qCDebug(OCR_LOG) << "word" << word << "len" << word.length()
                         << "rect" << data.property(OcrWordData::Rectangle)
                         << "alts" << data.property(OcrWordData::Alternatives);
    }

    if (!m_cursor->atBlockStart()) {
        m_cursor->insertText(" ", QTextCharFormat());
    }
    m_cursor->insertText(word, data);
    ++m_wordCount;
}


QString AbstractOcrEngine::tempFileName(const QString &suffix, const QString &baseName)
{
    QString protoName = QDir::tempPath()+'/'+baseName+"_XXXXXX";
    if (!suffix.isEmpty()) protoName += "."+suffix;

    QTemporaryFile tmpFile(protoName);
    tmpFile.setAutoRemove(false);

    if (!tmpFile.open())
    {
        qCDebug(OCR_LOG) << "error creating temporary file" << protoName;
        setErrorText(xi18nc("@info", "Cannot create temporary file <filename>%1</filename>", protoName));
        return (QString());
    }

    QString tmpName = QFile::encodeName(tmpFile.fileName());
    tmpFile.close();					// just want its name
    return (tmpName);
}


QString AbstractOcrEngine::tempSaveImage(ScanImage::Ptr img, const ImageFormat &format, int colors)
{
    if (img.isNull()) return (QString());		// no image to save

    QString tmpName = tempFileName(format.extension(), "imagetemp");

    ScanImage::Ptr tmpImg = img;			// original image
    if (colors!=-1 && img->depth()!=colors)		// need to convert it?
    {
        QImage::Format newfmt;
        switch (colors)
        {
case 1:     newfmt = QImage::Format_Mono;
            break;

case 8:     newfmt = QImage::Format_Indexed8;
            break;

case 24:    newfmt = QImage::Format_RGB888;
            break;

case 32:    newfmt = QImage::Format_RGB32;
            break;

default:    qCWarning(OCR_LOG) << "bad colour depth" << colors;
            return (QString());
        }

        tmpImg.clear();					// lose reference to original
        tmpImg.reset(new ScanImage(img->convertToFormat(newfmt)));
    }							// update with converted image

    qCDebug(OCR_LOG) << "saving to" << tmpName << "in format" << format;
    if (!tmpImg->save(tmpName, format.name()))
    {
        qCDebug(OCR_LOG) << "error saving to" << tmpName;
        setErrorText(xi18nc("@info", "Cannot save image to temporary file <filename>%1</filename>", tmpName));
        tmpName.clear();
    }

    return (tmpName);
}


bool AbstractOcrEngine::verboseDebug() const
{
    return (m_ocrDialog->verboseDebug());
}


QString AbstractOcrEngine::findExecutable(QString (*settingFunc)(), KConfigSkeletonItem *settingItem)
{
    QString exec = (*settingFunc)();			// get current setting
    if (exec.isEmpty()) settingItem->setDefault();	// if null, apply default
    exec = (*settingFunc)();				// and get new setting
    Q_ASSERT(!exec.isEmpty());				// should now have something
    qCDebug(OCR_LOG) << "configured/default" << exec;

    if (!QDir::isAbsolutePath(exec))			// not specified absolute path
    {
        const QString pathExec = QStandardPaths::findExecutable(exec);
        if (pathExec.isEmpty())				// try to find executable
        {
            qCDebug(OCR_LOG) << "no" << exec << "found on PATH";
            setErrorText(xi18nc("@info", "The executable <command>%1</command> could not be found on <envar>PATH</envar>."));
            return (QString());
        }
        exec = pathExec;
    }

    QFileInfo fi(exec);					// now check it is usable
    if (!fi.exists() || fi.isDir() || !fi.isExecutable())
    {
        qCDebug(OCR_LOG) << "configured" << exec << "not usable";
        setErrorText(xi18nc("@info", "The executable <filename>%1</filename> does not exist or is not usable.", fi.absoluteFilePath()));
        return (QString());
    }

    qCDebug(OCR_LOG) << "found" << exec;
    return (exec);
}


QString AbstractOcrEngine::collectErrorMessages(const QString &starter, const QString &ender)
{
    // Any error message(s) in m_errorText will already have been converted
    // from KUIT markup to HTML by xi18nc() or similar.  So all that is
    // needed is to build the rest of the error message in rich text also.
    // There will be some spurious <html> tags around each separate message
    // which has been converted, but they don't seem to cause any problem.

    m_errorText.prepend(QString());
    m_errorText.prepend(starter);
    m_errorText.prepend("<html>");

    m_errorText.append(QString());
    m_errorText.append(ender);
    m_errorText.append("</html>");

    return (m_errorText.join("<br/>"));
}


QProcess *AbstractOcrEngine::initOcrProcess()
{
    if (m_ocrProcess!=nullptr) delete m_ocrProcess;	// kill old process if still there

    m_ocrProcess = new QProcess();			// start new OCR process
    Q_CHECK_PTR(m_ocrProcess);
    qCDebug(OCR_LOG);

    m_ocrProcess->setStandardInputFile(QProcess::nullDevice());

    m_ocrProcess->setProcessChannelMode(QProcess::SeparateChannels);
    m_ocrStderrLog = tempFileName("stderr.log");
    m_ocrProcess->setStandardErrorFile(m_ocrStderrLog);

    return (m_ocrProcess);
}


bool AbstractOcrEngine::runOcrProcess()
{
    qCDebug(OCR_LOG) << "Running OCR," << m_ocrProcess->program() << m_ocrProcess->arguments();
    connect(m_ocrProcess, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished), this, &AbstractOcrEngine::slotProcessExited);

    m_ocrProcess->start();
    if (!m_ocrProcess->waitForStarted(5000))
    {
        qCWarning(OCR_LOG) << "Error starting OCR process";
        return (false);
    }

    return (true);
}


void AbstractOcrEngine::slotProcessExited(int exitCode, QProcess::ExitStatus exitStatus)
{
    qCDebug(OCR_LOG) << "exit code" << exitCode << "status" << exitStatus;

    bool success = (exitStatus==QProcess::NormalExit && exitCode==0);
    if (!success)					// OCR command failed
    {
        if (exitStatus==QProcess::CrashExit)
        {
            setErrorText(xi18nc("@info", "Command <command>%1</command> crashed with exit status <numid>%2</numid>",
                                m_ocrProcess->program(), exitCode));
        }
        else
        {
            setErrorText(xi18nc("@info", "Command <command>%1</command> exited with status <numid>%2</numid>",
                                m_ocrProcess->program(), exitCode));
        }

        const QString msg = collectErrorMessages(xi18nc("@info", "Running the OCR process failed."),
                                                 xi18nc("@info", "More information may be available in its <link url=\"%1\">standard error</link> log file.",
                                                        QUrl::fromLocalFile(m_ocrStderrLog).url()));

        KMessageBox::sorry(m_parent, msg, i18n("OCR Command Failed"), KMessageBox::AllowLink);
    }
    else						// OCR command succeeded
    {
        success = finishedOcrProcess(m_ocrProcess);	// process the OCR results
        if (!success)					// OCR processing failed
        {
            const QString msg = collectErrorMessages(xi18nc("@info", "Processing the OCR results failed."), QString());
            KMessageBox::sorry(m_parent, msg, i18n("OCR Processing Failed"), KMessageBox::AllowLink);
        }
    }

    finishedOcr(success);
}
