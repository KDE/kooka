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

#include "abstractocrengine.h"


class OcrOcradEngine : public AbstractOcrEngine
{
    Q_OBJECT

public:
    OcrOcradEngine(QObject *pnt, const QVariantList &args);
    virtual ~OcrOcradEngine() = default;

    AbstractOcrDialogue *createOcrDialogue(AbstractOcrEngine *plugin, QWidget *pnt) override;

protected:
    QStringList tempFiles(bool retain) override;

protected slots:
    void slotOcradExited(int exitCode, QProcess::ExitStatus exitStatus);

private:
    AbstractOcrEngine::EngineStatus startOcrProcess(AbstractOcrDialogue *dia, const KookaImage *img) override;
    QString readORF(const QString &fileName);

private:
    QString m_ocrImagePBM;
    QString m_tempOrfName;
    QString m_tempStdoutLog;
    QString m_tempStderrLog;

    int ocradVersion;
};

#endif                          // OCROCRADENGINE_H
