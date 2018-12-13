/***************************************************** -*- mode:c++; -*- ***
                  kocrbase.h  - base dialog for OCR
                             -------------------
    begin                : Sun Jun 11 2000
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

#ifndef ABSTRACTOCRDIALOGUE_H
#define ABSTRACTOCRDIALOGUE_H

#include <kpagedialog.h>

#include "abstractocrengine.h"


/**
  *@author Klaas Freitag
  */

class QLabel;
class QSize;
class QCheckBox;
class QGroupBox;
class QPushButton;
class QRadioButton;
class QProgressBar;

class KPageWidgetItem;
class KFileItem;

class KookaImage;


class PLUGIN_EXPORT AbstractOcrDialogue : public KPageDialog
{
    Q_OBJECT

public:
    virtual ~AbstractOcrDialogue() = default;

    virtual AbstractOcrEngine::EngineStatus setupGui();
    virtual void introduceImage(const KookaImage *img);

    bool keepTempFiles() const;
    bool verboseDebug() const;

    void enableGUI(bool running);

    bool wantInteractiveSpellCheck() const;
    bool wantBackgroundSpellCheck() const;
    QString customSpellConfigFile() const;

signals:
    void signalOcrStart();
    void signalOcrStop();
    void signalOcrClose();

protected:
    explicit AbstractOcrDialogue(AbstractOcrEngine *plugin, QWidget *pnt = nullptr);

    AbstractOcrEngine *engine() const			{ return (m_plugin); }

    /**
     * Enable or disable dialogue fields.
     *
     * @param enable Whether the dialogue fields should be enabled
     *
     * @note This is called when the OCR process starts with the parameter @p enable
     * set to @c false, and called after OCR has finished and the GUI should accept
     * user input again with the parameter @p enable set to @c true.
     */
    virtual void enableFields(bool enable) = 0;

    QProgressBar *progressBar() const;

    void ocrShowInfo(const QString &binary, const QString &version = QString::null);
    void ocrShowVersion(const QString &version);

    QWidget *addExtraSetupWidget(QWidget *wid = NULL, bool stretchBefore = false);
    QWidget *addExtraEngineWidget(QWidget *wid = NULL, bool stretchBefore = false);
    QWidget *addExtraDebugWidget(QWidget *wid = NULL, bool stretchBefore = false);

protected slots:
    virtual void slotWriteConfig();
    void slotStartOCR();
    void slotCustomSpellDialog();

private:
    void setupSetupPage();
    void setupSourcePage();
    void setupEnginePage();
    void setupSpellPage();
    void setupDebugPage();

    QWidget *addExtraPageWidget(KPageWidgetItem *page, QWidget *wid, bool stretchBefore);

private slots:
    void slotGotPreview(const KFileItem &item, const QPixmap &newPix);
    void stopAnimation();
    void startAnimation();

private:
    AbstractOcrEngine *m_plugin;

    KPageWidgetItem *m_setupPage;
    KPageWidgetItem *m_sourcePage;
    KPageWidgetItem *m_enginePage;
    KPageWidgetItem *m_spellPage;
    KPageWidgetItem *m_debugPage;

    QLabel *m_previewPix;
    QLabel *m_previewLabel;

    QRadioButton *m_rbGlobalSpellSettings;
    QRadioButton *m_rbCustomSpellSettings;
    QPushButton *m_pbCustomSpellDialog;
    QGroupBox *m_gbBackgroundCheck;
    QGroupBox *m_gbInteractiveCheck;

    QSize m_previewSize;
    bool m_wantDebugCfg;                // show the debug options?

    QCheckBox *m_cbRetainFiles;
    QCheckBox *m_cbVerboseDebug;
    bool m_retainFiles;
    bool m_verboseDebug;

    QLabel *m_lVersion;
    QProgressBar *m_progress;
};

#endif							// ABSTRACTOCRDIALOGUE_H
