/***************************************************************************
                      kadmosocr.h - Kadmos cpp interface
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
#ifndef __KADMOS_OCR_
#define __KADMOS_OCR_

#include <qthread.h>
#include <qobject.h>

#undef QT_THREAD_SUPPORT

#ifdef HAVE_KADMOS
/* class declarations */
class QImage;
class QString;
class QMutex;

namespace Kadmos {

class CRec;
class CRel;
class CRep;

/* include files */
#include "kadmos.h"

/*!
Single character recognition engine interface class
*/
class CRec {
private:
  RecData m_RecData;
  KADMOS_ERROR m_Error;

public:
  CRec();
  ~CRec();

  /*! Starts recognition session
    \param ClassifierFilename is a name of a classifier file (*.rec)
  */
  KADMOS_ERROR Init(const char* ClassifierFilename);

  /*! Recognize character image */
  KADMOS_ERROR Recognize();

  /*! Ends recognition session  */
  KADMOS_ERROR End();

  //! Returns the number of alternatives
  //! \return number of alternatives
  int GetAlternatives();

  /*!
    \param n is the index of result to be returned
    \param c is the index of character to be returned. Valid range is 0..1 .
  */
  char GetResult(int n, int c);
  /*!
    \param n is the index of confidence value to be returned
    \return confidence value in range 0..255. (0 is the highest confidence)
  */
  unsigned int GetConfidenceValue(int n);

  /*!
    \param Image is an image object
  */
  KADMOS_ERROR SetImage(QImage& Image);
    KADMOS_ERROR SetImage(const QString );

  /* Parameters */

  /*! Load parameter set from file */
  void LoadParameter(const char* paramfile, const char* section);

  /*! Save parameter set to file */
  void SaveParameter(const char* paramfile);

  /*! Enable/disable noise reduction
    \param TRUE(enable)/FALSE(disable) noise reduction
  */
  void SetNoiseReduction(bool bNoiseReduction);

  /*! Enable/disable scaling (size normalization)
    \param TRUE(enable)/FALSE(disable) scaling (size normalization)
  */
  void SetScaling(bool bScaling);

private:
  void CheckError();
  void ReportError(const char* ErrText, const char* Program);
};

/* REL */
const int GRID_MAX_LEN  =  50;   //!< Maximum number of grid elements in a line
const int GRAPH_MAX_LEN = 500;   //!< Maximum number of graph elements in a line
const int CHAR_MAX_LEN  = 500;   //!< Maximum number of characters in a line

/* Error handling */
const char CPP_ERROR[] = "Kadmos CPP interface error";

/*!
Line recognition engine interface class
*/
class CRel {
private:
  RelData m_RelData;
  KADMOS_ERROR m_Error;

  /* result buffers */
  RelGrid relgrid[GRID_MAX_LEN];
  RelGraph relgraph[GRAPH_MAX_LEN];
  RelResult relresult[CHAR_MAX_LEN];

  char m_Result[2*CHAR_MAX_LEN];

public:
  CRel();
  ~CRel();

  /*!
    \param ClassifierFilename is a name of a classifier file (*.rec)
  */
  KADMOS_ERROR Init(const char* ClassifierFile);
  KADMOS_ERROR Recognize();
  KADMOS_ERROR End();

  //! Returns the most likely text result
  const char* RelTextLine(unsigned char RejectLevel=128, int RejectChar='~', long Format=TEXT_FORMAT_ANSI);

  /*!
    \param Image is an image object
  */
  KADMOS_ERROR SetImage(QImage& Image);

  /*! Enable/disable noise reduction
    \param TRUE(enable)/FALSE(disable) noise reduction
  */
  void SetNoiseReduction(bool bNoiseReduction);

  /*! Enable/disable scaling (size normalization)
    \param TRUE(enable)/FALSE(disable) scaling (size normalization)
  */
  void SetScaling(bool bScaling);

private:
  void CheckError();
  void ReportError(const char* ErrText, const char* Program);
};

/* ---------------------------------------- REP ---------------------------------------- */
//! Maximum number of lines in a paragraph
const int LINE_MAX_LEN  = 100;

#ifndef _WIN32_WCE
/*!
Paragraph recognition engine interface class
*/

class CRep
#ifdef QT_THREAD_SUPPORT
    : public QThread
#endif
{
private:
    RepData       m_RepData;
    KADMOS_ERROR  m_Error;
    QMutex        m_mutex;

    char m_Line[2*CHAR_MAX_LEN];

public:
    CRep();
    virtual ~CRep();

    /*!
      \param ClassifierFilename is a name of a classifier file (*.rec)
    */
    KADMOS_ERROR Init(const char* ClassifierFile);

    virtual void run();
#ifndef QT_THREAD_SUPPORT
    /* if not having threads, simulate the QThread start method here. */
    void start() { run(); }

    /* finished is always true if no threads are here. */
    bool finished() { return true; }
#endif

    // KADMOS_ERROR Recognize();
    KADMOS_ERROR End();

    /*!
      \param Image is an image object
    */
    KADMOS_ERROR SetImage(QImage* Image);
    KADMOS_ERROR SetImage( const QString );
    int GetMaxLine();
    const char* RepTextLine(int Line, unsigned char RejectLevel=128,
                            int RejectChar='~', long Format=TEXT_FORMAT_ANSI);

    void analyseGraph();
    void analyseLine(short);
    /*! Enable/disable noise reduction
      \param TRUE(enable)/FALSE(disable) noise reduction
    */
    void SetNoiseReduction(bool bNoiseReduction);

    /*! Enable/disable scaling (size normalization)
      \param TRUE(enable)/FALSE(disable) scaling (size normalization)
    */
    void SetScaling(bool bScaling);

private:
    void CheckError();
    void ReportError(const char* ErrText, const char* Program);
};
#endif



} /* End of Kadmos namespace */
#endif
#endif
