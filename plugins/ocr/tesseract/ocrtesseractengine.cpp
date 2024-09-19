/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2020      Jonathan Marten <jjm@keelhaul.me.uk>	*
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

#include "ocrtesseractengine.h"

#include <qfile.h>
#include <qfileinfo.h>
#include <qregularexpression.h>

#include <QXmlStreamReader>

#include <klocalizedstring.h>
#include <kpluginfactory.h>

#include "imageformat.h"
#include "kookasettings.h"
#include "ocrtesseractdialog.h"
#include "executablepathdialogue.h"
#include "ocr_logging.h"


K_PLUGIN_FACTORY_WITH_JSON(OcrTesseractEngineFactory, "kookaocr-tesseract.json", registerPlugin<OcrTesseractEngine>();)
#include "ocrtesseractengine.moc"


OcrTesseractEngine::OcrTesseractEngine(QObject *pnt, const QVariantList &args)
    : AbstractOcrEngine(pnt, "OcrTesseractEngine")
{
    m_ocrImageIn = QString();
    m_tempHOCROut = QString();
    m_tesseractVersion = 0;
}


AbstractOcrDialogue *OcrTesseractEngine::createOcrDialogue(AbstractOcrEngine *plugin, QWidget *pnt)
{
    return (new OcrTesseractDialog(plugin, pnt));
}


bool OcrTesseractEngine::createOcrProcess(AbstractOcrDialogue *dia, ScanImage::Ptr img)
{
    OcrTesseractDialog *parentDialog = static_cast<OcrTesseractDialog *>(dia);
    m_tesseractVersion = parentDialog->getNumVersion();

    const QString cmd = parentDialog->getOCRCmd();

    const QString ocrResultFile = tempSaveImage(img, ImageFormat("PGM"), 8);
    setResultImage(ocrResultFile);
    // TODO: if the input file is local and is readable by Tesseract,
    // can use it directly (but don't delete it afterwards!)
    m_ocrImageIn = tempSaveImage(img, ImageFormat("PNG"), 8);

    QProcess *proc = initOcrProcess();			// start process for OCR
    QStringList args;					// arguments for process

    // Input file
    args << QFile::encodeName(m_ocrImageIn);		// file with the input image

    // Output base name
    m_tempHOCROut = tempFileName("");			// Tesseract just wants base name
    args << QFile::encodeName(m_tempHOCROut);		// the HOCR result file
    m_tempHOCROut += ".hocr";				// suffix that it will have

    // Language
    QString s = KookaSettings::ocrTesseractLanguage();
    if (!s.isEmpty()) args << "-l" << s;

    // User words
    QUrl u = KookaSettings::ocrTesseractUserWords();
    if (u.isValid()) args << "--user-words" << u.toLocalFile();

    // User patterns
    u = KookaSettings::ocrTesseractUserPatterns();
    if (u.isValid()) args << "--user-patterns" << u.toLocalFile();

    // Page segmentation mode
    s = KookaSettings::ocrTesseractSegmentationMode();
    if (!s.isEmpty()) args << "--psm" << s;

    // OCR engine mode
    s = KookaSettings::ocrTesseractEngineMode();
    if (!s.isEmpty()) args << "--oem" << s;

    //if (verboseDebug()) args << "-v";

    s = KookaSettings::ocrTesseractExtraArguments();
    if (!s.isEmpty()) args << s;

    // Output format.  This option generates HOCR (HTML with OCR markup)
    // as specificied at http://kba.cloud/hocr-spec/1.2/
    args << "hocr";

    proc->setProgram(cmd);
    proc->setArguments(args);

    proc->setProcessChannelMode(QProcess::SeparateChannels);
    m_tempStdoutLog = tempFileName("stdout.log");
    proc->setStandardOutputFile(m_tempStdoutLog);

    return (runOcrProcess());
}


QStringList OcrTesseractEngine::tempFiles(bool retain)
{
    QStringList result;
    result << m_ocrImageIn;
    result << m_tempHOCROut;
    result << m_tempStdoutLog;
    return (result);
}


bool OcrTesseractEngine::finishedOcrProcess(QProcess *proc)
{
    qCDebug(OCR_LOG);
    QString errStr = readHOCR(m_tempHOCROut);		// parse the OCR results
    if (errStr.isEmpty()) return (true);		// parsed successfully

    setErrorText(errStr);				// record the parse error
    return (false);					// parsing failed
}


QString OcrTesseractEngine::readHOCR(const QString &fileName)
{
    // some basic checks on the file
    QFileInfo fi(fileName);
    if (!fi.exists()) return (xi18nc("@info", "File <filename>%1</filename> does not exist", fileName));
    if (!fi.isReadable()) return (xi18nc("@info", "File <filename>%1</filename> unreadable", fileName));

    qCDebug(OCR_LOG) << "Starting to analyse HOCR" << fileName;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        return (xi18nc("@info", "Cannot open file <filename>%1</filename>", fileName));
    }

    startResultDocument();				// start document to receive results

    QXmlStreamReader reader(&file);
    while (!reader.atEnd())
    {
        reader.readNext();				// get next XML token
        if (!reader.isStartElement()) continue;		// only interested in element start

        // We only take note of elements defining new paragraphs, new lines and words.
        // Examples of these are:
        //
        //   <p class='ocr_par' id='par_1_1' lang='eng' title="bbox 52 35 1469 84">
        //   <span class='ocr_line' id='line_1_1' title="bbox 52 35 1469 84; baseline 0.001 -10; x_size 41; x_descenders 9; x_ascenders 8">
        //   <span class='ocrx_word' id='word_1_1' title='bbox 52 42 123 74; x_wconf 92'>The</span> 
        //
        // These same HTML elements may specify other page layout data which are
        // not supported, but the class ID allows them to be distinguished.

        const QStringView name = reader.name();
        if (name==QStringLiteral("span") || name==QStringLiteral("p"))
        {						// may be either SPAN or P element
            const QStringView cls = reader.attributes().value("class");
            if (cls==QStringLiteral("ocr_par") || cls==QStringLiteral("ocr_line"))
            {						// paragraph or line start
                // The start of a paragraph is always followed by the start of a line.
                // Generating a new output line for both means that paragraphs are
                // separated by blank lines, as intended.
                startLine();
            }
            else if (cls==QStringLiteral("ocrx_word"))
            {
                OcrWordData wd;

                // The TITLE attribute of the SPAN element indicates the word bounding box.
                const QString ttl = reader.attributes().value("title").toString();

                const QRegularExpression rx("bbox\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+);");
                const QRegularExpressionMatch match = rx.match(ttl);
                if (!ttl.isEmpty() && match.hasMatch())
                {
                    QRect wordRect;
                    wordRect.setLeft(match.captured(1).toInt());
                    wordRect.setTop(match.captured(2).toInt());
                    wordRect.setRight(match.captured(3).toInt());
                    wordRect.setBottom(match.captured(4).toInt());
                    wd.setProperty(OcrWordData::Rectangle, wordRect);
                }

                // The contained text is the recognised word.  No child HTML elements
                // are expected inside a word SPAN.
                QString text = reader.readElementText(QXmlStreamReader::SkipChildElements);
                addWord(text, wd);
            }
        }
    }

    if (reader.hasError())
    {
        qCDebug(OCR_LOG) << "XML reader error, line" << reader.lineNumber() << reader.error();
        return (i18n("HOCR parsing error, %1", reader.errorString()));
    }

    finishResultDocument();				// finished with output document
    file.close();					// finished with HOCR file

    qCDebug(OCR_LOG) << "Finished analysing HOCR";

    return (QString());					// no error detected
}


void OcrTesseractEngine::openAdvancedSettings()
{
    ExecutablePathDialogue d(nullptr);

    QString exec = KookaSettings::ocrTesseractBinary();
    if (exec.isEmpty())
    {
        KConfigSkeletonItem *ski = KookaSettings::self()->ocrTesseractBinaryItem();
        ski->setDefault();
        exec = KookaSettings::ocrTesseractBinary();
    }

    d.setPath(exec);
    d.setLabel(i18n("Name or path of the Tesseract executable:"));
    if (!d.exec()) return;

    KookaSettings::setOcrTesseractBinary(d.path());
}
