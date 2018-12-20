/***************************************************************************
                kocrkadmos.h - ocr dialog for KADMOS ocr engine
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

#ifndef OCRKADMOSDIALOG_H
#define OCRKADMOSDIALOG_H

#include <qmap.h>

#include "ocrbasedialog.h"

/**
  *@author Klaas Freitag
  */

class QWidget;
class Q3GroupBox;
class QCheckBox;
class QRadioButton;
class QComboBox;
class Q3ButtonGroup;

//class KScanCombo;

class KadmosClassifier   /* Not yet used FIXME */
{
public:
    virtual KadmosClassifier(const QString &lang, const QString &filename);

    QString getCmplFilename() const
    {
        return path + filename;
    }
    QString getFilename()     const
    {
        return filename;
    }
    QString language()        const
    {
        return languagesName;
    }

    void setPath(const QString &p)
    {
        path = p;
    }

private:
    QString filename;
    QString path;
    QString languagesName;
};

class KadmosDialog: public OcrBaseDialog
{
    Q_OBJECT

public:
    KadmosDialog(QWidget *parent);
    virtual ~KadmosDialog();

    typedef QMap<QString, QString> StrMap;

    OcrEngine::EngineError setupGui() override;
    bool getAutoScale();
    bool getNoiseReduction();
    bool getSelClassifier(QString &) const;
    QString getSelClassifierName() const;

    QString ocrEngineName() const override;
    QString ocrEngineDesc() const override;
    QString ocrEngineLogo() const override;

public slots:
    void enableFields(bool) override;

protected:
    void setupPreprocessing(QWidget *box);
    void setupSegmentation(QWidget *box);
    void setupClassification(QWidget *box);

    OcrEngine::EngineError findClassifiers();
    OcrEngine::EngineError findClassifierPath();

protected slots:
    void slotWriteConfig() override;

private slots:
    void slotFontChanged(int id);

private:
    StrMap                m_classifierTranslate;

    QCheckBox             *m_cbNoise;
    QCheckBox             *m_cbAutoscale;
    QString                m_customClassifierPath;

    Q3ButtonGroup     *m_bbFont;

    QRadioButton          *m_rbMachine;
    QRadioButton          *m_rbHand;
    QRadioButton          *m_rbNorm;

    Q3GroupBox            *m_gbLang;

    QComboBox             *m_cbLang;

    QStringList            m_ttfClassifier;
    QStringList            m_handClassifier;
    QStringList            m_classifierPath;

    bool                   m_haveNorm;

    typedef QMap<QString, QString> StringMap;
    StringMap m_longCountry2short;
};

#endif                          // OCRKADMOSDIALOG_H
