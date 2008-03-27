/***************************************************** -*- mode:c++; -*- ***
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

#ifndef OCRENGINE_H
#define OCRENGINE_H

#include "config.h"

#include <qobject.h>

#include "ocrword.h"

#define CFG_OCR_ENGINE    "ocrEngine"
#define CFG_OCR_ENGINE2   "ocrEngine2"
#define CFG_OCR_CLEANUP   "unlinkORF"  /* delete orf file? */

/**
  *@author Klaas Freitag
  */

class QRect;
class QPixmap;
class QStringList;

class KProcess;
class KSpell;
class KSpellConfig;

class KookaImage;
class ImageCanvas;
class OcrBaseDialog;


class OcrEngine : public QObject
{
    Q_OBJECT

public:
    enum EngineError					// OCR Engine setup errors
    {
        ENG_ERROR,
        ENG_OK,
        ENG_DATA_MISSING,
        ENG_BAD_SETUP
    };

    enum EngineType					// OCR Engine configured type
    {
        EngineNone,
        EngineGocr,
        EngineOcrad,
        EngineKadmos
    };

    OcrEngine(QWidget *parent);
    virtual ~OcrEngine();

    bool startOCRVisible(QWidget *parent = NULL);
    void performSpellCheck();

    /**
     * @return the current spell config.
     */
    KSpellConfig* ocrSpellConfig() const { return (m_spellInitialConfig); }

    /**
     * Sets an image Canvas that displays the result image of OCR. If this
     * is set to NULL (or never set) no result image is displayed.
     * The OCR fabric passes a new image to the canvas which is a copy of
     * the image to OCR.
     */
    void setImageCanvas(ImageCanvas *canvas);
    void setImage(const KookaImage *img);

    bool engineValid() const;

    virtual OcrEngine::EngineType engineType() const = 0;

    static const QString engineName(OcrEngine::EngineType eng);
    static OcrEngine *createEngine(QWidget *parent,bool *gotoPrefs = NULL);

signals:
    void newOCRResultText(const QString &text);
    void clearOCRResultText();
    void newOCRResultPixmap(const QPixmap &pix);

    /**
     * progress of the ocr process. The first integer is the main progress,
     * the second the sub progress. If there is only on progress, it is the
     * first parameter, the second is always -1 than.
     * Both have a range from 0..100.
     * Note that this signal may not be emitted if the engine does not support
     * progress.
     */
    void ocrProgress(int progress,int subprogress);

    /**
     * select a word in the editor in line line.
     */
    void selectWord(int line,const ocrWord &word);

    /**
     * signal to indicate that a ocr text must be updated due to better results
     * retrieved from spell check. The internal ocr data structure is already
     * updated when this signal is fired.
     *
     * @param line      the line in which the word must be changed (start at 0)
     * @param wordFrom  the original word
     * @param wordTo    the new word(s).
     */
    void updateWord(int line,const QString &wordFrom,const QString &wordTo);

    /**
     * signal to indicate that word word was ignored by the user. This should result
     * in a special coloring in the editor.
     */
    void ignoreWord(int line,const ocrWord &word);

    /**
     * signal that comes if a word is considered to be wrong in the editor.
     * The word should be marked in any way, e.g. with a signal color.
     **/
    void markWordWrong(int line,const ocrWord &word);

    /**
     * signal the tells that the result image was modified.
     */
    void repaintOCRResImage();

    /**
     * indicates that the text editor holding the text that came through
     * newOCRResultText should be set to readonly or not. Can be connected
     * to QTextEdit::setReadOnly directly.
     */
    void readOnlyEditor(bool isReadOnly);

protected:
    /**
     * Event handler to handle the mouse events to the image viewer showing the
     * ocr result image
     */
    bool eventFilter(QObject *object,QEvent *event);

    void finishedOCRVisible(bool success);
    virtual OcrBaseDialog *createOCRDialog(QWidget *parent,KSpellConfig *spellConfig = NULL) = 0;

protected:
    KProcess *daemon;
    QString m_ocrResultImage;
    QString m_ocrResultText;
    KookaImage *m_introducedImage;
    QWidget *m_parent;
    rectList m_blocks;					// dimensions of recognised blocks
    bool m_unlinkORF;

    ocrBlock m_ocrPage;					// wordLists for every line of OCR results
							// one block contains all lines of the page
protected slots:
    void slotClose();
    void slotStopOCR();

    void slSpellReady(KSpell *spell);
    void slSpellDead();

    void slMisspelling(const QString &originalword,const QStringList &suggestions,unsigned int pos);
    void slSpellCorrected(const QString &originalword,const QString &newword,unsigned int pos);
    void slSpellIgnoreWord(const QString &word);

    void slCheckListDone(bool shouldUpdate);
    bool slUpdateWord(int line,int spellWordIndx,const QString &origWord,const QString &newWord);

private:
    virtual void cleanUpFiles() = 0;
    virtual void startProcess(OcrBaseDialog *dia,KookaImage *img) = 0;

    /**
     * checks after a ocr run if the line number exists in the result
     */
    bool lineValid(int line) const;

    /**
     *  Start spell checking on a specific line that is stored in m_ocrCurrLine.
     *  This method starts the spell checking.
     **/
    void startLineSpellCheck();
    ocrWord ocrWordFromKSpellWord(int line,const QString &word);

    QString ocrResultText();

private slots:
    void startOCRProcess();

private:
    OcrBaseDialog *m_ocrProcessDia;
    bool visibleOCRRunning;

    QImage *m_resultImage;
    ImageCanvas *m_imgCanvas;

    KSpell *m_spell;
    bool m_wantKSpell;
    bool m_kspellVisible;
    bool m_hideDiaWhileSpellcheck;
    KSpellConfig *m_spellInitialConfig;

    unsigned m_ocrCurrLine;				// current processed line to speed kspell correction
    QStringList m_checkStrings;

    int m_currHighlight;
    bool m_applyFilter;
};

#endif							// OCRENGINE_H
