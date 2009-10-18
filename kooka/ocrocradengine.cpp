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
#include "ocrocraddialog.h"


static const char UndetectedChar = '_';


OcrOcradEngine::OcrOcradEngine(QWidget *parent)
    : OcrEngine(parent)
{
    m_ocrResultImage = QString::null;
    m_ocrImagePBM = QString::null;
    m_tempOrfName = QString::null;
    ocradVersion = 0;
}


OcrOcradEngine::~OcrOcradEngine()
{
   cleanUpFiles();
}


OcrBaseDialog *OcrOcradEngine::createOCRDialog(QWidget *parent,KSpellConfig *spellConfig)
{
    return (new OcrOcradDialog(parent,spellConfig));
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


void OcrOcradEngine::startProcess(OcrBaseDialog *dia,KookaImage *img)
{
    const KConfigGroup grp = KGlobal::config()->group(CFG_GROUP_OCRAD);

    OcrOcradDialog *parentDialog = static_cast<OcrOcradDialog *>(dia);
    ocradVersion = parentDialog->getNumVersion();
    const QString cmd = parentDialog->getOCRCmd();

    m_ocrResultImage = ImgSaver::tempSaveImage(img,"BMP",8);
    m_ocrImagePBM = ImgSaver::tempSaveImage(img,"PBM",1);

    KTemporaryFile tmpOrf;				// temporary file for ORF result
    tmpOrf.setSuffix(".orf");
    tmpOrf.setAutoRemove(false);
    if (!tmpOrf.open())
    {
        kDebug() << "error creating temporary file";
        return;
    }
    m_tmpOrfName = QFile::encodeName(tmpOrf.fileName());
    tmpOrf.close();					// just want its name

    if (daemon!=NULL) delete daemon;			// kill old process if still there
    daemon = new KProcess();				// start new OCRAD process
    Q_CHECK_PTR(daemon);

    connect(daemon, SIGNAL(finished(int, QProcess::ExitStatus)), SLOT(slotOcradExited(int, QProcess::ExitStatus)));
    connect(daemon, SIGNAL(readyReadStandardOutput()), SLOT(slotOcradStdout()));
    connect(daemon, SIGNAL(readyReadStandardError()), SLOT(slotOcradStderr()));

    QStringList args;					// arguments for process
    args << "-x" << m_tmpOrfName;			// the ORF result file

    args << QFile::encodeName(m_ocrImagePBM);		// name of the input image

    args << "-l" << QString::number(parentDialog->layoutDetectionMode());

    QString format = grp.readEntry(CFG_OCRAD_FORMAT, "utf8");
    args << "-F" << format;

    QString charset = grp.readEntry(CFG_OCRAD_CHARSET, "iso-8859-15");
    args << "-c" << charset;

    QString addArgs = grp.readEntry(CFG_OCRAD_EXTRA_ARGUMENTS);
    if (!addArgs.isEmpty()) args << addArgs;

    kDebug() << "Running OCRAD as [" << cmd << args.join(" ") << "]";

    daemon->setProgram(QFile::encodeName(cmd), args);
    daemon->setNextOpenMode(QIODevice::ReadOnly);
    daemon->setOutputChannelMode(KProcess::SeparateChannels);

    daemon->start();
    if (!daemon->waitForStarted(5000))
    {
        kDebug() << "Error starting OCRAD process!";
    }
    else kDebug() << "OCRAD process started";
}


void OcrOcradEngine::cleanUpFiles()
{
    if (!m_ocrResultImage.isNull())
    {
        kDebug() << "Removing result file" << m_ocrResultImage;
        QFile::remove(m_ocrResultImage);
        m_ocrResultImage = QString::null;
    }

    if (!m_ocrImagePBM.isNull())
    {
        kDebug() << "Removing result file" << m_ocrImagePBM;
        QFile::remove(m_ocrImagePBM);
        m_ocrImagePBM = QString::null;
    }

    if (!m_tempOrfName.isNull() && m_unlinkORF)
    {
        kDebug() << "Removing ORF file" << m_tempOrfName;
        QFile::remove(m_tempOrfName);
        m_tempOrfName = QString::null;
    }
}


void OcrOcradEngine::slotOcradExited(int exitCode, QProcess::ExitStatus exitStatus)
{
    kDebug() << "exit code" << exitCode << "status" << exitStatus;

    slotOcradStdout();					// read anything remaining
    slotOcradStderr();

    bool parseRes = true;
    QString errStr = readORF(m_tmpOrfName);
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
    daemon->setReadChannel(QProcess::StandardError);
    while (true)
    {
        QByteArray line = daemon->readLine();
        if (line.isEmpty()) break;
        kDebug() << "OCRAD stderr:" << line;
    }
}


void OcrOcradEngine::slotOcradStdout()
{
    daemon->setReadChannel(QProcess::StandardOutput);
    while (true)
    {
        QByteArray line = daemon->readLine();
        if (line.isEmpty()) break;
        kDebug() << "OCRAD stdout:" << line;
    }
}




/*
 *  A sample orf snippet:
 *
 *  # Ocr Results File. Created by GNU ocrad version 0.3pre1
 *  total blocks 2
 *  block 1 0 0 560 344
 *  lines 5
 *  line 1 chars 10 height 26
 *  71 109 17 26;2,'0'1,'o'0
 *  93 109 15 26;2,'1'1,'l'0
 *  110 109 18 26;1,'2'0
 *  131 109 18 26;1,'3'0
 *  151 109 19 26;1,'4'0
 *  172 109 17 26;1,'5'0
 *  193 109 17 26;1,'6'0
 *  213 108 17 27;1,'7'0
 *  232 109 18 26;1,'8'0
 *  253 109 17 26;1,'9'0
 *  line 2 chars 14 height 27
 *
 */

QString OcrOcradEngine::readORF(const QString &fileName)
{
    QFile file(fileName);
    QRegExp rx;

    /* use a global line number counter here, not the one from the orf. The orf one
     * starts at 0 for every block, but we want line-no counting page global here.
     */
    int lineNo = 0;
    int	blockCnt = 0;
    int currBlock = -1;

    m_ocrPage.clear();					// clear the OCR result page

    /* some checks on the ORF */
    if (!file.exists()) return (i18n("File '%1' does not exist", fileName));
    QFileInfo fi(fileName);
    if (!fi.isReadable()) return (i18n("File '%1' unreadable", fileName));

    if (!file.open(QIODevice::ReadOnly)) return (i18n("Cannot open file '%1'", fileName));
    QTextStream stream(&file);
    QString line;
    QString recLine;					// recognised line

    kDebug() << "starting to analyse ORF" << fileName << "version" << ocradVersion;

    while ( !stream.atEnd() )
    {
            line = stream.readLine().trimmed();		// line of text excluding '\n'
	    int len = line.length();

	    if (line.startsWith("#")) continue;		// ignore comments

		kDebug() << "# Line check |" << line << "|";
		if( line.startsWith( "total blocks " ) )  // total count fo blocks, must be first line
		{
		    blockCnt = line.right( len - 13 /* QString("total blocks ").length() */ ).toInt();
		    kDebug() << "Amount of blocks:" << blockCnt;
		    m_blocks.resize(blockCnt);
		}
                else if( line.startsWith( "total text blocks " ))
                {
		    blockCnt = line.right( len - 18 /* QString("total text blocks ").length() */ ).toInt();
		    kDebug() << "Amount of blocks (V.10):" << blockCnt;
		    m_blocks.resize(blockCnt);
                }
		else if( line.startsWith( "block ") || line.startsWith( "text block ") )
		{
		    rx.setPattern("^.*block\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)");
		    if( rx.indexIn( line ) > -1)
		    {
			currBlock = (rx.cap(1).toInt())-1;
			kDebug() << "Setting current block" << currBlock;
			QRect r( rx.cap(2).toInt(), rx.cap(3).toInt(), rx.cap(4).toInt(), rx.cap(5).toInt());
			m_blocks[currBlock] = r;
		    }
		    else
		    {
			kDebug() << "Unknown block line:" << line;
                        // Not a killing bug
		    }
		}
		else if( line.startsWith( "lines " ))
		{
		    int lineCnt = line.right( len - 6 /* QString("lines ").length() */).toInt();
		    m_ocrPage.resize(m_ocrPage.size()+lineCnt);
		    kDebug() << "Resized ocrPage to linecount" << lineCnt;
		}
		else if( line.startsWith( "line" ))
		{
		    // line 5 chars 13 height 20
		    rx.setPattern("^line\\s+(\\d+)\\s+chars\\s+(\\d+)\\s+height\\s+\\d+" );
		    if( rx.indexIn( line )>-1 )
		    {
			kDebug() << "RegExp-Result:" << rx.cap(1) << ":" << rx.cap(2);
			int charCount = rx.cap(2).toInt();
			OcrWord word;
			QRect   brect;
			OcrWordList ocrLine;
                        ocrLine.setBlock(currBlock);
			/* Loop over all characters in the line. Every char has it's own line
			 * defined in the orf file */
			kDebug() << "Found" << charCount << "chars for line" << lineNo;

			for( int c=0; c < charCount && !stream.atEnd(); c++ )
			{
			    /* Read one line per character */
			    QString charLine = stream.readLine();
			    int semiPos = charLine.indexOf(';');
			    if( semiPos == -1 )
			    {
				kDebug() << "invalid line:" << charLine;
			    }
			    else
			    {
 				QString rectStr = charLine.left( semiPos );
				QString results = charLine.remove(0, semiPos+1 );
                                bool lineErr = false;

				// rectStr contains the rectangle info of for the character
				// results contains the real result caracter

                                // find the amount of alternatives.
                                int altCount = 0;
                                int h = results.indexOf(',');  // search the first comma
                                if( h > -1 ) {
                                    altCount = results.left(h).toInt();
                                    results = results.remove( 0, h+1 ).trimmed();
                                } else {
                                    lineErr = true;
                                }
                                QChar detectedChar = UndetectedChar;
                                if( !lineErr )
                                {
                                    /* take the first alternative only FIXME */
                                    if( altCount > 0 )
                                        detectedChar = results[1];
                                }

                                /* Analyse the rectangle */
                                if( ! lineErr && detectedChar != ' ' )
                                {
                                    rx.setPattern( "(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)");
                                    if(rx.indexIn(rectStr)!=-1)
                                    {
                                        /* unite the rectangles */
                                        QRect privRect( rx.cap(1).toInt(), rx.cap(2).toInt(),
                                                        rx.cap(3).toInt(), rx.cap(4).toInt() );
                                        word.setRect( word.rect() | privRect );
                                    }
                                    else
                                    {
                                        kDebug() << "Error: Unable to read rect info for char!";
                                        lineErr = true;
                                    }
                                }

                                if( ! lineErr )
                                {
                                    /* store word if finished by a space */
                                    if( detectedChar == ' ' )
                                    {
					/* add the block offset to the rect of the word */
					QRect r = word.rect();
                                        if( ocradVersion < 10 )
                                        {
                                            QRect blockRect = m_blocks[currBlock];
                                            r.translate( blockRect.x(), blockRect.y());
                                        }

					word.setRect( r );
                                        ocrLine.append( word );
                                        word = OcrWord();
                                    }
                                    else
                                    {
                                        word.append( detectedChar );
                                    }
                                }
			    }
			}
			if( !word.isEmpty() )
			{
			    /* add the block offset to the rect of the word */
			    QRect r = word.rect();
                            if( ocradVersion < 10 )
                            {
                                QRect blockRect = m_blocks[currBlock];
                                r.translate( blockRect.x(), blockRect.y());
                            }
			    word.setRect( r );

			    ocrLine.append( word );
			}
			if( lineNo<m_ocrPage.size() )
			{
			    kDebug() << "Store result line no" << lineNo << "=" << ocrLine.first();
			    m_ocrPage[lineNo] = ocrLine;
			    lineNo++;
			}
			else
			{
			    kDebug() << "Error: line index overflow" << lineNo;
			}
		    }
                    else
                    {
                        kDebug() << "Error: Unknown line found" << line;
                    }
		}
		else
		{
		    kDebug() << "Unknown line" << line;
		}

        }
        file.close();


    return (QString::null);				// no error detected
}
