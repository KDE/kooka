/***************************************************************************
                      kadmosocr.cpp - Kadmos cpp interface
                             -------------------
    begin                : Fri Jun 30 2000

    (c) 2002 re Recognition AG      Hafenstrasse 50b  CH-8280 Kreuzlingen
    Switzerland          Phone: +41 (0)71 6780000  Fax: +41 (0)71 6780099
    Website: www.reRecognition.com         E-mail: info@reRecognition.com

    Author: Tamas Nagy (nagy@rerecognition.com)
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/* Kadmos CPP object oriented interface */

#include <qimage.h>
#include <qpainter.h>

#include <assert.h>

#include <string.h>
#include <malloc.h>
#include <memory.h>

#include "kadmosocr.h"

#include <kdebug.h>



namespace Kadmos {

#ifdef HAVE_KADMOS
#include <qimage.h>
#include <qstring.h>
#include <qthread.h>
#include <qmutex.h>
#include <qrect.h>


#undef  QT_THREAD_SUPPORT

/* CRec */
CRec::CRec()
{
  memset(&m_RecData, 0, sizeof(m_RecData));
  m_Error = RE_SUCCESS;
}

CRec::~CRec()
{
}

KADMOS_ERROR CRec::Init(const char* ClassifierFilename)
{
  strcpy(m_RecData.init.version, INC_KADMOS);
  m_Error = rec_init(&m_RecData, (char*)ClassifierFilename);
  CheckError();
  return m_Error;
}

KADMOS_ERROR CRec::Recognize()
{
  m_Error = rec_do(&m_RecData);
  CheckError();
  return m_Error;
}

KADMOS_ERROR CRec::End()
{
  m_Error = rec_end(&m_RecData);
  CheckError();
  return m_Error;
}

int CRec::GetAlternatives()
{
  int res=0;

  for (int i=0; i<REC_ALT; i++)
  {
    if (m_RecData.rec_char[i][0] != 0) res++;
    else break;
  }

  return res;
}

char CRec::GetResult(int n, int c)
{
  return m_RecData.rec_char[n][c];
}

unsigned int CRec::GetConfidenceValue(int n)
{
  return m_RecData.rec_value[n];
}

KADMOS_ERROR CRec::SetImage(QImage& Image)
{
  memcpy(&m_RecData.image, Image.bits(), Image.numBytes() );

  return RE_SUCCESS;
}

void CRec::LoadParameter(const char* paramfile, const char* section)
{
  unsigned char reject;

  m_Error = re_readparm(&m_RecData.parm, &reject, (char*)paramfile, section);
  CheckError();
}

void CRec::SetNoiseReduction(bool bNoiseReduction)
{
  if (bNoiseReduction) {
    m_RecData.parm.prep |= PREP_AUTO_NOISEREDUCTION;
  }
  else {
    m_RecData.parm.prep &= !PREP_AUTO_NOISEREDUCTION;
  }
}

void CRec::SetScaling(bool bScaling)
{
  if (bScaling) {
    m_RecData.parm.prep |= PREP_SCALING;
  }
  else {
    m_RecData.parm.prep &= !PREP_SCALING;
  }
}

void CRec::CheckError()
{
  if (m_Error!=RE_SUCCESS) {
    re_ErrorText Err;
    re_GetErrorText(&Err);
  }
}

void CRec::ReportError(const char* ErrText, const char* Program)
{
  re_ErrorText Err;
  strcpy(Err.text, ErrText);
  strcpy(Err.program, Program);
}


/* CRel */
CRel::CRel()
{
  memset(&m_RelData, 0, sizeof(m_RelData));
  memset(&m_Result, 0, sizeof(m_Result));
  m_Error = RE_SUCCESS;
}

CRel::~CRel()
{
}

KADMOS_ERROR CRel::Init(const char* ClassifierFilename)
{
  m_RelData.init.rel_grid_maxlen   = GRID_MAX_LEN;
  m_RelData.init.rel_graph_maxlen  = GRAPH_MAX_LEN;
  m_RelData.init.rel_result_maxlen = CHAR_MAX_LEN;
  strcpy(m_RelData.init.version, INC_KADMOS);

  m_Error = rel_init(&m_RelData, (char*)ClassifierFilename);

  m_RelData.rel_grid   = relgrid;
  m_RelData.rel_graph  = relgraph;
  m_RelData.rel_result = relresult;

  CheckError();
  return m_Error;
}

KADMOS_ERROR CRel::Recognize()
{
  m_Error = rel_do(&m_RelData);
  CheckError();
  return m_Error;
}

KADMOS_ERROR CRel::End()
{
  m_Error = rel_end(&m_RelData);
  CheckError();
  return m_Error;
}

const char* CRel::RelTextLine(unsigned char RejectLevel, int RejectChar, long Format)
{
  m_Error = rel_textline(&m_RelData, m_Result, 2*CHAR_MAX_LEN, RejectLevel, RejectChar, Format);
  CheckError();
  return m_Result;
}


KADMOS_ERROR CRel::SetImage(QImage& Image)
{
  memcpy(&m_RelData.image, Image.bits(), Image.numBytes());

  return RE_SUCCESS;
}

void CRel::SetNoiseReduction(bool bNoiseReduction)
{
  if (bNoiseReduction) {
    m_RelData.parm.prep |= PREP_AUTO_NOISEREDUCTION;
  }
  else {
    m_RelData.parm.prep &= !PREP_AUTO_NOISEREDUCTION;
  }
}

void CRel::SetScaling(bool bScaling)
{
  if (bScaling) {
    m_RelData.parm.prep |= PREP_SCALING;
  }
  else {
    m_RelData.parm.prep &= !PREP_SCALING;
  }
}

void CRel::CheckError()
{
  if (m_Error!=RE_SUCCESS) {
    re_ErrorText Err;
    re_GetErrorText(&Err);
  }
}

void CRel::ReportError(const char* ErrText, const char* Program)
{
  re_ErrorText Err;
  strcpy(Err.text, ErrText);
  strcpy(Err.program, Program);
}

/* CRep */
CRep::CRep()
#ifdef QT_THREAD_SUPPORT
    :QThread()
#endif
{
  memset(&m_RepData, 0, sizeof(m_RepData));
  m_Error = RE_SUCCESS;
}

CRep::~CRep()
{
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
    ReportError("Out of memory", CPP_ERROR);
  }
  strcpy(m_RepData.init.version, INC_KADMOS);

  m_Error = rep_init(&m_RepData, (char*)ClassifierFilename);
  CheckError();
  return m_Error;
}

void CRep::run() // KADMOS_ERROR CRep::Recognize()
{
#ifdef QT_THREAD_SUPPORT
    m_mutex.lock();
#endif
    kdDebug() << "ooo Locked and ocr!" << endl;
    m_Error = rep_do(&m_RepData);
    CheckError();
#ifdef QT_THREAD_SUPPORT
    m_mutex.unlock();
#endif
  // return m_Error;
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
  m_Error = rep_textline(&m_RepData, nLine, m_Line, 2*CHAR_MAX_LEN, RejectLevel, RejectChar, Format);
  CheckError();
  return m_Line;
}

void CRep::analyseLine( short line, QPixmap* pix )
{
    Kadmos::RepResult *repRes = &(m_RepData.rep_result[line]);

    if( ! repRes )
    {
        kdDebug() << "repRes-Pointer is null" << endl;
        return;
    }

    /* check if index is in range */
    int resLen = m_RepData.rep_result->rel_result_len;
    if( line < 0 || line > resLen-1 )
    {
        kdDebug() << "Line index is out of range" << endl;
        return;
    }

    /* Handle line box */
    kdDebug() << "RepResult Box at " << repRes->left << ", " << repRes->top << endl;
    kdDebug() << "Size is " << repRes->width << "x" << repRes->height << endl;
    drawLineBox( pix, QRect( repRes->left, repRes->top, repRes->width, repRes->height ));

    Kadmos::RelResult *relr = &(repRes->rel_result[0]);
    Kadmos::RelGraph  *relg = &(repRes->rel_graph[0] );

    for (;relr && relg;)
    {
        relr = &(repRes->rel_result[0]) + relg->result_number[0];

        if (!*relr->rec_char[0]) strcpy(relr->rec_char[0], "~ ");
        // printf("\n   %2s  %d (%d,%d;%d,%d)", relr->rec_char[0], relr->rec_value[0],
        //        (int)relr->left, (int)relr->top, (int)relr->width, (int)relr->height);
        if (relg->leading_blanks) printf("  %d leading blanks", relg->leading_blanks);

        drawCharBox( pix, QRect(repRes->left+relr->left,
                                repRes->top+relr->top,
                                relr->width,
                                relr->height ));

        kdDebug() << "RelGraph->next is " << relg->next[0] << endl;
        if (relg->next[0] == -1) break;
        relg = &(repRes->rel_graph[0]) + relg->next[0];
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


#if 0
void CRep::analyseGraph()
{
    Kadmos::RepResult *repRes = m_RepData.rep_result;
    kdDebug() << "REP-Image: " << repRes->left << ", " << repRes->top << ", "
              << repRes->width << ", " << repRes->height << endl;

    kdDebug() << "Amount of resul items: " << repRes->rel_graph_len << endl;
    kdDebug() << "Amount of graph items: " << repRes->rel_result_len << endl;

    if( repRes )
    {
        for( short line = 0; line < repRes->rel_result_len-1; line ++ )
        {
            analyseLine( line );
        }
    }
    else
    {
        /* undefined result -> hmmm  :-( */
    }
}
#endif

KADMOS_ERROR CRep::SetImage( const QString file )
{
    ReImageHandle image_handle;
    image_handle = re_readimage(file.latin1(), &m_RepData.image);
    if( ! image_handle )
    {
        kdDebug() << "Can not load input file" << endl;
    }
    CheckError();
    return RE_SUCCESS;

}

KADMOS_ERROR CRep::SetImage(QImage *Image)
{
    // memcpy(&m_RepData.image, Image.bits(), Image.numBytes());
#if 0
    /*  does not compile because too few parameters, second param is not
     *  yet documented.
     */
    ReImageHandle h = re_bmp2image( (void*) Image.bits(), 0L,
                                    &m_RepData.image );
#endif
    if( !Image ) return RE_PARAMETERERROR;

    kdDebug() << "Setting image manually." << endl;
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
        kdDebug() << "Setting Binary" << endl;
    } else if( Image->isGrayscale() ) {
        m_RepData.image.color = COLOR_GRAY;
        kdDebug() << "Setting GRAY" << endl;

    } else {
        m_RepData.image.color = COLOR_RGB;
        kdDebug() << "Setting Color RGB" << endl;
    }
    // orientation
    m_RepData.image.orientation = ORIENTATION_TOPLEFT;
    m_RepData.image.photometric = PHOTOMETRIC_MINISWHITE;
    m_RepData.image.resunit = RESUNIT_INCH;
    m_RepData.image.xresolution = 200;
    m_RepData.image.yresolution = 200;

#if 0
    m_RepData.image.subimage.left  = 0;
    m_RepData.image.subimage.top   = 0;
    m_RepData.image.subimage.width = Image->width();
    m_RepData.image.subimage.height  = Image->height();
#endif
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
  if (m_Error!=RE_SUCCESS) {
    re_ErrorText Err;
    re_GetErrorText(&Err);
    kdDebug(28000) << "ERROR: " << Err.text << endl;
  }
}

void CRep::ReportError(const char* ErrText, const char* Program)
{
  re_ErrorText Err;
  strcpy(Err.text, ErrText);
  strcpy(Err.program, Program);
}

#endif /* HAVE_KADMOS */

} /* End of Kadmos namespace */
