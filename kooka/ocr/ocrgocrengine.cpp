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

#include "ocrgocrengine.h"

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_STRERROR
#include <string.h>
#endif

#include <qregexp.h>
#include <qfile.h>
#include <qdir.h>
#include <qstringlist.h>

#include <QDebug>
#include <kprocess.h>
#include <ktempdir.h>
#include <KLocalizedString>
#include <ktemporaryfile.h>
#include <kmessagebox.h>

#include "imgsaver.h"
#include "kookaimage.h"
#include "imageformat.h"
#include "ocrgocrdialog.h"

static const char *possibleResultFiles[] = { "out30.png", "out20.png", "out30.bmp", "out20.bmp", NULL };

OcrGocrEngine::OcrGocrEngine(QWidget *parent)
    : OcrEngine(parent)
{
    m_tempDir = NULL;
    m_inputFile = QString::null;            // input image file
    m_resultFile = QString::null;           // OCR result text file
    m_stderrFile = QString::null;           // stderr log/debug output
    m_ocrResultFile = QString::null;            // OCR result image
}

OcrGocrEngine::~OcrGocrEngine()
{
}

OcrBaseDialog *OcrGocrEngine::createOCRDialog(QWidget *parent)
{
    return (new OcrGocrDialog(parent));
}

OcrEngine::EngineType OcrGocrEngine::engineType() const
{
    return (OcrEngine::EngineGocr);
}

QString OcrGocrEngine::engineDesc()
{
    return (i18n("<qt>"
                 "<p>"
                 "<b>GOCR</b> (sometimes known as <b>JOCR</b>) is an open source "
                 "OCR engine, originally started by Joerg&nbsp;Schulenburg and now "
                 "with a team of active developers. "
                 "<p>"
                 "See <a href=\"http://jocr.sourceforge.net\">jocr.sourceforge.net</a> "
                 "for more information on GOCR."));
}

static QString newTempFile(const QString &suffix)
{
    KTemporaryFile tf;
    tf.setSuffix(suffix);
    tf.setAutoRemove(false);                // we will do this later
    tf.open();                      // create the file
    QString result = tf.fileName();         // save its assigned name
    tf.close();                     // don't need it any more
    return (result);
}

void OcrGocrEngine::startProcess(OcrBaseDialog *dia, const KookaImage *img)
{
    OcrGocrDialog *gocrDia = static_cast<OcrGocrDialog *>(dia);
    const QString cmd = gocrDia->getOCRCmd();

    const char *format;
    if (img->depth() == 1) {
        format = "PBM";    // B&W bitmap
    } else if (img->isGrayscale()) {
        format = "PGM";    // greyscale
    } else {
        format = "PPM";    // colour
    }

    // TODO: if the input file is local and is readable by GOCR,
    // can use it directly (but don't delete it afterwards!)
    m_inputFile = ImgSaver::tempSaveImage(img, ImageFormat(format));
    // save image to a temp file
    m_ocrResultFile = QString::null;            // don't know this until finished

    if (m_ocrProcess != NULL) {
        delete m_ocrProcess;    // kill old process if still there
    }
    m_ocrProcess = new KProcess();          // start new GOCR process
    Q_CHECK_PTR(m_ocrProcess);

    m_ocrResultText = QString::null;            // clear buffer for capturing

    m_tempDir = new KTempDir();             // new unique temporary directory
    m_ocrProcess->setWorkingDirectory(m_tempDir->name());
    // run process in there
    connect(m_ocrProcess, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(slotGOcrExited(int,QProcess::ExitStatus)));
    connect(m_ocrProcess, SIGNAL(readyReadStandardOutput()), SLOT(slotGOcrStdout()));

    QStringList args;                   // arguments for process

    if (img->numColors() < 0 || img->numColors() > 3) { // Not a B&W image
        args << "-l" << QString::number(gocrDia->getGraylevel());
    }
    args << "-s" << QString::number(gocrDia->getSpaceWidth());
    args << "-d" << QString::number(gocrDia->getDustsize());

    args << "-v" << "32";               // write a result image

    // Specify this explicitly, because "-" does not mean the same
    // as "/dev/stdout".  The former interleaves the progress outout
    // with the OCR result text.  See GOCR's process_arguments()
    // in gocr.c and ini_progress() in progress.c.
    //
    // This is still not very useful because GOCR only outputs progress
    // information every 10 seconds with no option for a shorter interval.
    // Set by 'time_t printinterval = 10' in GOCR's progress.c.
    //
    // Even for OCR processing which takes longer than that, it is still
    // not very useful because Kooka doesn't use the progress result - nothing
    // connects to the ocrProgress() signal :-(
    args << "-x" << "/dev/stdout";          // progress to stdout

    m_resultFile = newTempFile(".gocrout.txt");     // OCR result text file
    args << "-o" << QFile::encodeName(m_resultFile);

    m_stderrFile = newTempFile(".gocrerr.txt");     // stderr log/debug output
    args << "-e" << QFile::encodeName(m_stderrFile);

    args << "-i" << QFile::encodeName(m_inputFile); // input image file

    qDebug() << "Running GOCR on" << format << "file as" << cmd << args.join(" ");

    m_ocrProcess->setProgram(QFile::encodeName(cmd), args);
    m_ocrProcess->setNextOpenMode(QIODevice::ReadOnly);
    m_ocrProcess->setOutputChannelMode(KProcess::OnlyStdoutChannel);

    m_ocrProcess->start();
    if (!m_ocrProcess->waitForStarted(5000)) qWarning() << "Error starting GOCR process!";
}

void OcrGocrEngine::slotGOcrExited(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << "exit code" << exitCode << "status" << exitStatus;

    QFile rf(m_resultFile);
    if (!rf.open(QIODevice::ReadOnly)) {
#ifdef HAVE_STRERROR
        const char *reason = strerror(errno);
#else
        const char *reason = "";
#endif
        KMessageBox::error(NULL,
                           i18n("<qt>Cannot read GOCR result file <filename>%1</filename><br/>%2",
                                m_resultFile, reason),
                           i18n("GOCR Result File Error"));
        finishedOCRVisible(false);
        return;
    }

    m_ocrResultText = rf.readAll();         // read all the result text
    rf.close();                     // finished with result file

    // Now all the text output by GOCR is in m_ocrResultText. Split this up
    // first into lines and then into words, and save this as the OCR results.
    QStringList lines = m_ocrResultText.split('\n', QString::SkipEmptyParts);
    //qDebug() << "RESULT" << m_ocrResultText << "split to" << lines.count() << "lines";

    startResultDocument();

    for (QStringList::const_iterator itLine = lines.constBegin(); itLine != lines.constEnd(); ++itLine) {
        startLine();
        QStringList words = (*itLine).split(QRegExp("\\s+"), QString::SkipEmptyParts);
        for (QStringList::const_iterator itWord = words.constBegin(); itWord != words.constEnd(); ++itWord) {
            OcrWordData wd;
            addWord((*itWord), wd);
        }
        finishLine();
    }

    finishResultDocument();
    //qDebug() << "Finished splitting";

    // Find the GOCR result image
    QDir dir(m_tempDir->name());
    const char **prf = possibleResultFiles;
    while (*prf != NULL) {              // search for result files
        QString ri = dir.absoluteFilePath(*prf);
        if (QFile::exists(ri)) {            // take first one that matches
            qDebug() << "found result image" << ri;
            m_ocrResultFile = ri;
            break;
        }
        ++prf;
    }

    // This used to replace the introduced image with the result file, having
    // been cleared above:
    //
    //   if (m_introducedImage!=NULL) delete m_introducedImage;
    //   m_introducedImage = new KookaImage();
    //   ...
    //   if (!m_ocrResultFile.isNull()) m_introducedImage->load(m_ocrResultFile);
    //   else //qDebug() << "cannot find result image in" << dir.absolutePath();
    //
    // But that seems pointless - the replaced m_introducedImage was not
    // subsequently used anywhere (although would it be reused if "Start OCR"
    // were done again without closing the dialogue?).  Not doing this means
    // that m_introducedImage can be const, which in turn means that it can
    // refer to the viewed image directly - i.e. there is no need to take a
    // copy in OcrEngine::setImage().  The result image is still displayed in
    // the image viewer in OcrEngine::finishedOCRVisible().
    if (m_ocrResultFile.isNull()) {
        qDebug() << "cannot find result image in" << dir.absolutePath();
    }

    finishedOCRVisible(m_ocrProcess->exitStatus() == QProcess::NormalExit);
}

QStringList OcrGocrEngine::tempFiles(bool retain)
{
    QStringList result;

    result << m_inputFile << m_resultFile << m_stderrFile << m_ocrResultFile;

    if (m_tempDir != NULL) {
        result << m_tempDir->name();
        m_tempDir->setAutoRemove(!retain);
        delete m_tempDir;               // autoRemove will do the rest
        m_tempDir = NULL;
    }

    return (result);
}

void OcrGocrEngine::slotGOcrStdout()
{
    // This never seems to match!  Format from an earlier GOCR version?
    QRegExp rx1("^\\s*(\\d+)\\s+(\\d+)");

    // GOCR 0.49 20100924 prints progress as:
    //
    //  progress pgm2asc_main   100 / 100  time[s]     7 /     7  (skip=63)
    //
    // Split up because we don't know what the 2nd field (counter name)
    // may contain.
    QRegExp rx2a("^\\s*progress ");
    QRegExp rx2b("\\s(\\d+)\\s+/\\s+(\\d+)\\s+time");

    int progress = -1;
    int subProgress;

    m_ocrProcess->setReadChannel(QProcess::StandardOutput);
    QByteArray line;
    while (!(line = m_ocrProcess->readLine()).isEmpty()) {
        //qDebug() << "GOCR stdout:" << line;
        // Calculate OCR progress
        if (rx1.indexIn(line) > -1) {
            progress = rx1.capturedTexts()[1].toInt();
            subProgress = rx1.capturedTexts()[2].toInt();
        } else if (rx2a.indexIn(line) > -1 && rx2b.indexIn(line) > -1) {
            progress = rx2b.capturedTexts()[1].toInt();
            subProgress = rx2b.capturedTexts()[2].toInt();
        }

        if (progress > 0) {
            emit ocrProgress(progress, subProgress);
        }
    }
}