/***************************************************** -*- mode:c++; -*- ***
                     kocrocrad.h - ocr dialog for ocrad
                             -------------------
    begin                : Tue Jul 15 2003
    copyright            : (C) 2003 by Klaas Freitag
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


#ifndef OCROCRADDIALOG_H
#define OCROCRADDIALOG_H

#include "ocrbasedialog.h"

#define CFG_GROUP_OCRAD "ocrad"
#define CFG_OCRAD_LAYOUT_DETECTION "layoutDetection"
#define CFG_OCRAD_EXTRA_ARGUMENTS  "extraArguments"
#define CFG_OCRAD_FORMAT "format"
#define CFG_OCRAD_CHARSET "charset"


/**
  *@author Klaas Freitag
  */

class KURLRequester;
class QComboBox;

class OcrOcradDialog : public OcrBaseDialog
{
    Q_OBJECT

public:
    OcrOcradDialog(QWidget *parent,KSpellConfig *spellConfig = NULL);
    ~OcrOcradDialog();

    OcrEngine::EngineError setupGui();

    QString ocrEngineName() const;
    QString ocrEngineDesc() const;
    QString ocrEngineLogo() const;

    QString getOCRCmd() const	{ return (m_ocrCmd); }

    /*
     * returns the numeric version of the ocrad program.
     * Attention: This method returns 10 for ocrad v. 0.10 and 8 for ocrad-0.8
     */
    int getNumVersion() const	{ return (m_version); }

    QString orfUrl() const;
    int layoutDetectionMode() const;

protected:
    void enableFields(bool enable);

protected slots:
    void writeConfig();

private:
    void version(const QString &exe);

private slots:
    void slReceiveStdIn( KProcess *proc,char *buffer,int buflen);

private:
    QString         m_ocrCmd;
    KURLRequester  *m_orfUrlRequester;
    QComboBox      *m_layoutMode;
    KProcess       *m_proc;
    int             m_version;
};

#endif							// OCROCRADDIALOG_H