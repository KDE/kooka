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
#include <ktempfile.h>
#include <klocale.h>
#include <kmessagebox.h>

#include <qregexp.h>
#include <qfile.h>
#include <qfileinfo.h>

#include "imgsaver.h"
#include "ocrocraddialog.h"

#include "ocrocradengine.h"
#include "ocrocradengine.moc"


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
    KConfig *konf = KGlobal::config();
    KConfigGroupSaver(konf,CFG_GROUP_OCRAD);

    OcrOcradDialog *parentDialog = static_cast<OcrOcradDialog *>(dia);
    ocradVersion = parentDialog->getNumVersion();
    const QString cmd = parentDialog->getOCRCmd();

    m_ocrResultImage = ImgSaver::tempSaveImage(img,"BMP",8);
    m_ocrImagePBM = ImgSaver::tempSaveImage(img,"PBM",1);

    KTempFile tmpOrf(QString::null,".orf");
    tmpOrf.setAutoDelete(false);			// temporary file for ORF result
    tmpOrf.close();
    m_tmpOrfName = QFile::encodeName(tmpOrf.name());

    if (daemon!=NULL) delete daemon;			// kill old process if still there
    daemon = new KProcess();				// start new OCRAD process
    Q_CHECK_PTR(daemon);

    *daemon << cmd;
    *daemon << "-x" << m_tmpOrfName;			// the ORF result file

    *daemon << QFile::encodeName(m_ocrImagePBM);	// name of the input image

    *daemon << "-l" << QString::number(parentDialog->layoutDetectionMode());

    QString format = konf->readEntry(CFG_OCRAD_FORMAT,"utf8");
    *daemon << "-F" << format;

    QString charset = konf->readEntry(CFG_OCRAD_CHARSET,"iso-8859-15");
    *daemon << "-c" << charset;

    QString addArgs = konf->readEntry(CFG_OCRAD_EXTRA_ARGUMENTS);
    if (!addArgs.isEmpty()) *daemon << addArgs;

    m_ocrResultText = "";

    connect(daemon,SIGNAL(processExited(KProcess *)),SLOT(ocradExited(KProcess *)));
    connect(daemon,SIGNAL(receivedStdout(KProcess *,char *,int)),SLOT(ocradStdIn(KProcess *,char *,int)));
    connect(daemon,SIGNAL(receivedStderr(KProcess *,char *,int)),SLOT(ocradStdErr(KProcess *,char *,int)));

    const QValueList<QCString> args = daemon->args();
    QStringList arglist;				// report the complete command
    for (QValueList<QCString>::const_iterator it = args.constBegin(); it!=args.constEnd(); ++it)
    {
        arglist.append(*it);
    }
    kdDebug(28000) <<  k_funcinfo << "Running OCRAD as [" << arglist.join(" ") << "]" << endl;

    if (!daemon->start(KProcess::NotifyOnExit,KProcess::All))
    {
        kdDebug(28000) << "Error starting OCRAD process!" << endl;
    }
    else kdDebug(28000) << "OCRAD process started OK" << endl;
}


void OcrOcradEngine::cleanUpFiles()
{
    if (!m_ocrResultImage.isNull())
    {
        kdDebug(28000) << k_funcinfo << "Removing result file " << m_ocrResultImage << endl;
        QFile::remove(m_ocrResultImage);
        m_ocrResultImage = QString::null;
    }

    if (!m_ocrImagePBM.isNull())
    {
        kdDebug(28000) << k_funcinfo << "Removing result file " << m_ocrImagePBM << endl;
        QFile::remove(m_ocrImagePBM);
        m_ocrImagePBM = QString::null;
    }

    if (!m_tempOrfName.isNull() && m_unlinkORF)
    {
        kdDebug(28000) << k_funcinfo << "Removing ORF file " << m_tempOrfName << endl;
        QFile::remove(m_tempOrfName);
        m_tempOrfName = QString::null;
    }
}


void OcrOcradEngine::ocradExited(KProcess *proc)
{
    kdDebug(28000) << k_funcinfo << endl;

    bool parseRes = true;
    QString errStr = readORF(m_tmpOrfName);
    if (!errStr.isNull())
    {
        KMessageBox::error(m_parent,i18n("Parsing the OCRAD result file failed\n%1").arg(errStr),
                           i18n("OCR Result Parse Problem"));
        parseRes = false;
    }

    finishedOCRVisible(parseRes);
}




// TODO: send this to a log for viewing
void OcrOcradEngine::ocradStdErr(KProcess *proc,char *buffer,int buflen)
{
   QString errorBuffer = QString::fromLocal8Bit(buffer,buflen);
   kdDebug(28000) << "ocrad stderr: " << errorBuffer << endl;

}

void OcrOcradEngine::ocradStdIn(KProcess *proc,char *buffer,int buflen)
{
   QString errorBuffer = QString::fromLocal8Bit(buffer,buflen);
   kdDebug(28000) << "ocrad stdin: " << errorBuffer << endl;
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
    unsigned int lineNo = 0;
    int	blockCnt = 0;
    int currBlock = -1;

    m_ocrPage.clear();					// clear the OCR result page

    /* some checks on the ORF */
    if (!file.exists()) return (i18n("File '%1' does not exist").arg(fileName));
    QFileInfo fi(fileName);
    if (!fi.isReadable()) return (i18n("File '%1' unreadable").arg(fileName));

    if (!file.open(IO_ReadOnly)) return (i18n("Cannot open file '%1'").arg(fileName));
    QTextStream stream(&file);
    QString line;
    QString recLine;					// recognised line

    kdDebug(28000) << k_funcinfo << "starting to analyse ORF " << fileName
                   << " version " << ocradVersion << endl;

    while ( !stream.atEnd() )
    {
            line = stream.readLine().stripWhiteSpace(); // line of text excluding '\n'
	    int len = line.length();

	    if( ! line.startsWith( "#" ))  // Comments
	    {
		kdDebug(28000) << "# Line check |" << line << "|" << endl;
		if( line.startsWith( "total blocks " ) )  // total count fo blocks, must be first line
		{
		    blockCnt = line.right( len - 13 /* QString("total blocks ").length() */ ).toInt();
		    kdDebug(28000) << "Amount of blocks: " << blockCnt << endl;
		    m_blocks.resize(blockCnt);
		}
                else if( line.startsWith( "total text blocks " ))
                {
		    blockCnt = line.right( len - 18 /* QString("total text blocks ").length() */ ).toInt();
		    kdDebug(28000) << "Amount of blocks (V. 10): " << blockCnt << endl;
		    m_blocks.resize(blockCnt);
                }
		else if( line.startsWith( "block ") || line.startsWith( "text block ") )
		{
		    rx.setPattern("^.*block\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)");
		    if( rx.search( line ) > -1)
		    {
			currBlock = (rx.cap(1).toInt())-1;
			kdDebug(28000) << "Setting current block " << currBlock << endl;
			QRect r( rx.cap(2).toInt(), rx.cap(3).toInt(), rx.cap(4).toInt(), rx.cap(5).toInt());
			m_blocks[currBlock] = r;
		    }
		    else
		    {
			kdDebug(28000) << "WRN: Unknown block line: " << line << endl;
                        // Not a killing bug
		    }
		}
		else if( line.startsWith( "lines " ))
		{
		    int lineCnt = line.right( len - 6 /* QString("lines ").length() */).toInt();
		    m_ocrPage.resize(m_ocrPage.size()+lineCnt);
		    kdDebug(28000) << "Resized ocrPage to linecount " << lineCnt << endl;
		}
		else if( line.startsWith( "line" ))
		{
		    // line 5 chars 13 height 20
		    rx.setPattern("^line\\s+(\\d+)\\s+chars\\s+(\\d+)\\s+height\\s+\\d+" );
		    if( rx.search( line )>-1 )
		    {
			kdDebug(28000) << "RegExp-Result: " << rx.cap(1) << " : " << rx.cap(2) << endl;
			int charCount = rx.cap(2).toInt();
			ocrWord word;
			QRect   brect;
			ocrWordList ocrLine;
                        ocrLine.setBlock(currBlock);
			/* Loop over all characters in the line. Every char has it's own line
			 * defined in the orf file */
			kdDebug(28000) << "Found " << charCount << " chars for line " << lineNo << endl;

			for( int c=0; c < charCount && !stream.atEnd(); c++ )
			{
			    /* Read one line per character */
			    QString charLine = stream.readLine();
			    int semiPos = charLine.find(';');
			    if( semiPos == -1 )
			    {
				kdDebug(28000) << "invalid line: " << charLine << endl;
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
                                int h = results.find(',');  // search the first comma
                                if( h > -1 ) {
                                    // kdDebug(28000) << "Results of count search: " << results.left(h) << endl;
                                    altCount = results.left(h).toInt();
                                    results = results.remove( 0, h+1 ).stripWhiteSpace();
                                } else {
                                    lineErr = true;
                                }
                                // kdDebug(28000) << "Results-line after cutting the alter: " << results << endl;
                                QChar detectedChar = UndetectedChar;
                                if( !lineErr )
                                {
                                    /* take the first alternative only FIXME */
                                    if( altCount > 0 )
                                        detectedChar = results[1];
                                    // kdDebug(28000) << "Found " << altCount << " alternatives for "
                                    //	       << QString(detectedChar) << endl;
                                }

                                /* Analyse the rectangle */
                                if( ! lineErr && detectedChar != ' ' )
                                {
                                    // kdDebug(28000) << "STRING: " << rectStr << "<" << endl;
                                    rx.setPattern( "(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)");
                                    if( rx.search( rectStr ) != -1 )
                                    {
                                        /* unite the rectangles */
                                        QRect privRect( rx.cap(1).toInt(), rx.cap(2).toInt(),
                                                        rx.cap(3).toInt(), rx.cap(4).toInt() );
                                        word.setRect( word.rect() | privRect );
                                    }
                                    else
                                    {
                                        kdDebug(28000) << "ERR: Unable to read rect info for char!" << endl;
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
                                            r.moveBy( blockRect.x(), blockRect.y());
                                        }

					word.setRect( r );
                                        ocrLine.append( word );
                                        word = ocrWord();
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
                                r.moveBy( blockRect.x(), blockRect.y());
                            }
			    word.setRect( r );

			    ocrLine.append( word );
			}
			if( lineNo < m_ocrPage.size() )
			{
			    kdDebug(28000) << "Store result line no " << lineNo << "=\"" <<
                                ocrLine.first() << "..." << endl;
			    m_ocrPage[lineNo] = ocrLine;
			    lineNo++;
			}
			else
			{
			    kdDebug(28000) << "ERR: line index overflow: " << lineNo << endl;
			}
		    }
                    else
                    {
                        kdDebug(28000) << "ERR: Unknown line found: " << line << endl;
                    }
		}
		else
		{
		    kdDebug(28000) << "Unknown line: " << line << endl;
		}
	    }  /* is a comment? */

        }
        file.close();


    return (QString::null);				// no error detected
}
