/***************************************************************************
                          ksaneocr.h  - ocr-engine class
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

#ifndef KSANEOCR_H
#define KSANEOCR_H
#include <qwidget.h>
#include <qobject.h>

#include "ocrword.h"

#define CFG_OCR_ENGINE    "ocrEngine"
#define CFG_OCR_CLEANUP   "unlinkORF"  /* delete orf file? */

#define CFG_OCR_KSPELL    "ocrSpellSettings"
#define CFG_WANT_KSPELL   "ocrKSpellEnabled"
#define CFG_KS_NOROOTAFFIX  "KSpell_NoRootAffix"
#define CFG_KS_RUNTOGETHER  "KSpell_RunTogether"
#define CFG_KS_DICTIONARY   "KSpell_Dictionary"
#define CFG_KS_DICTFROMLIST "KSpell_DictFromList"
#define CFG_KS_ENCODING     "KSpell_Encoding"
#define CFG_KS_CLIENT       "KSpell_Client"


#define HIDE_BASE_DIALOG "hideOCRDialogWhileSpellCheck"
/**
  *@author Klaas Freitag
  */

class KOCRBase;
class KookaImage;
class KTempFile;
class KProcess;
class QRect;
class QPixmap;
class QStringList;
class KSpell;
class KSpellConfig;
class ImageCanvas;
class KConfig;
// class ocrWord;
// class ocrPage;

#ifdef HAVE_KADMOS
#include "kadmosocr.h"
#endif

/*
 * Error Classifier the report errors on bad engine setup
 */
typedef enum{ ENG_ERROR, ENG_OK, ENG_DATA_MISSING, ENG_BAD_SETUP } EngineError;

class KSANEOCR : public QObject
{
    Q_OBJECT
public:
    enum OCREngines{ GOCR, OCRAD, KADMOS };

    KSANEOCR( QWidget*, KConfig *);
    ~KSANEOCR();

    bool startOCRVisible( QWidget* parent=0);

    void finishedOCRVisible( bool );

    /**
     * checks after a ocr run if the line number exists in the result
     */
    bool lineValid( int line );

#ifdef HAVE_KADMOS
    bool startKadmosOCR();
#endif

    /**
     * return the final ocr result
     */

    QString ocrResultText();

    /**
     * @return the current spell config.
     */
    KSpellConfig* ocrSpellConfig() const
        { return m_spellInitialConfig; }


    /**
     * Sets an image Canvas that displays the result image of ocr. If this
     * is set to zero (or never set) no result image is displayed.
     * The ocr fabric passes a new image to the canvas which is a copy of
     * the image to ocr.
     */
    void setImageCanvas( ImageCanvas* canvas );

signals:
    void newOCRResultText( const QString& );
    void clearOCRResultText();
    void newOCRResultPixmap( const QPixmap& );

    /**
     * progress of the ocr process. The first integer is the main progress,
     * the second the sub progress. If there is only on progress, it is the
     * first parameter, the second is always -1 than.
     * Both have a range from 0..100.
     * Note that this signal may not be emitted if the engine does not support
     * progress.
     */
    void ocrProgress(int, int);

    /**
     * select a word in the editor in line line.
     */
     void selectWord( int line, const ocrWord& word );

    /**
     * signal to indicate that a ocr text must be updated due to better results
     * retrieved from spell check. The internal ocr data structure is already
     * updated when this signal is fired.
     *
     * @param line      the line in which the word must be changed (start at 0)
     * @param wordFrom  the original word
     * @param wordTo    the new word(s).
     */
    void updateWord( int line, const QString& wordFrom, const QString& wordTo );

    /**
     * signal to indicate that word word was ignored by the user. This should result
     * in a special coloring in the editor.
     */
    void ignoreWord( int, const ocrWord& );

    /**
     * signal that comes if a word is considered to be wrong in the editor.
     * The word should be marked in any way, e.g. with a signal color.
     **/
    void markWordWrong( int, const ocrWord& );

    /**
     * signal the tells that the result image was modified.
     */
    void repaintOCRResImage( );

    /**
     * indicates that the text editor holding the text that came through
     * newOCRResultText should be set to readonly or not. Can be connected
     * to QTextEdit::setReadOnly directly.
     */
    void readOnlyEditor( bool );

public slots:
    void slSetImage( KookaImage* );

    void slLineBox( const QRect& );

protected:
    /**
     *  Start spell checking on a specific line that is stored in m_ocrCurrLine.
     *  This method starts the spell checking.
     **/
    void startLineSpellCheck();
    ocrWord ocrWordFromKSpellWord( int line, const QString& word );

    /**
     * Eventhandler to handle the mouse events to the image viewer showing the
     * ocr result image
     */
    bool eventFilter( QObject *object, QEvent *event );

    void startOCRAD();
protected slots:
    void slotClose ();
    void slotStopOCR();

    void slSpellReady( KSpell* );
    void slSpellDead( );
    /**
     * a new list of ocr results of the current ocr process arrived and is available
     * in the member m_ocrPage[line]
     */
    // void gotOCRLine( int line );

    void slMisspelling( const QString& originalword,
                        const QStringList& suggestions,
                        unsigned int pos );
    void slSpellCorrected( const QString& originalword,
                           const QString& newword,
                           unsigned int pos );

    void slSpellIgnoreWord( const QString& word );

    void slCheckListDone( bool );

    bool  slUpdateWord( int line, int spellWordIndx,
                        const QString& origWord,
                        const QString& newWord );

private slots:

    void slotKadmosResult();
    void startOCRProcess( void );
    void gocrStdIn(KProcess*, char* buffer, int buflen);
    void gocrStdErr(KProcess*, char* buffer, int buflen);
    void gocrExited(KProcess*);

    void ocradStdIn(KProcess*, char* buffer, int buflen);
    void ocradStdErr(KProcess*, char* buffer, int buflen);
    void ocradExited(KProcess*);

    /*
     * reads orf files from a file and fills the result structures
     * accordingly.
     */
    bool readORF( const QString&, QString& );

private:
    void     cleanUpFiles( void );


    KOCRBase        *m_ocrProcessDia;
    KProcess        *daemon;
    bool             visibleOCRRunning;
    KTempFile       *m_tmpFile;

    KookaImage      *m_img;
    QString         m_ocrResultText;
    QString         m_ocrResultImage;
    QString         m_ocrImagePBM;
    QString         m_tmpOrfName;
    QImage          *m_resultImage;

    OCREngines      m_ocrEngine;
    QPixmap         m_resPixmap;
    QPixmap         m_storePixmap;

    ImageCanvas     *m_imgCanvas;

    KSpell          *m_spell;
    bool             m_wantKSpell;
    bool             m_kspellVisible;
    bool             m_hideDiaWhileSpellcheck;
    KSpellConfig    *m_spellInitialConfig;

    /* ValueVector of wordLists for every line of ocr results */
    ocrBlock         m_ocrPage; /* one block contains all lines of the page */
    QWidget          *m_parent;
    /* current processed line to speed kspell correction */
    unsigned         m_ocrCurrLine;
    QStringList      m_checkStrings;

    int              m_currHighlight;
    bool             m_applyFilter;

    bool             m_unlinkORF;
    rectList         m_blocks;   // dimensions of blocks

    static char UndetectedChar;
#ifdef HAVE_KADMOS
    Kadmos::CRep   m_rep;
#endif
};

#endif
