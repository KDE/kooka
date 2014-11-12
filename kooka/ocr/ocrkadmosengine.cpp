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

#include "ocrkadmosengine.h"

#ifdef QT_THREAD_SUPPORT
#include <qtimer.h>
#endif

#include <QDebug>
#include <KLocalizedString>
#include <ktemporaryfile.h>
#include <kmessagebox.h>

#include "kookaimage.h"
#include "ocrkadmosdialog.h"

#define USE_KADMOS_FILEOP /* use a save-file for OCR instead of filling the reImage struct manually */

/*
 * Thread support is disabled here because the kadmos lib seems not to be
 * thread safe, unfortunately. See slotKadmosResult-comments for more information
 */

OcrKadmosEngine::OcrKadmosEngine(QWidget *parent)
    : OcrEngine(parent)
{
    m_tmpFile = QString::null;
}

OcrKadmosEngine::~OcrKadmosEngine()
{
}

OcrBaseDialog *OcrKadmosEngine::createOCRDialog(QWidget *parent)
{
    //QT5 return (new KadmosDialog(parent));
    return 0;
}

OcrEngine::EngineType OcrKadmosEngine::engineType() const
{
    return (OcrEngine::EngineKadmos);
}

QString OcrKadmosEngine::engineDesc()
{
    return (i18nc("%1 is one of the two following messages",
                  "<qt>"
                  "<p><b>Kadmos</b> is a commercial OCR/ICR library produced by reRecognition&nbsp;AG.</p>"
                  "<p>%1</p>"
                  "<p>See <a href=\"http://www.rerecognition.com\">www.rerecognition.com</a> "
                  "for more information on Kadmos.</p>"
                  "</qt>",
#ifdef HAVE_KADMOS
                  i18n("This version of Kooka is configured to use the Kadmos engine.")
#else
                  i18n("This version of Kooka is not configured for Kadmos.  The Kadmos "
                       "libraries need to be installed, and Kooka needs to be rebuilt with "
                       "the '--with-kadmos' option.")
#endif
                 ));
}

void OcrKadmosEngine::startProcess(OcrBaseDialog *dia, const KookaImage *img)
{
    //qDebug();
#if 0 //QT5
    KadmosDialog *kadDia = static_cast<KadmosDialog *>(dia);

    QString clasPath;                   /* target where the clasPath is written in */
    if (! kadDia->getSelClassifier(clasPath)) {
        KMessageBox::error(m_parent,
                           i18n("The classifier file necessary for OCR cannot be loaded: %1;\n"
                                "OCR with the KADMOS engine is not possible.",
                                clasPath), i18n("KADMOS Installation Problem"));
        finishedOCRVisible(false);
        return;
    }
    QByteArray c = clasPath.toLatin1();

    //qDebug() << "Using classifier" << c;
#ifdef HAVE_KADMOS
    m_rep.Init(c);
    if (m_rep.kadmosError()) { /* check if kadmos initialised OK */
        KMessageBox::error(m_parent,
                           i18n("The KADMOS OCR system could not be started:\n"
                                "%1\n"
                                "Please check the configuration.",
                                m_rep.getErrorText()),
                           i18n("KADMOS Failure"));
    } else {
        /** Since initialising succeeded, we start the ocr here **/
        m_rep.SetNoiseReduction(kadDia->getNoiseReduction());
        m_rep.SetScaling(kadDia->getAutoScale());
        //qDebug() << "Image size [" << img->width() << " x " << img->height() << "]";
        //qDebug() << "Image depth" << img->depth() << "colors" << img->numColors();
#ifdef USE_KADMOS_FILEOP
        KTemporaryFile tmpFile;
        tmpFile.setSuffix(".bmp");
        tmpFile.setAutoRemove(false);

        if (!tmpFile.open()) {
            //qDebug() << "error creating temporary file";
            return;
        }
        m_tmpFile = QFile::encodeName(tmpFile.fileName());

        //qDebug() << "Saving to file" << m_tmpFile;
        img->save(&tmpFile, "BMP");         // save to temp file
        tmpFile.close();
        m_rep.SetImage(tmpFile);
#else                           // USE_KADMOS_FILEOP
        m_rep.SetImage(img);
#endif                          // USE_KADMOS_FILEOP
        m_rep.run();

        /* Dealing with threads or no threads (using QT_THREAD_SUPPORT to distinguish)
         * If threads are here, the recognition task is started in its own thread. The gui thread
         * needs to wait until the recognition thread is finished. Therefore, a timer is fired once
         * that calls slotKadmosResult and checks if the recognition task is finished. If it is not,
         * a new one-shot-timer is fired in slotKadmosResult. If it is, the OCR result can be
         * processed.
         * In case the system has no threads, the method start of the recognition engine does not
         * return until it is ready, the user has to live with a non responsive gui while
         * recognition is performed. The start()-method is implemented as a wrapper to the run()
         * method of CRep, which does the recognition job. Instead of pulling up a timer, simply
         * the result slot is called if start()=run() has finished. In the result slot, finished()
         * is only a dummy always returning true to avoid more preprocessor tags here.
         * Hope that works ...
         * It does not :( That is why it is not used here. Maybe some day...
         */
    }
#endif                          // HAVE_KADMOS
#ifdef QT_THREAD_SUPPORT
    /* start a timer and wait until it fires. */
    QTimer::singleShot(500, this, SLOT(slotKadmosResult()));
#else                           // QT_THREAD_SUPPORT
    slotKadmosResult();
#endif                          // QT_THREAD_SUPPORT
#endif
    //qDebug() << "done";
}

/*
 * This method is called to check if the kadmos process was already finished, if
 * thread support is enabled (check for preprocessor variable QT_THREAD_SUPPORT)
 * The problem is that the kadmos library seems not to be thread stable so thread
 * support should not be enabled by default. In case threads are enabled, this slot
 * checks if the KADMOS engine is finished already and if not it fires a timer.
 */

void OcrKadmosEngine::slotKadmosResult()
{
    //qDebug();

#ifdef HAVE_KADMOS
    if (m_rep.finished()) {
        /* The recognition thread is finished. */
        //qDebug() << "Kadmos is finished";

        m_ocrResultText = "";
        if (! m_rep.kadmosError()) {
            int lines = m_rep.GetMaxLine();
            //qDebug() << "Count lines" << lines;
            m_ocrPage.clear();
            m_ocrPage.resize(lines);

            for (int line = 0; line < m_rep.GetMaxLine(); line++) {
                // ocrWordList wordList = m_rep.getLineWords(line);
                /* call an ocr engine independent method to use the spellbook */
                ocrWordList words = m_rep.getLineWords(line);
                //qDebug() << "Have" << words.count() << "entries in list";
                m_ocrPage[line] = words;
            }

            /* show results of ocr */
            m_rep.End();
        }
        finishedOCRVisible(!m_rep.kadmosError());
    } else {
        /* recognition thread is not yet finished. Wait another half a second. */
        QTimer::singleShot(500, this, SLOT(slotKadmosResult()));
        /* Never comes here if no threads exist on the system */
    }
#endif
}

QStringList OcrKadmosEngine::tempFiles(bool retain)
{
    QStringList result;

#ifdef USE_KADMOS_FILEOP
    if (!m_tmpFile.isNull()) {
        result << m_tmpFile;
        m_tmpFile = QString::null;
    }
#endif

    return (result);
}
