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

#ifndef OCRKADMOSENGINE_H
#define OCRKADMOSENGINE_H

#include "ocrengine.h"

#ifdef HAVE_KADMOS
#include "kadmosocr.h"
#endif

class OcrBaseDialog;

class OcrKadmosEngine : public OcrEngine
{
    Q_OBJECT

public:
    explicit OcrKadmosEngine(QWidget *parent = Q_NULLPTR);
    virtual ~OcrKadmosEngine();

    OcrBaseDialog *createOCRDialog(QWidget *parent) override;

    OcrEngine::EngineType engineType() const override;
    static QString engineDesc();

protected:
    QStringList tempFiles(bool retain) override;

protected slots:
    void slotKadmosResult();

private:
    void startProcess(OcrBaseDialog *dia, const KookaImage *img) override;

private:
    QString m_tmpFile;
#ifdef HAVE_KADMOS
    Kadmos::CRep m_rep;
#endif
};

#endif                          // OCRKADMOSENGINE_H
