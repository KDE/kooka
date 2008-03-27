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

#include "config.h"

#include <kdebug.h>
#include <kmessagebox.h>
#include <kapplication.h>
#include <kprocess.h>
#include <kspell.h>
#include <kspelldlg.h>
#include <ksconfig.h>
#include <kguiitem.h>
#include <klocale.h>
#include <kdialogbase.h>

#include <qpen.h>
#include <qvbox.h>

#include "img_canvas.h"

#include "kookaimage.h"
#include "kookapref.h"

#include "ocrbasedialog.h"
#include "ocrkadmosdialog.h"
#include "ocrocraddialog.h"
#include "ocrgocrdialog.h"
#include "ocrword.h"
#include "ocrgocrengine.h"
#include "ocrocradengine.h"
#ifdef HAVE_KADMOS
#include "kadmosocr.h"
#include "ocrkadmosengine.h"
#endif

#include "ocrengine.h"
#include "ocrengine.moc"


#define HIDE_BASE_DIALOG "hideOCRDialogWhileSpellCheck"


OcrEngine::OcrEngine(QWidget *parent)
    : daemon(NULL),
      m_unlinkORF(true),
      m_ocrProcessDia(NULL),
      visibleOCRRunning(false),
      m_resultImage(0),
      m_imgCanvas(0L),
      m_spell(0L),
      m_wantKSpell(true),
      m_kspellVisible(true),
      m_hideDiaWhileSpellcheck(true),
      m_spellInitialConfig(NULL),
      m_ocrCurrLine(0),
      m_currHighlight(-1),
      m_applyFilter(false)
{
    m_introducedImage = NULL;
    m_ocrResultImage = QString::null;
    m_parent = NULL;

    /*
     * a initial config is needed as a starting point for the config dialog
     * but also for ocr without visible dialog.
     */
    m_spellInitialConfig = new KSpellConfig(NULL,NULL,NULL,false);

    KConfig *konf = KGlobal::config();
    konf->setGroup(CFG_GROUP_OCR_DIA);			// OCR dialog information
    m_hideDiaWhileSpellcheck = konf->readBoolEntry( HIDE_BASE_DIALOG, true );
    m_unlinkORF = konf->readBoolEntry( CFG_OCR_CLEANUP, true );

    m_blocks.resize(1);					// there is at least one block
}


OcrEngine::~OcrEngine()
{
   if (daemon!=NULL) delete daemon;
   if (m_introducedImage!=NULL) delete m_introducedImage;
   if (m_spellInitialConfig!=NULL) delete m_spellInitialConfig;
}



OcrEngine *OcrEngine::createEngine(QWidget *parent,bool *gotoPrefs)
{
    KConfig *konf = KGlobal::config();
    konf->setGroup(CFG_GROUP_OCR_DIA);

    OcrEngine::EngineType eng = static_cast<OcrEngine::EngineType>(konf->readNumEntry(CFG_OCR_ENGINE2,OcrEngine::EngineNone));
    kdDebug(28000) << k_funcinfo << "configured OCR engine is " << eng << " = " << engineName(eng) << endl;

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

default:			kdDebug(28000) << "Cannot create engine of type " << eng << endl;
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

    KConfig *konf = KGlobal::config();
    konf->setGroup(CFG_GROUP_OCR_DIA);
    OcrEngine::EngineType confEngine = static_cast<OcrEngine::EngineType>(konf->readNumEntry(CFG_OCR_ENGINE2,OcrEngine::EngineNone));

    kdDebug(28000) << k_funcinfo << "cur=" << curEngine << " conf=" << confEngine << endl;
    return (curEngine==confEngine);
}



/*
 * This is called to introduce a new image, usually if the user clicks on a
 * new image either in the gallery or on the thumbnailview.
 */
void OcrEngine::setImage(const KookaImage *img)
{
   if( ! img ) return           ;

   if( m_introducedImage )
       delete m_introducedImage;

   // FIXME: copy all the image is bad.
   m_introducedImage = new KookaImage(*img);

   if( m_ocrProcessDia )
   {
       m_ocrProcessDia->introduceImage( m_introducedImage );
   }

   m_applyFilter = false;
}


/*
 * starts visual ocr process. Depending on the ocr engine, this function creates
 * a new dialog, and shows it.
 */
bool OcrEngine::startOCRVisible(QWidget *parent)
{
    if (visibleOCRRunning)
    {
        KMessageBox::sorry(NULL,i18n("An OCR process is already running"));
        return (false);
    }

    m_parent = parent;

    m_ocrProcessDia = createOCRDialog(parent,m_spellInitialConfig);
    if (m_ocrProcessDia==NULL) return (false);

    m_ocrProcessDia->setupGui();
    m_ocrProcessDia->introduceImage(m_introducedImage);
    visibleOCRRunning = true;

    connect( m_ocrProcessDia,SIGNAL(user1Clicked()),SLOT(startOCRProcess()));
    connect( m_ocrProcessDia,SIGNAL(closeClicked()),SLOT(slotClose()));
    connect( m_ocrProcessDia,SIGNAL(user2Clicked()),SLOT(slotStopOCR()));
    m_ocrProcessDia->show();

    return (true);
}

/**
 * This method should be called by the engine specific finish slots.
 * It does the not engine dependant cleanups like re-enabling buttons etc.
 */

void OcrEngine::finishedOCRVisible( bool success )
{
    bool doSpellcheck = m_wantKSpell;

    if (m_ocrProcessDia)
    {
        m_ocrProcessDia->stopOCR();
        doSpellcheck = m_ocrProcessDia->wantSpellCheck();
    }

    if (success)
    {
        QString goof = ocrResultText();
        emit newOCRResultText(goof);

        if (m_imgCanvas!=NULL)
        {
            if (m_resultImage!=NULL) delete m_resultImage;

            m_resultImage = new QImage(m_ocrResultImage);
            kdDebug(28000) << k_funcinfo << "Result image " << m_ocrResultImage
                           << " dimensions " << m_resultImage->width()
                           << "x" << m_resultImage->height() << endl;

            /* The image canvas is present. Set it to our image */
            m_imgCanvas->newImageHoldZoom(m_resultImage);
            m_imgCanvas->setReadOnly(true);

            /* now handle double clicks to jump to the word */
            m_applyFilter=true;
        }

        /* now it is time to invoke the dictionary if required */
        emit readOnlyEditor(false);
        if (doSpellcheck) performSpellCheck();

        delete m_ocrProcessDia;				// close the dialogue
    }
    m_ocrProcessDia = NULL;				// no longer using dialogue

    visibleOCRRunning = false;
    cleanUpFiles();

    kdDebug(28000) << "# OCR finished #" << endl;
}



void OcrEngine::performSpellCheck()
{
   kdDebug(28000) << k_funcinfo << endl;

   KDialogBase *soloDialog = NULL;			// standalone spell check options
							// Don't make this automatic in
							// the 'if' block below!  When it
   							// gets deleted, its grandchild - the
							// KSpellConfig - gets deleted too,
							// so leaving 'spconf' as a
							// dangling pointer...
   KSpellConfig *spconf;
   if (m_ocrProcessDia==NULL)				// not called by OCR dialogue
   {							// get spell check options
       soloDialog = new KDialogBase(m_parent,NULL,true,i18n("OCR Spell Check Options"),
                                    KDialogBase::Ok+KDialogBase::Cancel,
                                    KDialogBase::Ok,true);
       soloDialog->setButtonOK(KGuiItem(i18n("Start Spell Check")));

       QVBox *vb = soloDialog->makeVBoxMainWidget();
       spconf = new KSpellConfig(vb,NULL,NULL,false);

       if (!soloDialog->exec()) return;
       soloDialog->hide();
   }
   else spconf = m_ocrProcessDia->spellConfig();	// options from OCR dialogue

   m_ocrCurrLine = 0;					// start at beginning of text
   KSpell *speller = new KSpell(m_parent,i18n("OCR Spell Check"),
                                this,SLOT(slSpellReady(KSpell*)),spconf);
   connect(speller,SIGNAL(death()),SLOT(slSpellDead()));
							// begin spell checking
   if (soloDialog!=NULL) delete soloDialog;		// finished with the dialogue
}



/*
 * starting the spell check on line m_ocrCurrLine if the line exists.
 * If not, the function returns.
 */
void OcrEngine::startLineSpellCheck()
{
    if( m_ocrCurrLine < m_ocrPage.size() )
    {
        m_checkStrings = (m_ocrPage[m_ocrCurrLine]).stringList();

        /* In case the checklist is empty, call the result slot immediately */
        if( m_checkStrings.count() == 0 )
        {
            slCheckListDone(false);
	    return;
        }

        kdDebug(28000)<< "Wordlist (size " << m_ocrPage[m_ocrCurrLine].count() << ", line " << m_ocrCurrLine << "):" << m_checkStrings.join(", ") << endl;

        // if( list.count() > 0 )

        m_spell->checkList( &m_checkStrings, m_kspellVisible );
	kdDebug(28000)<< "Started!" << endl;
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
        kdDebug(28000) << k_funcinfo <<" -- no more lines !" << endl;
        m_spell->cleanUp();
    }
}


/* Called by "Close" used while OCR is not in progress */
void OcrEngine::slotClose()
{
    kdDebug(28000) << k_funcinfo << "close dialogue" << endl;
    if (daemon!=NULL && daemon->isRunning())
    {
        kdDebug(28000) << "Killing daemon" << endl;
        daemon->kill(9);
    }

    finishedOCRVisible(false);
}


/* Called by "Cancel" used while OCR is in progress */
void OcrEngine::slotStopOCR()
{
    kdDebug(28000) << k_funcinfo << "stop OCR" << endl;
    if( daemon && daemon->isRunning() )
    {
        kdDebug(28000) << "Killing daemon" << endl;
        daemon->kill(9);
        KMessageBox::error(m_parent,i18n("The OCR process was stopped"));
    }

    finishedOCRVisible(false);
}


/* This slot is fired if the user clicks on the "Start" button of the GUI */
void OcrEngine::startOCRProcess()
{
    if (m_ocrProcessDia==NULL) return;

    m_ocrProcessDia->startOCR();			// start the animation,
							// set fields disabled
    kapp->processEvents();

    m_ocrResultText = "";
    startProcess(m_ocrProcessDia,m_introducedImage);
}




/*
 * Assemble the result text
 */
QString OcrEngine::ocrResultText()
{
    QString res;
    const QString space(" ");

    /* start from the back and search the original word to replace it */
    QValueVector<ocrWordList>::iterator pageIt;

    for( pageIt = m_ocrPage.begin(); pageIt != m_ocrPage.end(); ++pageIt )
    {
        /* thats goes over all lines */
        QValueList<ocrWord>::iterator lineIt;
        for( lineIt = (*pageIt).begin(); lineIt != (*pageIt).end(); ++lineIt )
        {
            res += space + *lineIt;
        }
        res += "\n";
    }
    kdDebug(28000) << "Returning result String  " << res << endl;
    return res;
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

	    m_imgCanvas->viewportToContents( mev->x(), mev->y(),
					     x, y );

            kdDebug(28000) << "Clicked to " << x << "/" << y << ", scale " << scale << endl;
            if( scale != 100 )
            {
                // Scale is e.g. 50 that means tha the image is only half of size.
                // thus the clicked coords must be multiplied with 2
                y = int(double(y)*100/scale);
                x = int(double(x)*100/scale);
            }
            /* now search the word that was clicked on */
            QValueVector<ocrWordList>::iterator pageIt;

	    int line = 0;
	    bool valid = false;
	    ocrWord wordToFind;

            for( pageIt = m_ocrPage.begin(); pageIt != m_ocrPage.end(); ++pageIt )
            {
                QRect r = (*pageIt).wordListRect();

                if( y > r.top() && y < r.bottom() )
                {
		   kdDebug(28000)<< "It is in between " << r.top() << "/" << r.bottom()
				 << ", line " << line << endl;
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
	       ocrWordList words = *pageIt;
	       ocrWordList::iterator wordIt;

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
	       kdDebug(28000) << "Found the clicked word " << wordToFind << endl;
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
void OcrEngine::slSpellCorrected( const QString& originalword,
                                 const QString& newword,
                                 unsigned int pos )
{
    kdDebug(28000) << "Corrected: Original Word " << originalword << " was corrected to "
                   << newword << ", pos ist " << pos << endl;

    kdDebug(28000) << "Dialog state is " << m_spell->dlgResult() << endl;

    if( slUpdateWord( m_ocrCurrLine, pos, originalword, newword ) )
    {
        if( m_imgCanvas && m_currHighlight > -1 )
        {
            if( m_applyFilter )
                m_imgCanvas->removeHighlight( m_currHighlight );
        }
        else
        {
            kdDebug(28000) << "No highlighting to remove!" << endl;
        }
    }

}


void OcrEngine::slSpellIgnoreWord( const QString& word )
{
    ocrWord ignoreOCRWord;

    ignoreOCRWord = ocrWordFromKSpellWord( m_ocrCurrLine, word );
    if( ! ignoreOCRWord.isEmpty() )
    {
        emit ignoreWord( m_ocrCurrLine, ignoreOCRWord );

        if( m_imgCanvas && m_currHighlight > -1 )
        {
            m_imgCanvas->removeHighlight( m_currHighlight );

            /* create a new highlight. That will never be removed */
            QBrush brush;
            QPen pen( gray, 1 );
            QRect r = ignoreOCRWord.rect();
            r.moveBy(0,2);  // a bit offset to the top

            if( m_applyFilter )
                m_imgCanvas->highlight( r, pen, brush );
        }
    }
}

ocrWord OcrEngine::ocrWordFromKSpellWord( int line, const QString& word )
{
    ocrWord resWord;
    if( lineValid(line) )
    {
        ocrWordList words = m_ocrPage[line];

        words.findFuzzyIndex( word, resWord );
    }

    return resWord;
}


bool OcrEngine::lineValid(int line) const
{
    return (line>=0 && ((uint) line)<m_ocrPage.count());
}


void OcrEngine::slMisspelling( const QString& originalword, const QStringList& suggestions,
                              unsigned int pos )
{
    /* for the first try, use the first suggestion */
    ocrWord s( suggestions.first());
    kdDebug(28000) << "Misspelled: " << originalword << " at position " << pos << endl;

    int line = m_ocrCurrLine;
    m_currHighlight = -1;

    // ocrWord resWord = ocrWordFromKSpellWord( line, originalword );
    ocrWordList words = m_ocrPage[line];
    ocrWord resWord;
    kdDebug(28000) << "Size of wordlist (line " << line << "): " << words.count() << endl;

    if( pos < words.count() )
    {
        resWord = words[pos];
    }

    if( ! resWord.isEmpty() )
    {
        QBrush brush;
        brush.setColor( QColor(red)); // , "Dense4Pattern" );
        brush.setStyle( Qt::Dense4Pattern );
        QPen pen( red, 2 );
        QRect r = resWord.rect();

        r.moveBy(0,2);  // a bit offset to the top

        if( m_applyFilter )
            m_currHighlight = m_imgCanvas->highlight( r, pen, brush, true );

        kdDebug(28000) << "Position ist " << r.x() << ", " << r.y() << ", width: "
		       << r.width() << ", height: " << r.height() << endl;

        /* draw a line under the word to check */

        /* copy the source */
        emit repaintOCRResImage();
    }
    else
    {
        kdDebug(28000) << "Could not find the ocrword for " << originalword << endl;
    }

    emit markWordWrong( line, resWord );
}

/*
 * This is the global starting point for spell checking of the ocr result.
 * After the KSpell object was created in method finishedOCRVisible, this
 * slot is called if the KSpell-object feels itself ready for operation.
 * Coming into this slot, the spelling starts in a line by line manner
 */
void OcrEngine::slSpellReady( KSpell *spell )
{
    m_spell = spell;
    connect ( m_spell, SIGNAL( misspelling( const QString&, const QStringList&,
                                            unsigned int )),
              this, SLOT( slMisspelling(const QString& ,
                                        const QStringList& ,
                                        unsigned int  )));
    connect( m_spell, SIGNAL( corrected ( const QString&, const QString&, unsigned int )),
             this, SLOT( slSpellCorrected( const QString&, const QString&, unsigned int )));

    connect( m_spell, SIGNAL( ignoreword( const QString& )),
             this, SLOT( slSpellIgnoreWord( const QString& )));

    connect( m_spell, SIGNAL( done(bool)), this, SLOT(slCheckListDone(bool)));

    kdDebug(28000) << "Spellcheck available" << endl;

    if( m_ocrProcessDia && m_hideDiaWhileSpellcheck )
        m_ocrProcessDia->hide();
    emit readOnlyEditor( true );
    startLineSpellCheck();
}

/**
 * slot called after either the spellcheck finished or the KSpell object found
 * out that it does not want to run because of whatever problems came up.
 * If it is an KSpell-init problem, the m_spell variable is still zero and
 * Kooka pops up a warning.
 */
void OcrEngine::slSpellDead()
{
    if( ! m_spell )
    {
        kdDebug(28000) << "Spellcheck NOT available" << endl;
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
            kdDebug(28000) << "KSpell cleans up" << endl;
        }
        else if( m_spell->status() == KSpell::Finished )
        {
            kdDebug(28000) << "KSpell finished" << endl;
        }
        else if( m_spell->status() == KSpell::Error )
        {
            kdDebug(28000) << "KSpell finished with Errors" << endl;
        }
        else if( m_spell->status() == KSpell::Crashed )
        {
            kdDebug(28000) << "KSpell Chrashed" << endl;
        }
        else
        {
            kdDebug(28000) << "KSpell finished with unknown state!" << endl;
        }

        /* save the current config */
        delete m_spell;
        m_spell = 0L;

        /* reset values */
        m_checkStrings.clear();
        m_ocrCurrLine = 0;
        if( m_imgCanvas && m_currHighlight > -1 )
            m_imgCanvas->removeHighlight( m_currHighlight );

    }
    if( m_ocrProcessDia )
        m_ocrProcessDia->show();
    emit readOnlyEditor( false );
}


/**
 * This slot reads the current line from the member m_ocrCurrLine and
 * writes the corrected wordlist to the member page word lists
 */
void OcrEngine::slCheckListDone(bool shouldUpdate)
{

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
	kdDebug(28000) << "Starting spellcheck from CheckListDone" << endl;
        startLineSpellCheck();
    }
}

/**
 * updates the word at position spellWordIndx in line line to the new word newWord.
 * The original word was origWord. This slot is called from slSpellCorrected
 *
 */
bool OcrEngine::slUpdateWord( int line, int spellWordIndx, const QString& origWord,
                             const QString& newWord )
{
    bool result = false;

    if( lineValid( line ))
    {
        ocrWordList words = m_ocrPage[line];
        kdDebug(28000) << "Updating word " << origWord << " to " << newWord << endl;

        if( words.updateOCRWord( words[spellWordIndx] /* origWord */, newWord ) )  // searches for the word and updates
        {
            result = true;
            emit updateWord( line, origWord, newWord );
        }
        else
            kdDebug(28000) << "WRN: Update from " << origWord << " to " << newWord << " failed" << endl;

    }
    else
    {
        kdDebug(28000) << "WRN: Line " << line << " no not valid!" << endl;
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
