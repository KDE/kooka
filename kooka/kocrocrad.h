/***************************************************************************
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


#ifndef KOCROCRAD_H
#define KOCROCRAD_H

#include <kdialogbase.h>
#include <qimage.h>
#include <qstring.h>

#include <kscanslider.h>
#include <kanimwidget.h>

#include "kocrbase.h"

#define CFG_GROUP_OCRAD "ocrad"
#define CFG_OCRAD_LAYOUT_DETECTION "layoutDetection"
#define CFG_OCRAD_EXTRA_ARGUMENTS  "extraArguments"

/**
  *@author Klaas Freitag
  */

class KSpellConfig;
class KURLRequester;
class QComboBox;

class ocradDialog: public KOCRBase
{
    Q_OBJECT
public:
    ocradDialog( QWidget*, KSpellConfig* );
    ~ocradDialog();

    QString getOCRCmd( void ) const
        { return m_ocrCmd;}

    EngineError setupGui();

    QString ocrEngineName() const;
    QString ocrEngineDesc() const;
    QString ocrEngineLogo() const;

    QString orfUrl() const;

    int layoutDetectionMode() const;
    
public slots:
    void enableFields(bool);
    void introduceImage( KookaImage* );

protected:
    void writeConfig();


private:
    QString      m_ocrCmd;
    KURLRequester *m_orfUrlRequester;
    QComboBox      *m_layoutMode;
};

#endif
