/***************************************************************************
                  ocrresedit.h  - ocr-result edit widget
                             -------------------
    begin                : Fri 12 Feb 2003
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

#ifndef OCRRESEDIT_H
#define OCRRESEDIT_H

#include <ktextedit.h>

class QPoint;
class QRect;

class OcrResEdit : public KTextEdit
{
    Q_OBJECT

public:
    OcrResEdit(QWidget *parent);

public slots:
    void slotSelectWord(const QPoint &pos);

    void slotSaveText();
    void slotSetReadOnly(bool isRO);

signals:
    void highlightWord(const QRect &r);
    void scrollToWord(const QRect &r);

private slots:
    void slotUpdateHighlight();
};

#endif                          // OCRRESEDIT_H
