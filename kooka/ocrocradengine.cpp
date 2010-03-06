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

#include "ocrocradengine.h"
#include "ocrocradengine.moc"

#include <qregexp.h>
#include <qfile.h>
#include <qfileinfo.h>

#include <kdebug.h>
#include <kprocess.h>
#include <ktemporaryfile.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "imgsaver.h"
#include "imageformat.h"
#include "ocrocraddialog.h"


static const char UndetectedChar = '_';


OcrOcradEngine::OcrOcradEngine(QWidget *parent)
    : OcrEngine(parent)
{
    m_ocrResultFile = QString::null;
    m_ocrImagePBM = QString::null;
    m_tempOrfName = QString::null;
    ocradVersion = 0;
}


OcrOcradEngine::~OcrOcradEngine()
{
}


OcrBaseDialog *OcrOcradEngine::createOCRDialog(QWidget *parent)
{
    return (new OcrOcradDialog(parent));
}


QString OcrOcradEngine::engineDesc()
{
    return (i18n("<qt>"
                 "<p>"
                 "<b>OCRAD</b> is an free software OCR engine by Antonio&nbsp;Diaz, "
                 "part of the GNU&nbsp;Project. "
                 "<p>"
                 "Images for OCR should be scanned in black/white (lineart) mode. "
                 "Best results are achieved if the characters are at least "
                 "20&nbsp;pixels high.  Problems may arise, as usual, with very bold "
                 "or very light or broken characters, or with merged character groups."
                 "<p>"
                 "See <a href=\"http://www.gnu.org/software/ocrad/ocrad.html\">www.gnu.org/software/ocrad/ocrad.html</a> "
                 "for more information on OCRAD."));
}


void OcrOcradEngine::startProcess(OcrBaseDialog *dia, const KookaImage *img)
{
    const KConfigGroup grp = KGlobal::config()->group(CFG_GROUP_OCRAD);

    OcrOcradDialog *parentDialog = static_cast<OcrOcradDialog *>(dia);
    ocradVersion = parentDialog->getNumVersion();
    const QString cmd = parentDialog->getOCRCmd();
    m_verboseDebug = dia->verboseDebug();

    m_ocrResultFile = ImgSaver::tempSaveImage(img, ImageFormat("BMP"), 8);
    // TODO: if the input file is local and is readable by OCRAD,
    // can use it directly (but don't delete it afterwards!)
    m_ocrImagePBM = ImgSaver::tempSaveImage(img, ImageFormat("PBM"), 1);

    KTemporaryFile tmpOrf;				// temporary file for ORF result
    tmpOrf.setSuffix(".orf");
    tmpOrf.setAutoRemove(false);
    if (!tmpOrf.open())
    {
        kDebug() << "error creating temporary file";
        return;
    }
    m_tempOrfName = QFile::encodeName(tmpOrf.fileName());
    tmpOrf.close();					// just want its name

    if (m_ocrProcess!=NULL) delete m_ocrProcess;	// kill old process if still there
    m_ocrProcess = new KProcess();			// start new OCRAD process
    Q_CHECK_PTR(m_ocrProcess);

    connect(m_ocrProcess, SIGNAL(finished(int, QProcess::ExitStatus)), SLOT(slotOcradExited(int, QProcess::ExitStatus)));
    connect(m_ocrProcess, SIGNAL(readyReadStandardOutput()), SLOT(slotOcradStdout()));
    connect(m_ocrProcess, SIGNAL(readyReadStandardError()), SLOT(slotOcradStderr()));

    QStringList args;					// arguments for process
    args << "-x" << m_tempOrfName;			// the ORF result file

    args << QFile::encodeName(m_ocrImagePBM);		// name of the input image

    args << "-l" << QString::number(parentDialog->layoutDetectionMode());

    QString format = grp.readEntry(CFG_OCRAD_FORMAT, "utf8");
    args << "-F" << format;

    QString charset = grp.readEntry(CFG_OCRAD_CHARSET, "iso-8859-15");
    args << "-c" << charset;

    QString addArgs = grp.readEntry(CFG_OCRAD_EXTRA_ARGUMENTS);
    if (!addArgs.isEmpty()) args << addArgs;

    kDebug() << "Running OCRAD as [" << cmd << args.join(" ") << "]";

    m_ocrProcess->setProgram(QFile::encodeName(cmd), args);
    m_ocrProcess->setNextOpenMode(QIODevice::ReadOnly);
    m_ocrProcess->setOutputChannelMode(KProcess::SeparateChannels);

    m_ocrProcess->start();
    if (!m_ocrProcess->waitForStarted(5000))
    {
        kDebug() << "Error starting OCRAD process!";
    }
    else kDebug() << "OCRAD process started";
}


QStringList OcrOcradEngine::tempFiles(bool retain)
{
    QStringList result;

    if (!m_ocrResultFile.isNull())
    {
        result << m_ocrResultFile;
        m_ocrResultFile = QString::null;
    }

    if (!m_ocrImagePBM.isNull())
    {
        result << m_ocrImagePBM;
        m_ocrImagePBM = QString::null;
    }

    if (!m_tempOrfName.isNull())
    {
        result << m_tempOrfName;
        m_tempOrfName = QString::null;
    }

    return (result);
}


void OcrOcradEngine::slotOcradExited(int exitCode, QProcess::ExitStatus exitStatus)
{
    kDebug() << "exit code" << exitCode << "status" << exitStatus;

    slotOcradStdout();					// read anything remaining
    slotOcradStderr();

    bool parseRes = true;
    QString errStr = readORF(m_tempOrfName);
    if (!errStr.isNull())
    {
        KMessageBox::error(m_parent,
                           i18n("Parsing the OCRAD result file failed\n%1", errStr),
                           i18n("OCR Result Parse Problem"));
        parseRes = false;
    }

    finishedOCRVisible(parseRes);
}


// TODO: send this to a log for viewing
void OcrOcradEngine::slotOcradStderr()
{
    m_ocrProcess->setReadChannel(QProcess::StandardError);
    while (true)
    {
        QByteArray line = m_ocrProcess->readLine();
        if (line.isEmpty()) break;
        kDebug() << "OCRAD stderr:" << line;
    }
}


void OcrOcradEngine::slotOcradStdout()
{
    m_ocrProcess->setReadChannel(QProcess::StandardOutput);
    while (true)
    {
        QByteArray line = m_ocrProcess->readLine();
        if (line.isEmpty()) break;
        if (m_verboseDebug) kDebug() << "OCRAD stdout:" << line;
    }
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
run and show up the the results visually. 

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
    if (!file.exists()) return (i18n("File '%1' does not exist", fileName));
    QFileInfo fi(fileName);
    if (!fi.isReadable()) return (i18n("File '%1' unreadable", fileName));

    if (!file.open(QIODevice::ReadOnly)) return (i18n("Cannot open file '%1'", fileName));
    QTextStream stream(&file);

    kDebug() << "Starting to analyse ORF" << fileName << "version" << ocradVersion;

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
    int	blockCnt = 0;
    int currBlock = -1;
    QString line;
    QRect blockRect;

    startResultDocument();

    while (!stream.atEnd())
    {
        line = stream.readLine().trimmed();		// line of text excluding '\n'

        if (line.startsWith("#")) continue;		// ignore comments

        if (m_verboseDebug) kDebug() << "# Line" << line;
        if (line.startsWith("source file ")) continue;	// source file name, ignore
        else if (line.startsWith("total blocks "))	// total count of blocks,
        {						// must be first line
            blockCnt = line.mid(13).toInt();
            kDebug() << "Block count (V<10)" << blockCnt;
        }
        else if (line.startsWith("total text blocks "))
        {
            blockCnt = line.mid(18).toInt();
            kDebug() << "Block count (V>10)" << blockCnt;
        }
        else if (line.startsWith("block ") || line.startsWith("text block "))
        {						// start of text block
            // matching "block 1 0 0 560 792"
            if (rx1.indexIn(line)==-1)
            {
                kDebug() << "Failed to match 'block' line" << line;
                continue;
            }

            currBlock = (rx1.cap(1).toInt())-1;
            blockRect.setRect(rx1.cap(2).toInt(), rx1.cap(3).toInt(),
                              rx1.cap(4).toInt(), rx1.cap(5).toInt());
            kDebug() << "Current block" << currBlock << "rect" << blockRect;
        }
        else if (line.startsWith("lines "))		// lines in this block
        {
            int lineCnt = line.mid(6).toInt();
            kDebug() << "Block line count" << lineCnt;
        }
        else if (line.startsWith("line "))		// start of text line
        {
            startLine();

            if (rx2.indexIn(line)==-1)
            {
                kDebug() << "Failed to match 'line' line" << line;
                continue;
            }

            int charCount = rx2.cap(2).toInt();
            if (m_verboseDebug) kDebug() << "Expecting" << charCount << "chars for line" << lineNo;

            QString word;
            QRect wordRect;

            for (int c = 0; c<charCount && !stream.atEnd(); ++c)
            {						// read one line per character
                QString charLine = stream.readLine();
                int semiPos = charLine.indexOf(';');
                if (semiPos==-1)
                {
                    kDebug() << "No ';' in 'char' line" << charLine;
                    continue;
                }

                // rectStr contains the rectangle of the character
                QString rectStr = charLine.left(semiPos);
                // resultStr contains the OCRed result character(s)
                QString resultStr = charLine.mid(semiPos+1);

                QChar detectedChar = UndetectedChar;

                // find how many alternatives, matching " 1, 'r'0"
                if (rx3.indexIn(resultStr)==-1)
                {
                    kDebug() << "Failed to match" << resultStr << "in 'char' line" << charLine;
                    continue;
                }

                int altCount = rx3.cap(1).toInt();
                if (altCount==0)			// no alternatives,
                {					// undecipherable character
                    if (m_verboseDebug) kDebug() << "Undecipherable character in 'char' line" << charLine;
                }
                else
                {
                    int h = resultStr.indexOf(',');
                    if (h==-1)
                    {
                        kDebug() << "No ',' in" << resultStr << "in 'char' line" << charLine;
                        continue;
                    }
                    resultStr = resultStr.remove(0, h+1).trimmed();

                    // TODO: this only uses the first alternative
                    detectedChar = resultStr.at(1);

                    // Analyse the result rectangle
                    if (detectedChar!=' ')
                    {
                        if (rx4.indexIn(rectStr)==-1)
                        {
                            kDebug() << "Failed to match" << rectStr << "in 'char' line" << charLine;
                            continue;
                        }

                        QRect r(rx4.cap(1).toInt(), rx4.cap(2).toInt(),
                                rx4.cap(3).toInt(), rx4.cap(4).toInt());
                        wordRect |= r;
                    }
                }

                if (detectedChar==' ')			// space terminates the word
                {
                    if (ocradVersion<10)		// offset is relative to block
                    {
                        wordRect.translate(blockRect.x(), blockRect.y());
                    }

                    OcrWordData wd;
                    wd.setProperty(OcrWordData::Rectangle, wordRect);
                    addWord(word, wd);

                    word = QString::null;		// reset for next time
                    wordRect = QRect();
                }
                else word.append(detectedChar);		// append char to word
            }						// end of text line loop
            ++lineNo;

            if (!word.isEmpty())			// last word in line
            {
                if (ocradVersion<10)			// offset is relative to block
                {
                    wordRect.translate(blockRect.x(), blockRect.y());
                }

                OcrWordData wd;
                wd.setProperty(OcrWordData::Rectangle, wordRect);
                addWord(word, wd);

                word = QString::null;			// reset for next time
                wordRect = QRect();
            }

            finishLine();
        }
        else kDebug() << "Unknown line format" << line;
    }

    file.close();					// finished with ORF file
    finishResultDocument();
    kDebug() << "Finished analysing ORF";

    return (QString::null);				// no error detected
}
