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
#include "ocrgocrengine.moc"

#include <qregexp.h>
#include <qfile.h>
#include <qdir.h>
#include <qstringlist.h>

#include <kdebug.h>
#include <kprocess.h>
#include <ktempdir.h>
#include <klocale.h>

#include "libkscan/img_canvas.h"

#include "imgsaver.h"
#include "kookaimage.h"
#include "imageformat.h"
#include "ocrgocrdialog.h"




static const char *possibleResultFiles[] = { "out30.png", "out20.png", "out30.bmp", "out20.bmp", NULL };



OcrGocrEngine::OcrGocrEngine(QWidget *parent)
    : OcrEngine(parent)
{
    m_tempDir = NULL;
    m_tempFile = QString::null;
    m_ocrResultImage = QString::null;
}


OcrGocrEngine::~OcrGocrEngine()
{
    cleanUpFiles();
}


OcrBaseDialog *OcrGocrEngine::createOCRDialog(QWidget *parent,KSpellConfig *spellConfig)
{
    return (new OcrGocrDialog(parent,spellConfig));
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


void OcrGocrEngine::startProcess(OcrBaseDialog *dia,KookaImage *img)
{
    OcrGocrDialog *gocrDia = static_cast<OcrGocrDialog *>(dia);
    const QString cmd = gocrDia->getOCRCmd();

    const char *format;
    if (img->depth()== 1) format = "PBM";		// B&W bitmap
    else if (img->isGrayscale()) format = "PGM";	// greyscale
    else format = "PPM";				// colour

    m_tempFile = ImgSaver::tempSaveImage(img,ImageFormat(format));
							// save image to a temp file
    m_ocrResultImage = QString::null;			// don't know this until finished

    if (daemon!=NULL) delete daemon;			// kill old process if still there
    daemon = new KProcess();				// start new GOCR process
    Q_CHECK_PTR(daemon);

    m_ocrResultText = QString::null;			// clear buffer for capturing

    m_tempDir = new KTempDir();				// new unique temporary directory
    m_tempDir->setAutoRemove(true);			// clear it when finished
    daemon->setWorkingDirectory(m_tempDir->name());	// run process in there

    connect(daemon, SIGNAL(finished(int, QProcess::ExitStatus)), SLOT(slotGOcrExited(int, QProcess::ExitStatus)));
    connect(daemon, SIGNAL(readyReadStandardOutput()), SLOT(slotGOocrStdout()));
    connect(daemon, SIGNAL(readyReadStandardError()), SLOT(slotGOcrStderr()));

    QStringList args;					// arguments for process
    args << "-x" << "-";				// progress to stdout

    if (img->numColors()<0 || img->numColors()>3)	// Not a B&W image
    {
        args << "-l" << QString::number(gocrDia->getGraylevel());
    }
    args << "-s" << QString::number(gocrDia->getSpaceWidth());
    args << "-d" << QString::number(gocrDia->getDustsize());

    args << "-v" << "32";				// write a result image

    args << "-i" << QFile::encodeName(m_tempFile);

    kDebug() << "Running GOGR on" << format << "file"
             << "as [" << cmd << args.join(" ") << "]";

    daemon->setProgram(QFile::encodeName(cmd), args);
    daemon->setNextOpenMode(QIODevice::ReadOnly);
    daemon->setOutputChannelMode(KProcess::SeparateChannels);

    daemon->start();
    if (!daemon->waitForStarted(5000))
    {
        kDebug() << "Error starting GOCR process!";
    }
    else kDebug() << "GOCR process started";
}





void OcrGocrEngine::slotGOcrExited(int exitCode, QProcess::ExitStatus exitStatus)
{
   kDebug() << "exit code" << exitCode << "status" << exitStatus;

   slotGOcrStdout();					// read anything remaining
   slotGOcrStderr();

   /* Now all the text of gocr is in the member m_ocrResultText. This one must
    * be split up now to m_ocrPage. First break up the lines, resize m_ocrPage
    * accordingly and than go through every line and create ocrwords for every
    * word.
    */
   QStringList lines = m_ocrResultText.split('\n', QString::SkipEmptyParts);

   m_ocrPage.clear();
   m_ocrPage.resize( lines.count() );

   kDebug() << "RESULT" << m_ocrResultText << "split to" << lines.count() << "lines";

   unsigned int lineCnt = 0;

   for ( QStringList::Iterator it = lines.begin(); it != lines.end(); ++it )
   {
       OcrWordList ocrLine;

       QStringList words = (*it).split(QRegExp("\\s+"), QString::SkipEmptyParts);
       for ( QStringList::Iterator itWord = words.begin(); itWord != words.end(); ++itWord )
       {
           ocrLine.append( OcrWord( *itWord ));
       }
       m_ocrPage[lineCnt] = ocrLine;
       lineCnt++;
   }
   kDebug() << "Finished splitting";

   if (m_introducedImage!=NULL) delete m_introducedImage;
   m_introducedImage = new KookaImage();		// start with a new image

   // Find and load the GOCR result image
   QDir dir(m_tempDir->name());
   const char **prf = possibleResultFiles;
   while (*prf!=NULL)					// search for result files
   {
       QString rf = dir.absoluteFilePath(*prf);
       if (QFile::exists(rf))				// take first one that matches
       {
           kDebug() << "found result image" << rf;
           m_ocrResultImage = rf;
           break;
       }

       ++prf;
   }

   if (!m_ocrResultImage.isNull()) m_introducedImage->load(m_ocrResultImage);
   else kDebug() << "cannot find result image in" << dir.absolutePath();

   finishedOCRVisible(daemon->exitStatus()==QProcess::NormalExit);
}




void OcrGocrEngine::cleanUpFiles()
{
    if (!m_ocrResultImage.isNull())
    {
        kDebug() << "Removing result file" << m_ocrResultImage;
        QFile::remove(m_ocrResultImage);
        m_ocrResultImage = QString::null;
    }

    if (!m_tempFile.isNull())
    {
        kDebug() << "Removing input file" << m_tempFile;
        QFile::remove(m_tempFile);
        m_tempFile = QString::null;
    }

    if (m_tempDir!=NULL)
    {
        kDebug() << "Removing temp directory" << m_tempDir->name();
        delete m_tempDir;				// autoDelete will do the rest
        m_tempDir = NULL;
    }
}


// TODO: send this to a log for viewing
void OcrGocrEngine::slotGOcrStderr()
{
    daemon->setReadChannel(QProcess::StandardError);
    while (true)
    {
        QByteArray line = daemon->readLine();
        if (line.isEmpty()) break;
        kDebug() << "GOCR stderr:" << line;
    }
}


void OcrGocrEngine::slotGOcrStdout()
{
    QRegExp rx( "^\\s*\\d+\\s+\\d+");			// this never seems to match!

    daemon->setReadChannel(QProcess::StandardOutput);
    while (true)
    {
        QByteArray line = daemon->readLine();
        if (line.isEmpty()) break;

        if (rx.indexIn(line)>-1)
        {
            /* calculate ocr progress for gocr */
            int progress = rx.capturedTexts()[0].toInt();
            int subProgress = rx.capturedTexts()[1].toInt();
            emit ocrProgress(progress, subProgress);
            continue;
        }

        m_ocrResultText += line;			// append to OCR results
    }
}
