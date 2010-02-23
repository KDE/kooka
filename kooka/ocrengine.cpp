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

#include "ocrengine.h"
#include "ocrengine.moc"

#include <qpen.h>
#include <qvector.h>
#include <qevent.h>
#include <qfileinfo.h>

#include <kdebug.h>
#include <kmessagebox.h>
#include <kapplication.h>
#include <kprocess.h>
#ifndef KDE4
#include <kspell.h>
#include <kspelldlg.h>
#include <ksconfig.h>
#endif
#include <klocale.h>
#include <kdialog.h>
#include <kvbox.h>

#include <kio/deletejob.h>

#include "libkscan/imagecanvas.h"

#include "kookaimage.h"
#include "kookapref.h"

#include "ocrbasedialog.h"
#include "ocrocraddialog.h"
#include "ocrgocrdialog.h"
#include "ocrword.h"
#include "ocrgocrengine.h"
#include "ocrocradengine.h"
#ifdef HAVE_KADMOS
#include "kadmosocr.h"
#include "ocrkadmosengine.h"
#include "ocrkadmosdialog.h"
#endif



// TODO: Spell checking needs to be ported to Sonnet for KDE4


#define HIDE_BASE_DIALOG "hideOCRDialogWhileSpellCheck"


OcrEngine::OcrEngine(QWidget *parent)
    : m_ocrProcess(NULL),
      m_ocrDialog(NULL),
      m_ocrRunning(false),
      m_resultImage(0),
      m_imgCanvas(NULL),
      m_spell(NULL),
      m_wantKSpell(true),
      m_kspellVisible(true),
      m_hideDiaWhileSpellcheck(true),
      m_spellInitialConfig(NULL),
      m_ocrCurrLine(0),
      m_currHighlight(-1),
      m_applyFilter(false)
{
    m_introducedImage = NULL;
    m_ocrResultFile = QString::null;
    m_parent = NULL;

    /*
     * a initial config is needed as a starting point for the config dialog
     * but also for OCR without visible dialog.
     */
#ifndef KDE4
    m_spellInitialConfig = new KSpellConfig(NULL,NULL,NULL,false);
#endif
							// OCR dialog information
    KConfigGroup grp = KGlobal::config()->group(CFG_GROUP_OCR_DIA);
    m_hideDiaWhileSpellcheck = grp.readEntry(HIDE_BASE_DIALOG, true);

    m_blocks.resize(1);					// there is at least one block
}


OcrEngine::~OcrEngine()
{
   if (m_ocrProcess!=NULL) delete m_ocrProcess;
   if (m_ocrDialog!=NULL) delete m_ocrDialog;
#ifndef KDE4
   if (m_spellInitialConfig!=NULL) delete m_spellInitialConfig;
#endif
}



OcrEngine *OcrEngine::createEngine(QWidget *parent,bool *gotoPrefs)
{
    KConfigGroup grp = KGlobal::config()->group(CFG_GROUP_OCR_DIA);

    OcrEngine::EngineType eng = static_cast<OcrEngine::EngineType>(grp.readEntry(CFG_OCR_ENGINE2,
                                                                                 static_cast<int>(OcrEngine::EngineNone)));
    kDebug() << "configured OCR engine is" << eng << "=" << engineName(eng);

    QString msg = QString::null;
    switch (eng)
    {
case OcrEngine::EngineGocr:	return (new OcrGocrEngine(parent));
case OcrEngine::EngineOcrad:	return (new OcrOcradEngine(parent));

case OcrEngine::EngineKadmos:
#ifdef HAVE_KADMOS
				return (new OcrKadmosEngine(parent));
#else							// HAVE_KADMOS
                                msg = i18n("This version of Kooka is not compiled with KADMOS support.\n"
                                           "Please select another OCR engine in the configuration dialog.");
                                break;
#endif							// HAVE_KADMOS

case OcrEngine::EngineNone:     msg = i18n("No OCR engine is configured.\n"
                                           "Please select and configure one in the OCR configuration dialog.");
                                break;

default:			kDebug() << "Cannot create engine of type" << eng;
				return (NULL);
    }

    if (!msg.isNull())					// failed, tell the user
    {
        int result = KMessageBox::warningContinueCancel(parent,msg,QString::null,
                                                        KGuiItem(i18n("Configure OCR...")));
        if (gotoPrefs!=NULL) *gotoPrefs = (result==KMessageBox::Continue);
    }

    return (NULL);
}


bool OcrEngine::engineValid() const
{
    OcrEngine::EngineType curEngine = engineType();

    KConfigGroup grp = KGlobal::config()->group(CFG_GROUP_OCR_DIA);
    OcrEngine::EngineType confEngine = static_cast<OcrEngine::EngineType>(grp.readEntry(CFG_OCR_ENGINE2,
                                                                                        static_cast<int>(OcrEngine::EngineNone)));

    kDebug() << "cur" << curEngine << "conf" << confEngine;
    return (curEngine==confEngine);
}



/*
 * This is called to introduce a new image, usually if the user clicks on a
 * new image either in the gallery or on the thumbnailview.
 */
void OcrEngine::setImage(const KookaImage *img)
{
   if (img==NULL) return;

   m_introducedImage = img;				// just refer to original

   if (m_ocrDialog!=NULL) m_ocrDialog->introduceImage(m_introducedImage);
   m_applyFilter = false;
}


/*
 * starts visual ocr process. Depending on the ocr engine, this function creates
 * a new dialog, and shows it.
 */
bool OcrEngine::startOCRVisible(QWidget *parent)
{
    if (m_ocrRunning)
    {
        KMessageBox::sorry(parent, i18n("An OCR process is already running"));
        return (false);
    }

    m_parent = parent;

    m_ocrDialog = createOCRDialog(parent, m_spellInitialConfig);
    if (m_ocrDialog==NULL) return (false);

    m_ocrDialog->setupGui();
    m_ocrDialog->introduceImage(m_introducedImage);

    connect(m_ocrDialog, SIGNAL(signalOcrStart()), SLOT(startOCRProcess()));
    connect(m_ocrDialog, SIGNAL(signalOcrStop()), SLOT(slotStopOCR()));
    connect(m_ocrDialog, SIGNAL(signalOcrClose()), SLOT(slotClose()));
    m_ocrDialog->show();

    m_ocrRunning = true;
    return (true);
}

/**
 * This method should be called by the engine specific finish slots.
 * It does the not engine dependant cleanups like re-enabling buttons etc.
 */

void OcrEngine::finishedOCRVisible(bool success)
{
    bool doSpellcheck = m_wantKSpell;

    if (m_ocrDialog!=NULL)
    {
        m_ocrDialog->enableGUI(false);
        doSpellcheck = m_ocrDialog->wantSpellCheck();
    }

    if (success)
    {
        QString goof = ocrResultText();
        emit newOCRResultText(goof);

        if (m_imgCanvas!=NULL)
        {
            if (m_resultImage!=NULL) delete m_resultImage;

            m_resultImage = new QImage(m_ocrResultFile);
            kDebug() << "Result image" << m_ocrResultFile
                     << "dimensions [" << m_resultImage->width()
                     << "x" << m_resultImage->height() << "]";

            /* The image canvas is present. Set it to our image */
            m_imgCanvas->newImageHoldZoom(m_resultImage);
            m_imgCanvas->setReadOnly(true);

            /* now handle double clicks to jump to the word */
            m_applyFilter=true;
        }

        /* now it is time to invoke the dictionary if required */
        emit readOnlyEditor(false);
        if (doSpellcheck) performSpellCheck();

    }

    m_ocrDialog->hide();				// close the dialogue

    m_ocrRunning = false;
    removeTempFiles();

    kDebug() << "OCR finished";
}


void OcrEngine::removeTempFiles()
{
    bool retain = m_ocrDialog->keepTempFiles();
    kDebug() << "retain=" << retain;

    const QStringList temps = tempFiles(retain);
    if (retain)
    {
        QString s = i18n("<qt>The following OCR temporary files are retained for debugging:<p>");
        for (QStringList::const_iterator it = temps.constBegin(); it!=temps.constEnd(); ++it)
        {
            KUrl u(*it);
            s += i18n("<filename><a href=\"%1\">%2</a></filename><br>", u.url(), u.pathOrUrl());
        }

        if (KMessageBox::questionYesNo(NULL, s,
                                       i18n("OCR Temporary Files"),
                                       KStandardGuiItem::del(),
                                       KStandardGuiItem::close(),
                                       QString::null,
                                       KMessageBox::AllowLink)==KMessageBox::Yes) retain = false;
    }

    if (!retain)
    {
        for (QStringList::const_iterator it = temps.constBegin(); it!=temps.constEnd(); ++it)
        {
            QString tf = (*it);
            QFileInfo fi(tf);
            if (!fi.exists())				// what happened?
            {
                kDebug() << "does not exist:" << tf;
            }
            else if (fi.isDir())
            {
                kDebug() << "temp dir:" << tf;
                KIO::del(tf, KIO::HideProgressInfo);	// for recursive deletion
            }
            else
            {
                kDebug() << "temp file:" << tf;
                QFile::remove(tf);			// just a simple file
            }
        }
    }
}


void OcrEngine::performSpellCheck()
{
   kDebug();

   KDialog *soloDialog = NULL;			// standalone spell check options
							// Don't make this automatic in
							// the 'if' block below!  When it
   							// gets deleted, its grandchild - the
							// KSpellConfig - gets deleted too,
							// so leaving 'spconf' as a
							// dangling pointer...
   KSpellConfig *spconf;
   if (m_ocrDialog==NULL)				// not called by OCR dialogue
   {							// get spell check options
       soloDialog = new KDialog(m_parent);
       soloDialog->setObjectName("SoloDialog");
       soloDialog->setModal(true);
       soloDialog->setCaption(i18n("OCR Spell Check Options"));
       soloDialog->setButtons(KDialog::Ok|KDialog::Cancel);
       soloDialog->setButtonText(KDialog::Ok, i18n("Start Spell Check"));
       soloDialog->showButtonSeparator(true);

       KVBox *vb = new KVBox(soloDialog);
       soloDialog->setMainWidget(vb);
#ifndef KDE4
       spconf = new KSpellConfig(vb,NULL,NULL,false);
#endif
       if (!soloDialog->exec()) return;
       soloDialog->hide();
   }
   else spconf = m_ocrDialog->spellConfig();	// options from OCR dialogue

   m_ocrCurrLine = 0;					// start at beginning of text
#ifndef KDE4
   KSpell *speller = new KSpell(m_parent,i18n("OCR Spell Check"),
                                this,SLOT(slotSpellReady(KSpell*)),spconf);
   connect(speller,SIGNAL(death()),SLOT(slSpellDead()));
							// begin spell checking
#endif
   if (soloDialog!=NULL) delete soloDialog;		// finished with the dialogue
}



/*
 * starting the spell check on line m_ocrCurrLine if the line exists.
 * If not, the function returns.
 */
void OcrEngine::startLineSpellCheck()
{
#ifndef KDE4
    if( m_ocrCurrLine < m_ocrPage.size() )
    {
        m_checkStrings = (m_ocrPage[m_ocrCurrLine]).stringList();

        /* In case the checklist is empty, call the result slot immediately */
        if( m_checkStrings.count() == 0 )
        {
            slotCheckListDone(false);
	    return;
        }

        kDebug()<< "Wordlist size" << m_ocrPage[m_ocrCurrLine].count()
                << "line" << m_ocrCurrLine << ":" << m_checkStrings.join(", ");

        // if( list.count() > 0 )

        m_spell->checkList( &m_checkStrings, m_kspellVisible );
	kDebug()<< "Started";
        /**
         * This call ends in three slots:
         * 1. slMisspelling:    Hit _before_ the dialog (if any) appears. Time to
         *    mark the wrong word.
         * 2. slSpellCorrected: Hit if the user decided which word to use.
         * 3. slCheckListDone:  The line is finished. The global counter needs to be
         *    increased and this function needs to be called again.
         **/

    }
    else
    {
        kDebug() << "no more lines";
        m_spell->cleanUp();
    }
#endif
}


void OcrEngine::stopOCRProcess(bool tellUser)
{
    if (m_ocrProcess!=NULL && m_ocrProcess->state()==QProcess::Running)
    {
        kDebug() << "Killing daemon";
        m_ocrProcess->kill();
        if (tellUser) KMessageBox::error(m_parent,i18n("The OCR process was stopped"));
    }

    finishedOCRVisible(false);
}


/* Called by "Close" used while OCR is not in progress */
void OcrEngine::slotClose()
{
    kDebug();
    stopOCRProcess(false);
}


/* Called by "Stop" used while OCR is in progress */
void OcrEngine::slotStopOCR()
{
    kDebug();
    stopOCRProcess(true);
}


/* This slot is fired if the user clicks on the "Start" button of the GUI */
void OcrEngine::startOCRProcess()
{
    if (m_ocrDialog==NULL) return;

    m_ocrResultText = "";
    startProcess(m_ocrDialog, m_introducedImage);
}


/*
 * Assemble the result text
 */
QString OcrEngine::ocrResultText()
{
    QString res;
    const QString space(" ");

    /* start from the back and search the original word to replace it */

    for (QVector<OcrWordList>::const_iterator pageIt = m_ocrPage.constBegin();
         pageIt != m_ocrPage.constEnd(); ++pageIt)
    {
        /* thats goes over all lines */
        for (QList<OcrWord>::const_iterator lineIt = (*pageIt).constBegin();
             lineIt != (*pageIt).constEnd(); ++lineIt)
        {
            res += space + (*lineIt);
        }
        res += "\n";
    }

    kDebug() << "Returning result" << res;
    return (res);
}


/* --------------------------------------------------------------------------------
 * event filter to filter the mouse events to the image viewer
 */

void OcrEngine::setImageCanvas( ImageCanvas *canvas )
{
    m_imgCanvas = canvas;

    m_imgCanvas->installEventFilter( this );
}


bool OcrEngine::eventFilter( QObject *object, QEvent *event )
{
    QWidget *w = (QWidget*) object;

    if( m_applyFilter && m_imgCanvas && w == m_imgCanvas )
    {
        if( event->type() == QEvent::MouseButtonDblClick )
        {
            QMouseEvent *mev = static_cast<QMouseEvent*>(event);

            int x = mev->x();
            int y = mev->y();
            int scale = m_imgCanvas->getScaleFactor();

// TODO: is this still needed?
//	    m_imgCanvas->viewportToContents( mev->x(), mev->y(),
//					     x, y );

            kDebug() << "Clicked to [" << x << "," << y << "] scale" << scale;
            if( scale != 100 )
            {
                // Scale is e.g. 50 that means tha the image is only half of size.
                // thus the clicked coords must be multiplied with 2
                y = int(double(y)*100/scale);
                x = int(double(x)*100/scale);
            }

            /* now search the word that was clicked on */
	    int line = 0;
	    bool valid = false;
	    OcrWord wordToFind;

            QVector<OcrWordList>::const_iterator pageIt = m_ocrPage.constBegin();
            for ( ; pageIt != m_ocrPage.constEnd(); ++pageIt )
            {
                QRect r = (*pageIt).wordListRect();

                if( y > r.top() && y < r.bottom() )
                {
		   kDebug() << "It is in between [" << r.top() << "," << r.bottom()
                            << "] line " << line;
		   valid = true;
		   break;
                }
		line++;
            }

	    /*
	     * If valid, we have the line into which the user clicked. Now we
	     * have to find out the actual word
	     */
	    if( valid )
	    {
	       valid = false;
	       /* find the word in the line and mark it */
	       OcrWordList words = *pageIt;
	       OcrWordList::iterator wordIt;

	       for( wordIt = words.begin(); wordIt != words.end() && !valid; ++wordIt )
	       {
		  QRect r = (*wordIt).rect();
		  if( x > r.left() && x < r.right() )
		  {
		     wordToFind = *wordIt;
		     valid = true;
		  }
	       }

	    }

	    /*
	     * if valid, the wordToFind contains the correct word now.
	     */
	    if( valid )
	    {
	       kDebug() << "Found clicked word" << wordToFind;
	       emit selectWord( line, wordToFind );
	    }

            return true;
        }
    }
    return false;
}



/* --------------------------------------------------------------------------------
 * Spellbook support
 */


/*
 * This slot is hit when the checkWord method of KSpell thinks a word is wrong.
 * KSpell detects the correction by itself and delivers it in newword here.
 * To see all alternatives KSpell proposes, slMissspelling must be used.
 */
void OcrEngine::slotSpellCorrected( const QString& originalword,
                                 const QString& newword,
                                 int pos )
{
#ifndef KDE4
    kDebug() << "Corrected: original word" << originalword
             << "was corrected to" << newword
             << "pos" << pos;

    kDebug() << "Dialog state is" << m_spell->dlgResult();

    if( slotUpdateWord( m_ocrCurrLine, pos, originalword, newword ) )
    {
        if( m_imgCanvas && m_currHighlight > -1 )
        {
            if( m_applyFilter )
                m_imgCanvas->removeHighlight( m_currHighlight );
        }
        else
        {
            kDebug() << "No highlighting to remove!";
        }
    }
#endif
}


void OcrEngine::slotSpellIgnoreWord( const QString& word )
{
    OcrWord ignoreOCRWord;

    ignoreOCRWord = OcrWordFromKSpellWord( m_ocrCurrLine, word );
    if( ! ignoreOCRWord.isEmpty() )
    {
        emit ignoreWord( m_ocrCurrLine, ignoreOCRWord );

        if( m_imgCanvas && m_currHighlight > -1 )
        {
            m_imgCanvas->removeHighlight( m_currHighlight );

            /* create a new highlight. That will never be removed */
            QBrush brush;
            QPen pen( Qt::gray, 1 );
            QRect r = ignoreOCRWord.rect();
            r.translate(0,2);  // a bit offset to the top

            if( m_applyFilter )
                m_imgCanvas->highlight( r, pen, brush );
        }
    }
}

OcrWord OcrEngine::OcrWordFromKSpellWord( int line, const QString& word )
{
    OcrWord resWord;
    if( lineValid(line) )
    {
        OcrWordList words = m_ocrPage[line];

        words.findFuzzyIndex( word, resWord );
    }

    return resWord;
}


bool OcrEngine::lineValid(int line) const
{
    return (line>=0 && line<m_ocrPage.count());
}


void OcrEngine::slotMisspelling( const QString& originalword, const QStringList& suggestions,
                              int pos )
{
    /* for the first try, use the first suggestion */
    OcrWord s( suggestions.first());
    kDebug() << "Misspelled:" << originalword << "at" << pos;

    int line = m_ocrCurrLine;
    m_currHighlight = -1;

    // OcrWord resWord = OcrWordFromKSpellWord( line, originalword );
    OcrWordList words = m_ocrPage[line];
    OcrWord resWord;
    kDebug() << "Size of wordlist for line" << line << "is" << words.count();

    if( pos < words.count() )
    {
        resWord = words[pos];
    }

    if( ! resWord.isEmpty() )
    {
        QBrush brush;
        brush.setColor( Qt::red); // , "Dense4Pattern" );
        brush.setStyle( Qt::Dense4Pattern );
        QPen pen( Qt::red, 2 );
        QRect r = resWord.rect();

        r.translate(0,2);  // a bit offset to the top

        if( m_applyFilter )
            m_currHighlight = m_imgCanvas->highlight( r, pen, brush, true );

        kDebug() << "Position [" << r.x() << "," << r.y()
                 << "] width" << r.width() << "height " << r.height();

        /* draw a line under the word to check */

        /* copy the source */
        emit repaintOCRResImage();
    }
    else
    {
        kDebug() << "Could not find the OcrWord for" << originalword;
    }

    emit markWordWrong( line, resWord );
}

/*
 * This is the global starting point for spell checking of the ocr result.
 * After the KSpell object was created in method finishedOCRVisible, this
 * slot is called if the KSpell-object feels itself ready for operation.
 * Coming into this slot, the spelling starts in a line by line manner
 */
void OcrEngine::slotSpellReady( KSpell *spell )
{
#ifndef KDE4
    m_spell = spell;
    connect ( m_spell, SIGNAL( misspelling( const QString&, const QStringList&,
                                            unsigned int )),
              this, SLOT( slotMisspelling(const QString& ,
                                        const QStringList& ,
                                        unsigned int  )));
    connect( m_spell, SIGNAL( corrected ( const QString&, const QString&, unsigned int )),
             this, SLOT( slotSpellCorrected( const QString&, const QString&, unsigned int )));

    connect( m_spell, SIGNAL( ignoreword( const QString& )),
             this, SLOT( slotSpellIgnoreWord( const QString& )));

    connect( m_spell, SIGNAL( done(bool)), this, SLOT(slotCheckListDone(bool)));

    kDebug() << "Spellcheck available";

    if( m_ocrDialog && m_hideDiaWhileSpellcheck )
        m_ocrDialog->hide();
    emit readOnlyEditor( true );
    startLineSpellCheck();
#endif
}

/**
 * slot called after either the spellcheck finished or the KSpell object found
 * out that it does not want to run because of whatever problems came up.
 * If it is an KSpell-init problem, the m_spell variable is still zero and
 * Kooka pops up a warning.
 */
void OcrEngine::slotSpellDead()
{
#ifndef KDE4
    if( ! m_spell )
    {
        kDebug() << "Spellcheck NOT available";
        /* Spellchecking has not yet been existing, thus there is a base problem with
         * spellcheck on this system.
         */
        KMessageBox::error( m_parent,
                            i18n("Spell-checking cannot be started on this system.\n"
                                 "Please check the configuration" ),
                            i18n("Spell-Check") );

    }
    else
    {
        if( m_spell->status() == KSpell::Cleaning )
        {
            kDebug() << "KSpell cleans up";
        }
        else if( m_spell->status() == KSpell::Finished )
        {
            kDebug() << "KSpell finished";
        }
        else if( m_spell->status() == KSpell::Error )
        {
            kDebug() << "KSpell finished with errors";
        }
        else if( m_spell->status() == KSpell::Crashed )
        {
            kDebug() << "KSpell crashed";
        }
        else
        {
            kDebug() << "KSpell finished with unknown state" << m_spell->status();
        }

        /* save the current config */
        delete m_spell;
        m_spell = NULL;

        /* reset values */
        m_checkStrings.clear();
        m_ocrCurrLine = 0;
        if( m_imgCanvas && m_currHighlight > -1 )
            m_imgCanvas->removeHighlight( m_currHighlight );

    }
    if( m_ocrDialog )
        m_ocrDialog->show();
    emit readOnlyEditor( false );
#endif
}


/**
 * This slot reads the current line from the member m_ocrCurrLine and
 * writes the corrected wordlist to the member page word lists
 */
// TODO: never used as slot
void OcrEngine::slotCheckListDone(bool shouldUpdate)
{
#ifndef KDE4
    /*
     * nothing needs to be updated here in the texts, because it is already done
     * in the slSpellCorrected  slot
     */

    /* Check the dialog state here */
    if( m_spell->dlgResult() == KS_CANCEL ||
        m_spell->dlgResult() == KS_STOP )
    {
        /* stop processing */
        m_spell->cleanUp();
    }
    else
    {
        m_ocrCurrLine++;
	kDebug() << "Starting spellcheck";
        startLineSpellCheck();
    }
#endif
}


/**
 * updates the word at position spellWordIndx in line line to the new word newWord.
 * The original word was origWord. This slot is called from slSpellCorrected
 *
 */
bool OcrEngine::slotUpdateWord( int line, int spellWordIndx, const QString& origWord,
                             const QString& newWord )
{
    bool result = false;

    if( lineValid( line ))
    {
        OcrWordList words = m_ocrPage[line];
        kDebug() << "Updating word" << origWord << "->" << newWord;

        if( words.updateOCRWord( words[spellWordIndx] /* origWord */, newWord ) )  // searches for the word and updates
        {
            result = true;
            emit updateWord( line, origWord, newWord );
        }
        else kDebug() << "Update failed!";
    }
    else
    {
        kDebug() << "Line" << line << "not valid!";
    }
    return result;
}



const QString OcrEngine::engineName(OcrEngine::EngineType eng)
{
    switch (eng)
    {
case OcrEngine::EngineNone:	return (i18n("None"));
case OcrEngine::EngineGocr:	return (i18n("GOCR"));
case OcrEngine::EngineOcrad:	return (i18n("OCRAD"));
case OcrEngine::EngineKadmos:	return (i18n("Kadmos"));
default:        		return (i18n("Unknown"));
    }
}
