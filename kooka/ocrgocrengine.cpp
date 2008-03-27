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

#include <kdebug.h>
#include <kprocess.h>
#include <ktempdir.h>
#include <klocale.h>

#include <qregexp.h>
#include <qfile.h>
#include <qdir.h>
#include <qstringlist.h>

#include "img_canvas.h"
#include "imgsaver.h"
#include "kookaimage.h"
#include "ocrgocrdialog.h"

#include "ocrgocrengine.h"
#include "ocrgocrengine.moc"



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

    QString format;
    if (img->depth()== 1) format = "PBM";		// B&W bitmap
    else if (img->isGrayscale()) format = "PGM";	// greyscale
    else format = "PPM";				// colour

    m_tempFile = ImgSaver::tempSaveImage(img,format);	// save image to a temp file

    m_ocrResultImage = QString::null;			// don't know this until finished

    if (daemon!=NULL) delete daemon;			// kill old process if still there
    daemon = new KProcess();				// start new GOCR process
    Q_CHECK_PTR(daemon);

    m_tempDir = new KTempDir();				// new unique temporary directory
    m_tempDir->setAutoDelete(true);			// clear it when finished
    daemon->setWorkingDirectory(m_tempDir->name());	// run process in there

    connect(daemon,SIGNAL(processExited(KProcess *)),SLOT(gocrExited(KProcess*)));
    connect(daemon,SIGNAL(receivedStdout(KProcess *,char *,int)),SLOT(gocrStdIn(KProcess *,char *,int)));
    connect(daemon,SIGNAL(receivedStderr(KProcess *,char *,int)),SLOT(gocrStdErr(KProcess *,char *,int)));

    *daemon << QFile::encodeName(cmd);
    *daemon << "-x" << "-";				// progress to stdout

    if (img->numColors()<0 || img->numColors()>3)	// Not a B&W image
    {
        *daemon << "-l" << QString::number(gocrDia->getGraylevel());
    }

    *daemon << "-s" << QString::number(gocrDia->getSpaceWidth());
    *daemon << "-d" << QString::number(gocrDia->getDustsize());

    *daemon << "-v" << "32";				// write a result image

    *daemon << "-i" << QFile::encodeName(m_tempFile);

    const QValueList<QCString> args = daemon->args();
    QStringList arglist;				// report the complete command
    for (QValueList<QCString>::const_iterator it = args.constBegin(); it!=args.constEnd(); ++it)
    {
        arglist.append(*it);
    }
    kdDebug(28000) <<  k_funcinfo << "Running GOGR on " << format << " file"
                   << " as [" << arglist.join(" ") << "]" << endl;

//       m_ocrCurrLine = 0;  // Important in gocrStdIn to store the results

    if (!daemon->start(KProcess::NotifyOnExit,KProcess::All))
    {
        kdDebug(28000) << "Error starting GOCR process!" << endl;
    }
    else kdDebug(28000) << "GOCR process started OK" << endl;
}





void OcrGocrEngine::gocrExited(KProcess *proc)
{
   kdDebug(28000) << k_funcinfo << endl;

   /* Now all the text of gocr is in the member m_ocrResultText. This one must
    * be split up now to m_ocrPage. First break up the lines, resize m_ocrPage
    * accordingly and than go through every line and create ocrwords for every
    * word.
    */
   QStringList lines = QStringList::split( '\n', m_ocrResultText, true );

   m_ocrPage.clear();
   m_ocrPage.resize( lines.count() );

   kdDebug(28000) << "RESULT " << m_ocrResultText << " was splitted to lines " << lines.count() << endl;

   unsigned int lineCnt = 0;

   for ( QStringList::Iterator it = lines.begin(); it != lines.end(); ++it )
   {
       //kdDebug(28000) << "Splitting up line " << *it << endl;
       ocrWordList ocrLine;

       QStringList words = QStringList::split( QRegExp( "\\s+" ), *it, false );
       for ( QStringList::Iterator itWord = words.begin(); itWord != words.end(); ++itWord )
       {
           //kdDebug(28000) << "Appending to results: " << *itWord << endl;
           ocrLine.append( ocrWord( *itWord ));
       }
       m_ocrPage[lineCnt] = ocrLine;
       lineCnt++;
   }
   kdDebug(28000) << "Finished to split!" << endl;

   if (m_introducedImage!=NULL) delete m_introducedImage;
   m_introducedImage = new KookaImage();		// start with a new image

   // Find and load the GOCR result image
   QDir *dir = m_tempDir->qDir();
   const char **prf = possibleResultFiles;
   while (*prf!=NULL)					// search for result files
   {
       QString rf = dir->absFilePath(*prf);
       if (QFile::exists(rf))				// take first one that matches
       {
           kdDebug(28000) << k_funcinfo << "found result image " << rf << endl;
           m_ocrResultImage = rf;
           break;
       }

       ++prf;
   }

   if (!m_ocrResultImage.isNull()) m_introducedImage->load(m_ocrResultImage);
   else kdDebug(28000) << k_funcinfo << "cannot find result image in" << dir->absPath() << endl;

   finishedOCRVisible(proc->normalExit());
}




void OcrGocrEngine::cleanUpFiles()
{
    if (!m_ocrResultImage.isNull())
    {
        kdDebug(28000) << k_funcinfo << "Removing result file " << m_ocrResultImage << endl;
        QFile::remove(m_ocrResultImage);
        m_ocrResultImage = QString::null;
    }

    if (!m_tempFile.isNull())
    {
        kdDebug(28000) << k_funcinfo << "Removing input file " << m_tempFile << endl;
        QFile::remove(m_tempFile);
        m_tempFile = QString::null;
    }

    if (m_tempDir!=NULL)
    {
        kdDebug(28000) << k_funcinfo << "Removing temp directory " << m_tempDir->name() << endl;
        delete m_tempDir;				// autoDelete will do the rest
        m_tempDir = NULL;
    }
}


// TODO: send this to a log for viewing
void OcrGocrEngine::gocrStdErr(KProcess *proc, char *buffer, int buflen)
{
   QString errorBuffer = QString::fromLocal8Bit(buffer, buflen);
   kdDebug(28000) << "gocr stderr: " << errorBuffer << endl;

}


void OcrGocrEngine::gocrStdIn(KProcess *proc, char* buffer, int buflen)
{
    QString aux = QString::fromLocal8Bit(buffer, buflen);

    QRegExp rx( "^\\s*\\d+\\s+\\d+");			// this never seems to match!
    if( rx.search( aux ) > -1 )
    {
        /* calculate ocr progress for gocr */
        int progress = rx.capturedTexts()[0].toInt();
        int subProgress = rx.capturedTexts()[1].toInt();
        // kdDebug(28000) << "Emitting progress: " << progress << endl;
        emit ocrProgress( progress, subProgress );
        return;
    }

    m_ocrResultText += aux;				// append to OCR results
}
