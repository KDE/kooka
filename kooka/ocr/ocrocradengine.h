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

#ifndef OCROCRADENGINE_H
#define OCROCRADENGINE_H

#include <qprocess.h>

#include "ocrengine.h"

class OcrOcradEngine : public OcrEngine
{
    Q_OBJECT

public:
    explicit OcrOcradEngine(QWidget *parent = Q_NULLPTR);
    ~OcrOcradEngine();

    OcrBaseDialog *createOCRDialog(QWidget *parent);

    OcrEngine::EngineType engineType() const
    {
        return (OcrEngine::EngineOcrad);
    }
    static QString engineDesc();

protected:
    QStringList tempFiles(bool retain);

protected slots:
    void slotOcradExited(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void startProcess(OcrBaseDialog *dia, const KookaImage *img);
    QString readORF(const QString &fileName);

private:
    QString m_ocrImagePBM;
    QString m_tempOrfName;
    QString m_tempStdoutLog;
    QString m_tempStderrLog;

    int ocradVersion;
    bool m_verboseDebug;
};

#endif                          // OCROCRADENGINE_H
