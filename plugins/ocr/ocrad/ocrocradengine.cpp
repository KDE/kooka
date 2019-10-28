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

#include "ocrocradengine.h"

#include <qregexp.h>
#include <qfile.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qdebug.h>
#include <qtemporaryfile.h>
#include <qprocess.h>

#include <klocalizedstring.h>
#include <kpluginfactory.h>

#include "imageformat.h"
#include "kookasettings.h"
#include "ocrocraddialog.h"
#include "executablepathdialogue.h"


K_PLUGIN_FACTORY_WITH_JSON(OcrOcradEngineFactory, "kookaocr-ocrad.json", registerPlugin<OcrOcradEngine>();)
#include "ocrocradengine.moc"


static const char UndetectedChar = '_';


OcrOcradEngine::OcrOcradEngine(QObject *pnt, const QVariantList &args)
    : AbstractOcrEngine(pnt, "OcrOcradEngine")
{
    m_ocrImagePBM = QString();
    m_tempOrfName = QString();
    ocradVersion = 0;
}


AbstractOcrDialogue *OcrOcradEngine::createOcrDialogue(AbstractOcrEngine *plugin, QWidget *pnt)
{
    return (new OcrOcradDialog(plugin, pnt));
}


bool OcrOcradEngine::createOcrProcess(AbstractOcrDialogue *dia, const KookaImage *img)
{
    OcrOcradDialog *parentDialog = static_cast<OcrOcradDialog *>(dia);
    ocradVersion = parentDialog->getNumVersion();

    const QString cmd = parentDialog->getOCRCmd();

    const QString ocrResultFile = tempSaveImage(img, ImageFormat("BMP"), 8);
    setResultImage(ocrResultFile);
    // TODO: if the input file is local and is readable by OCRAD,
    // can use it directly (but don't delete it afterwards!)
    m_ocrImagePBM = tempSaveImage(img, ImageFormat("PBM"), 1);

    QProcess *proc = initOcrProcess();			// start process for OCR
    QStringList args;					// arguments for process

    m_tempOrfName = tempFileName("orf");
    args << "-x" << m_tempOrfName;			// the ORF result file

    args << QFile::encodeName(m_ocrImagePBM);		// name of the input image

    // Layout Detection
    int layoutMode = KookaSettings::ocrOcradLayoutDetection();
    if (ocradVersion >= 18)				// OCRAD 0.18 or later
    {							// has only on/off
        if (layoutMode != 0) args << "-l";
    }
    else						// OCRAD 0.17 or earlier
    {							// had 3 options
        args << "-l" << QString::number(layoutMode);
    }

    QString s = KookaSettings::ocrOcradFormat();
    if (!s.isEmpty()) args << "-F" << s;

    s = KookaSettings::ocrOcradCharset();
    if (!s.isEmpty()) args << "-c" << s;

    s = KookaSettings::ocrOcradFilter();
    if (!s.isEmpty()) args << "-e" << s;

    s = KookaSettings::ocrOcradTransform();
    if (!s.isEmpty()) args << "-t" << s;

    if (KookaSettings::ocrOcradInvert()) args << "-i";

    if (KookaSettings::ocrOcradThresholdEnable()) {
        s = KookaSettings::ocrOcradThresholdValue();
        if (!s.isEmpty()) args << "-T" << (s + "%");
    }

    if (verboseDebug()) args << "-v";

    s = KookaSettings::ocrOcradExtraArguments();
    if (!s.isEmpty()) args << s;

    proc->setProgram(cmd);
    proc->setArguments(args);

    proc->setProcessChannelMode(QProcess::SeparateChannels);
    m_tempStdoutLog = tempFileName("stdout.log");
    proc->setStandardOutputFile(m_tempStdoutLog);

    return (runOcrProcess());
}


QStringList OcrOcradEngine::tempFiles(bool retain)
{
    QStringList result;
    result << m_ocrImagePBM;
    result << m_tempOrfName;
    result << m_tempStdoutLog;

    return (result);
}


bool OcrOcradEngine::finishedOcrProcess(QProcess *proc)
{
    qDebug();
    QString errStr = readORF(m_tempOrfName);		// parse the OCR results
    if (errStr.isEmpty()) return (true);		// parsed successfulyl

    setErrorText(errStr);				// record the parse error
    return (false);					// parsing failed
}


/*
  From http://kooka.kde.org/news/

ORF Proposal: Ocr Result File    August 20, 2003

Ocrad is the first OCR (Optical Character Recognition) application that implements
output of OCR results in a special file format that could be easily processed by
frontend programs.  To provide a proper frontend connection, ocrad implements the
export of the OCR results into a so called ORF, which simply means Ocr Result File.

The ORF Format is a special file format that contains OCR results like the detected
characters and their position on the source image in a simply parseable format.
Frontend programs can read the file and retrieve information about the OCR engine
run and show up the results visually.

All lines starting with '#' are ignored.

The first valid line has the form 'source file filename', where 'filename' is the
name of the PBM file being processed.

The second valid line has the form 'total blocks n', where 'n' is the total number
of text blocks in the source image.

For each text block in the source image, the following data follows:

  A line in the form 'block i x y w h', where 'i' is the block number and 'x y w h'
  are the block position and size as described below for character boxes.

  A line in the form 'lines n', where 'n' is the number of lines in this block.

For each line in every text block, the following data follows:

  A line in the form 'line i chars n height h', where 'i' is the line number,
  'n' is the number of characters in this line,
  and 'h' is the mean height of the characters in this line (in pixels).

  n lines (one for every character) in the form "x y w h b;g[,'c'v]...".
  'x' = the left border (x-coordinate) of the char bounding box in the source image (in pixels).
  'y' = the top border (y-coordinate).
  'w' = the width of the bounding box.
  'h' = the height of the bounding box.
  'b' = the percent of black pixels in the bounding box.
  'g' = the number of different recognition guesses for this character.

  The result characters follow after the number of guesses in the form of a
  comma-separated list of pairs. Every pair is formed by the actual recognised
  char enclosed in single quotes, followed by the confidence value without
  space between them.

See the following snippet (the beginning of an orf) as a sample ORF:

  # Ocr Results File. Created by GNU ocrad version 0.4
  source file test1.pbm
  total blocks 1
  block 1 0 0 560 792
  lines 12
  line 1 chars 10 height 26
  71 109 17 26;2,'0'1,'o'0
  93 109 15 26;2,'1'1,'l'0
  110 109 18 26;1,'2'0
  131 109 18 26;1,'3'0
  151 109 19 26;1,'4'0
  172 109 17 26;1,'5'0
  193 109 17 26;1,'6'0
  213 108 17 27;1,'7'0
  232 109 18 26;1,'8'0
  253 109 17 26;1,'9'0
  line 2 chars 14 height 27
  68 153 29 27;1,'A'0
  97 153 24 27;1,'B'0
  ...

The ORF format was defined by Antonio Diaz and Klaas Freitag. Comments are very
welcome.
*/

QString OcrOcradEngine::readORF(const QString &fileName)
{
    QFile file(fileName);
    // some checks on the ORF
    if (!file.exists()) {
        return (xi18nc("@info", "File <filename>%1</filename> does not exist", fileName));
    }
    QFileInfo fi(fileName);
    if (!fi.isReadable()) {
        return (xi18nc("@info", "File <filename>%1</filename> unreadable", fileName));
    }

    if (!file.open(QIODevice::ReadOnly)) {
        return (xi18nc("@info", "Cannot open file <filename>%1</filename>", fileName));
    }
    QTextStream stream(&file);

    qDebug() << "Starting to analyse ORF" << fileName << "version" << ocradVersion;

    // to match "block 1 0 0 560 792"
    const QRegExp rx1("^.*block\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)");
    // to match "line 5 chars 13 height 20"
    const QRegExp rx2("^line\\s+(\\d+)\\s+chars\\s+(\\d+)\\s+height\\s+\\d+");
    // to match " 1, 'r'0"
    const QRegExp rx3("^\\s*(\\d+)");
    // to match "110 109 18 26"
    const QRegExp rx4("(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)");

    /* use a global line number counter here, not the one from the orf. The orf one
     * starts at 0 for every block, but we want line-no counting page global here.
     */
    int lineNo = 0;
    int blockCnt = 0;
    QString line;
    QRect blockRect;

    startResultDocument();

    while (!stream.atEnd()) {
        line = stream.readLine().trimmed();		// line of text excluding '\n'

        if (line.startsWith("#")) {
            continue;    // ignore comments
        }

        if (verboseDebug()) {
            //qDebug() << "# Line" << line;
        }
        if (line.startsWith("source file ")) {
            continue;					// source file name, ignore
        } else if (line.startsWith("total blocks ")) {	// total count of blocks,
							// must be first line
            blockCnt = line.mid(13).toInt();
            qDebug() << "Block count (V<10)" << blockCnt;
        } else if (line.startsWith("total text blocks ")) {
            blockCnt = line.mid(18).toInt();
            qDebug() << "Block count (V>10)" << blockCnt;
        } else if (line.startsWith("block ") || line.startsWith("text block ")) {
							// start of text block
							// matching "block 1 0 0 560 792"
            if (rx1.indexIn(line) == -1) {
                //qDebug() << "Failed to match 'block' line" << line;
                continue;
            }

            int currBlock = (rx1.cap(1).toInt()) - 1;
            blockRect.setRect(rx1.cap(2).toInt(), rx1.cap(3).toInt(),
                              rx1.cap(4).toInt(), rx1.cap(5).toInt());
            //qDebug() << "Current block" << currBlock << "rect" << blockRect;
        } else if (line.startsWith("lines ")) {		// lines in this block
            //qDebug() << "Block line count" << line.mid(6).toInt();
        } else if (line.startsWith("line ")) {		// start of text line
            startLine();

            if (rx2.indexIn(line) == -1) {
                //qDebug() << "Failed to match 'line' line" << line;
                continue;
            }

            int charCount = rx2.cap(2).toInt();
            if (verboseDebug()) {
                //qDebug() << "Expecting" << charCount << "chars for line" << lineNo;
            }

            QString word;
            QRect wordRect;

            for (int c = 0; c < charCount && !stream.atEnd(); ++c) {
                // read one line per character
                QString charLine = stream.readLine();
                int semiPos = charLine.indexOf(';');
                if (semiPos == -1) {
                    //qDebug() << "No ';' in 'char' line" << charLine;
                    continue;
                }

                // rectStr contains the rectangle of the character
                QString rectStr = charLine.left(semiPos);
                // resultStr contains the OCRed result character(s)
                QString resultStr = charLine.mid(semiPos + 1);

                QChar detectedChar = UndetectedChar;

                // find how many alternatives, matching " 1, 'r'0"
                if (rx3.indexIn(resultStr) == -1) {
                    //qDebug() << "Failed to match" << resultStr << "in 'char' line" << charLine;
                    continue;
                }

                int altCount = rx3.cap(1).toInt();
                if (altCount == 0) {			// no alternatives,
							// undecipherable character
                    if (verboseDebug()) {
                        //qDebug() << "Undecipherable character in 'char' line" << charLine;
                    }
                } else {
                    int h = resultStr.indexOf(',');
                    if (h == -1) {
                        //qDebug() << "No ',' in" << resultStr << "in 'char' line" << charLine;
                        continue;
                    }
                    resultStr = resultStr.remove(0, h + 1).trimmed();

                    // TODO: this only uses the first alternative
                    detectedChar = resultStr.at(1);

                    // Analyse the result rectangle
                    if (detectedChar != ' ') {
                        if (rx4.indexIn(rectStr) == -1) {
                            //qDebug() << "Failed to match" << rectStr << "in 'char' line" << charLine;
                            continue;
                        }

                        QRect r(rx4.cap(1).toInt(), rx4.cap(2).toInt(),
                                rx4.cap(3).toInt(), rx4.cap(4).toInt());
                        wordRect |= r;
                    }
                }

                if (detectedChar == ' ') {		// space terminates the word
                    if (ocradVersion < 10) {		// offset is relative to block
                        wordRect.translate(blockRect.x(), blockRect.y());
                    }

                    OcrWordData wd;
                    wd.setProperty(OcrWordData::Rectangle, wordRect);
                    addWord(word, wd);

                    word = QString();			// reset for next time
                    wordRect = QRect();
                } else {
                    word.append(detectedChar);		// append char to word
                }
            }						// end of text line loop
            ++lineNo;

            if (!word.isEmpty()) {			// last word in line
                if (ocradVersion < 10) {		// offset is relative to block
                    wordRect.translate(blockRect.x(), blockRect.y());
                }

                OcrWordData wd;
                wd.setProperty(OcrWordData::Rectangle, wordRect);
                addWord(word, wd);

                word = QString();			// reset for next time
                wordRect = QRect();
            }

            finishLine();
        } else {
            //qDebug() << "Unknown line format" << line;
        }
    }

    file.close();					// finished with ORF file
    finishResultDocument();
    qDebug() << "Finished analysing ORF";

    return (QString());					// no error detected
}


void OcrOcradEngine::openAdvancedSettings()
{
    ExecutablePathDialogue d(nullptr);

    QString exec = KookaSettings::ocrOcradBinary();
    if (exec.isEmpty())
    {
        KConfigSkeletonItem *ski = KookaSettings::self()->ocrOcradBinaryItem();
        ski->setDefault();
        exec = KookaSettings::ocrOcradBinary();
    }

    d.setPath(exec);
    d.setLabel(i18n("Name or path of the OCRAD executable:"));
    if (!d.exec()) return;

    KookaSettings::setOcrOcradBinary(d.path());
}
