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

#ifndef _OCR_RESEDIT_
#define _OCR_RESEDIT_

#include <qtextedit.h>

class QString;
class QColor;
class ocrWord;

class ocrResEdit : public QTextEdit
{
    Q_OBJECT
public:
    ocrResEdit( QWidget  *parent );

public slots:
    void slUpdateOCRResult( int line, const QString& wordFrom,
                            const QString& wordTo );

    void slMarkWordWrong( int line, const ocrWord& word );

    void slIgnoreWrongWord( int line, const ocrWord& word );

    void slSelectWord( int line, const ocrWord& word );

    void slSaveText();

protected slots:
    void slReplaceWord( int line, const QString& wordFrom,
                        const QString& wordTo, const QColor& color );

private:
    QColor      m_updateColor;
    QColor      m_ignoreColor;
    QColor      m_wrnColor;

};

#endif
