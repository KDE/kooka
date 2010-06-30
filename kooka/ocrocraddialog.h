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


#define CFG_GROUP_OCRAD			"ocrad"

#define CFG_OCRAD_LAYOUT_DETECTION	"layoutDetection"
#define CFG_OCRAD_EXTRA_ARGUMENTS	"extraArguments"
#define CFG_OCRAD_FORMAT		"format"
#define CFG_OCRAD_CHARSET		"charset"
#define CFG_OCRAD_FILTER		"filter"
#define CFG_OCRAD_TRANSFORM		"transform"
#define CFG_OCRAD_INVERT		"invert"
#define CFG_OCRAD_THRESHOLD_ENABLE	"thresholdEnable"
#define CFG_OCRAD_THRESHOLD_VALUE	"thresholdValue"


/**
  *@author Klaas Freitag
  */

class QWidget;
class QComboBox;

class KUrlRequester;

class KScanSlider;


class OcrOcradDialog : public OcrBaseDialog
{
    Q_OBJECT

public:
    OcrOcradDialog(QWidget *parent);
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
    int getNumVersion() const	{ return (m_versionNum); }

    QString orfUrl() const;

protected:
    void enableFields(bool enable);

protected slots:
    void slotWriteConfig();

private:
    void getVersion(const QString &bin);
    QStringList getValidValues(const QString &opt);

private:
    QWidget *m_setupWidget;
    KUrlRequester *m_orfUrlRequester;
    QComboBox *m_layoutMode;
    QComboBox *m_characterSet;
    QComboBox *m_filter;
    QComboBox *m_transform;
    QCheckBox *m_invert;
    QCheckBox *m_thresholdEnable;
    KScanSlider *m_thresholdSlider;

    QString m_ocrCmd;
    int m_versionNum;
    QString m_versionStr;
};


#endif							// OCROCRADDIALOG_H
