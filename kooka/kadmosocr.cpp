/***************************************************************************
                      kadmosocr.cpp - Kadmos cpp interface
                             -------------------
    begin                : Fri Jun 30 2000

    (c) 2002 re Recognition AG      Hafenstrasse 50b  CH-8280 Kreuzlingen
    Switzerland          Phone: +41 (0)71 6780000  Fax: +41 (0)71 6780099
    Website: www.reRecognition.com         E-mail: info@reRecognition.com

    Author: Tamas Nagy (nagy@rerecognition.com)
            Klaas Freitag <freitag@suse.de>
            Heike Stuerzenhofecker <heike@freisturz.de>
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

/* Kadmos CPP object oriented interface */

#include <qimage.h>
#include <qpainter.h>
#include <qstring.h>
#include <qrect.h>
#include <qstringlist.h>

#include <assert.h>

#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include <kdebug.h>

#include "kadmosocr.h"
#include "ocrword.h"

#ifdef HAVE_KADMOS

using namespace Kadmos;

/* -------------------- CRep -------------------- */
CRep::CRep()
    :QObject()
{
  memset(&m_RepData, 0, sizeof(m_RepData));
  m_Error = RE_SUCCESS;
  m_undetectChar = QChar('_');
}

CRep::~CRep()
{
}

RelGraph* CRep::getGraphKnode(int line, int offset )
{
    Kadmos::RepResult *res = getRepResult(line);
    if( res )
        return ( &(getRepResult(line)->rel_graph[0])+offset);
    else
        return 0L;

}


RepResult* CRep::getRepResult(int line)
{
    if( line<0 || line >= m_RepData.rep_result_len ) return 0L;
    return &(m_RepData.rep_result[line]);
}

RelResult* CRep::getRelResult(int line, RelGraph* graph, int alternative)
{
    if( ! ( graph && getRepResult(line))) return 0L;
    int offset = graph->result_number[alternative];
    return( &(getRepResult(line)->rel_result[0]) + offset );
}


KADMOS_ERROR CRep::Init(const char* ClassifierFilename)
{
    /* prepare RepData structure */
    m_RepData.init.rel_grid_maxlen   = GRID_MAX_LEN;
    m_RepData.init.rel_graph_maxlen  = GRAPH_MAX_LEN;
    m_RepData.init.rel_result_maxlen = CHAR_MAX_LEN;
    m_RepData.init.rep_memory_size   = LINE_MAX_LEN * sizeof(RepResult) +
	(long)LINE_MAX_LEN * CHAR_MAX_LEN * (sizeof(RelGraph)+
					     sizeof(RelResult));
    m_RepData.init.rep_memory = malloc( m_RepData.init.rep_memory_size );
    if (!m_RepData.init.rep_memory) {
	CheckError();
	return m_Error;
    }
    strcpy(m_RepData.init.version, INC_KADMOS);

    m_Error = rep_init(&m_RepData, (char*)ClassifierFilename);
    CheckError();
    return m_Error;
}

void CRep::run() // KADMOS_ERROR CRep::Recognize()
{
    kdDebug(28000) << "ooo Locked and ocr!" << endl;
    m_Error = rep_do(&m_RepData);
    CheckError();
}

KADMOS_ERROR CRep::End()
{
  m_Error = rep_end(&m_RepData);
  CheckError();
  return m_Error;
}

int CRep::GetMaxLine()
{
  return m_RepData.rep_result_len;
}

const char* CRep::RepTextLine(int nLine, unsigned char RejectLevel, int RejectChar, long Format)
{
    m_Error = rep_textline(&m_RepData, nLine, m_Line,
                           2*CHAR_MAX_LEN, RejectLevel, RejectChar, Format);
    CheckError();
    return m_Line;
}

/**
 * This method handles the given line. It takes repRes and goes through the
 * kadmos result tree structures recursivly.
 */
ocrWordList CRep::getLineWords( int line )
{
    ocrWordList repWords;
    bool ok = true;

    Kadmos::RepResult *repRes = getRepResult(line);

    if( ! repRes )
    {
        kdDebug(28000) << "repRes-Pointer is null" << endl;
        ok = false;
    }

    if( ok )
    {
        int nextKnode=0;

        do
        {
            QString resultWord;
            QRect boundingRect;

            int newNextKnode = nextBestWord( line, nextKnode, resultWord, boundingRect );
            boundingRect.moveBy( repRes->left, repRes->top );

            ocrWord newWord;
            newWord = resultWord;
            newWord.setKnode(nextKnode);
            newWord.setLine(line);
            newWord.setRect(boundingRect);
            repWords.push_back(newWord);

            /* set nextKnode to the next Knode */
            nextKnode = newNextKnode;


            // Alternativen:
            // partStrings( line, nextKnode, QString());   // fills m_parts - list with alternative words
            // nextKnode = newNextKnode;
            // kdDebug(28000) << "NextKnodeWord: " << resultWord << endl;
        }
        while( nextKnode > 0 );
    }
    return repWords;
}


/* This fills theWord with the next best word and returns the
 * next knode or 0 if there is no next node
 */
int CRep::nextBestWord( int line, int knode, QString& theWord, QRect& brect )
{

    Kadmos::RelGraph  *relg = getGraphKnode( line, knode );
    // kdDebug(28000) << "GraphKnode is " << knode << endl;
    int nextKnode = knode;

    while( relg )
    {
        Kadmos::RelResult *relr = getRelResult( line, relg, 0 ); // best alternative
        if( relr )
        {
            // kdDebug(28000) << "Leading Blanks: " << relg->leading_blanks <<
            //    " und Knode " << knode << endl;
            char c = relr->rec_char[0][0];
            QChar newChar = c;
            if( c == 0 )
            {
                kdDebug(28000) << "Undetected char found !" << endl;
                newChar = m_undetectChar;
            }

            if ( (nextKnode != knode) && (relg->leading_blanks > 0))
            {
                /* this means the word ends here. */
                // kdDebug(28000) << "----" << theWord << endl;
                relg = 0L; /* Leave the loop. */
            }
            else
            {
                /* append the character */
                theWord.append(newChar);

                /* save the bounding rect */
                // kdDebug(28000) << "LEFT: " << relr->left << " TOP: " << relr->top << endl;
                QRect r( relr->left, relr->top, relr->width, relr->height );

                if( brect.isNull() )
                {
                    brect = r;
                }
                else
                {
                    brect = brect.unite( r );
                }

                /* next knode */
                if( relg->next[0] > 0 )
                {
                    nextKnode = relg->next[0];
                    relg = getGraphKnode( line, nextKnode );
                }
                else
                {
                    /* end of the line */
                    nextKnode = 0;
                    relg = 0L;
                }
            }
        }
    }
    return( nextKnode );
}



void CRep::partStrings( int line, int graphKnode, QString soFar )
{
    /* The following knodes after a word break */
    Kadmos::RelGraph  *relg = getGraphKnode( line, graphKnode );
    // kdDebug(28000) << "GraphKnode is " << graphKnode << endl;

    QString theWord="";
    for( int resNo=0; resNo < SEG_ALT; resNo++ )
    {
        // kdDebug(28000) << "Alternative " << resNo << " is " << relg->result_number[resNo] << endl;
        if( relg->result_number[resNo] == -1 )
        {
            /* This means that there is no other alternative. Go out here. */
            break;
        }

        Kadmos::RelResult *relr = getRelResult( line, relg, resNo );
        theWord = QChar(relr->rec_char[0][0]);

        if ( !soFar.isEmpty() && relg->leading_blanks )
        {
            /* this means the previous words end. */
            // TODO: This forgets the alternatives of _this_ first character of the new word.

            kdDebug(28000) << "---- " << soFar << endl;
            m_parts << soFar;
            break;
        }
        else
        {
            /* make a QString from this single char and append it. */
            soFar += theWord;
        }

        if( relg->next[resNo] > 0 )
        {
            /* There is a follower to this knode. Combine the result list from a recursive call
             * to this function with the follower knode.
             */
            partStrings( line, relg->next[resNo], soFar );
        }
        else
        {
            /* There is no follower */
            kdDebug(28000) << "No followers - theWord is " << soFar << endl;
            m_parts<<soFar;
            break;
        }
    }
}



void CRep::drawCharBox( QPixmap *pix, const QRect& r )
{
    drawBox( pix, r, QColor( Qt::red ));
}

void CRep::drawLineBox( QPixmap* pix, const QRect& r )
{
    drawBox( pix, r, QColor( Qt::blue ));
}

void CRep::drawBox( QPixmap* pix, const QRect& r, const QColor& color )
{
    QPainter p;
    p.begin(pix);

    p.setPen( color );
    p.drawRect(r);
}



KADMOS_ERROR CRep::SetImage( const QString file )
{
    ReImageHandle image_handle;
    image_handle = re_readimage(file.latin1(), &m_RepData.image);
    if( ! image_handle )
    {
        kdDebug(28000) << "Can not load input file" << endl;
    }
    CheckError();
    return RE_SUCCESS;

}

KADMOS_ERROR CRep::SetImage(QImage *Image)
{
    // memcpy(&m_RepData.image, Image.bits(), Image.numBytes());
    if( !Image ) return RE_PARAMETERERROR;

    kdDebug(28000) << "Setting image manually." << endl;
    m_RepData.image.data    = (void*)Image->bits();
    m_RepData.image.imgtype = IMGTYPE_PIXELARRAY;
    m_RepData.image.width   = Image->width();
    m_RepData.image.height  = Image->height();
    m_RepData.image.bitsperpixel = Image->depth();
    m_RepData.image.alignment = 1;
    m_RepData.image.fillorder = FILLORDER_MSB2LSB;
    // color
    if( Image->depth() == 1 || (Image->numColors()==2 && Image->depth() == 8) )
    {
        m_RepData.image.color=COLOR_BINARY;
        kdDebug(28000) << "Setting Binary" << endl;
    } else if( Image->isGrayscale() ) {
        m_RepData.image.color = COLOR_GRAY;
        kdDebug(28000) << "Setting GRAY" << endl;

    } else {
        m_RepData.image.color = COLOR_RGB;
        kdDebug(28000) << "Setting Color RGB" << endl;
    }
    // orientation
    m_RepData.image.orientation = ORIENTATION_TOPLEFT;
    m_RepData.image.photometric = PHOTOMETRIC_MINISWHITE;
    m_RepData.image.resunit = RESUNIT_INCH;
    m_RepData.image.xresolution = 200;
    m_RepData.image.yresolution = 200;

    CheckError();

    return RE_SUCCESS;
}

void CRep::SetNoiseReduction(bool bNoiseReduction)
{
  if (bNoiseReduction) {
    m_RepData.parm.prep |= PREP_AUTO_NOISEREDUCTION;
  }
  else {
    m_RepData.parm.prep &= !PREP_AUTO_NOISEREDUCTION;
  }
}

void CRep::SetScaling(bool bScaling)
{
  if (bScaling) {
    m_RepData.parm.prep |= PREP_SCALING;
  }
  else {
    m_RepData.parm.prep &= !PREP_SCALING;
  }
}

void CRep::CheckError()
{
    if ( kadmosError() )
    {
	kdDebug(28000) << "KADMOS ERROR: " << getErrorText() << endl;
    }
}

/* returns a QString containing the string describing the kadmos error */
QString CRep::getErrorText() const
{
    re_ErrorText Err;
    re_GetErrorText(&Err);
    return QString::fromLocal8Bit( Err.text );
}

bool CRep::kadmosError()
{
    return m_Error != RE_SUCCESS;
}

#include "kadmosocr.moc"

#endif /* HAVE_KADMOS */


// } /* End of Kadmos namespace */
