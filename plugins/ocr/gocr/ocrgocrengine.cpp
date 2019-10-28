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
#include <qtemporaryfile.h>
#include <qtemporarydir.h>
#include <qdebug.h>
#include <qprocess.h>

#include <klocalizedstring.h>
#include <kpluginfactory.h>
#include <kconfigskeleton.h>

#include "imgsaver.h"
#include "kookaimage.h"
#include "imageformat.h"
#include "ocrgocrdialog.h"
#include "executablepathdialogue.h"
#include "kookasettings.h"


K_PLUGIN_FACTORY_WITH_JSON(OcrGocrEngineFactory, "kookaocr-gocr.json", registerPlugin<OcrGocrEngine>();)
#include "ocrgocrengine.moc"


static const char *possibleResultFiles[] = { "out30.png", "out20.png", "out30.bmp", "out20.bmp", nullptr };


OcrGocrEngine::OcrGocrEngine(QObject *pnt, const QVariantList &args)
    : AbstractOcrEngine(pnt, "OcrGocrEngine")
{
    m_tempDir = nullptr;
    m_inputFile = QString();			// input image file
    m_resultFile = QString();			// OCR result text file
}


AbstractOcrDialogue *OcrGocrEngine::createOcrDialogue(AbstractOcrEngine *plugin, QWidget *pnt)
{
    return (new OcrGocrDialog(plugin, pnt));
}


bool OcrGocrEngine::createOcrProcess(AbstractOcrDialogue *dia, const KookaImage *img)
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
    m_inputFile = tempSaveImage(img, ImageFormat(format));
							// save image to a temp file
    QProcess *proc = initOcrProcess();			// start process for OCR
    QStringList args;					// arguments for process
							// new unique temporary directory
    m_tempDir = new QTemporaryDir(QDir::tempPath()+"/ocrgocrdir_XXXXXX");
    proc->setWorkingDirectory(m_tempDir->path());	// run process in there

    if (img->colorCount() < 0 || img->colorCount() > 3) { // Not a B&W image
        args << "-l" << QString::number(gocrDia->getGraylevel());
    }
    args << "-s" << QString::number(gocrDia->getSpaceWidth());
    args << "-d" << QString::number(gocrDia->getDustsize());

    args << "-v" << "32";               // write a result image

    // TODO: use '-f' to output XML (with position and accuracy data)

    // Specify this explicitly, because "-" does not mean the same
    // as "/dev/stdout".  The former interleaves the progress output
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

    m_resultFile = tempFileName("gocrout.txt");		// OCR result text file
    args << "-o" << QFile::encodeName(m_resultFile);

    args << "-i" << QFile::encodeName(m_inputFile);	// input image file

    proc->setProgram(cmd);
    proc->setArguments(args);

    proc->setReadChannel(QProcess::StandardOutput);	// collect stdout for progress
    connect(proc, &QProcess::readyReadStandardOutput, this, &OcrGocrEngine::slotGOcrStdout);

    return (runOcrProcess());
}


bool OcrGocrEngine::finishedOcrProcess(QProcess *proc)
{
    qDebug();

    QFile rf(m_resultFile);
    if (!rf.open(QIODevice::ReadOnly)) {
#ifdef HAVE_STRERROR
        const char *reason = strerror(errno);
#else
        const char *reason = "";
#endif
        setErrorText(xi18nc("@info", "Cannot read GOCR result file <filename>%1</filename><nl/>%2", m_resultFile, reason));
        return (false);
    }

    const QString ocrResultText = rf.readAll();         // read all the result text
    rf.close();						// finished with result file

    // Now all the text output by GOCR is in m_ocrResultText. Split this up
    // first into lines and then into words, and save this as the OCR results.
    QStringList lines = ocrResultText.split('\n', QString::SkipEmptyParts);
    //qDebug() << "RESULT" << ocrResultText << "split to" << lines.count() << "lines";

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
    QDir dir(m_tempDir->path());
    QString foundResult;
    const char **prf = possibleResultFiles;
    while (*prf != nullptr) {              // search for result files
        QString ri = dir.absoluteFilePath(*prf);
        if (QFile::exists(ri)) {            // take first one that matches
            qDebug() << "found result image" << ri;
            foundResult = ri;
            break;
        }
        ++prf;
    }

    // This used to replace the introduced image with the result file, having
    // been cleared above:
    //
    //   if (m_introducedImage!=nullptr) delete m_introducedImage;
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
    if (!foundResult.isEmpty()) setResultImage(foundResult);
    else qDebug() << "cannot find result image in" << dir.absolutePath();

    return (true);
}


QStringList OcrGocrEngine::tempFiles(bool retain)
{
    QStringList result;

    result << m_inputFile << m_resultFile;

    if (m_tempDir != nullptr) {
        result << m_tempDir->path();
        m_tempDir->setAutoRemove(!retain);
        delete m_tempDir;               // autoRemove will do the rest
        m_tempDir = nullptr;
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

    QByteArray line;
    while (!(line = ocrProcess()->readLine()).isEmpty()) {
        //qDebug() << "GOCR stdout:" << line;
        // Calculate OCR progress
        if (rx1.indexIn(line) > -1) {
            progress = rx1.capturedTexts()[1].toInt();
            subProgress = rx1.capturedTexts()[2].toInt();
        } else if (rx2a.indexIn(line) > -1 && rx2b.indexIn(line) > -1) {
            progress = rx2b.capturedTexts()[1].toInt();
            subProgress = rx2b.capturedTexts()[2].toInt();
        }

        if (progress > 0) emit ocrProgress(progress, subProgress);
    }
}


void OcrGocrEngine::openAdvancedSettings()
{
    ExecutablePathDialogue d(nullptr);

    QString exec = KookaSettings::ocrGocrBinary();
    if (exec.isEmpty())
    {
        KConfigSkeletonItem *ski = KookaSettings::self()->ocrGocrBinaryItem();
        ski->setDefault();
        exec = KookaSettings::ocrGocrBinary();
    }

    d.setPath(exec);
    d.setLabel(i18n("Name or path of the GOCR executable:"));
    if (!d.exec()) return;

    KookaSettings::setOcrGocrBinary(d.path());
}
