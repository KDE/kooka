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


#ifndef KOCRKADMOS_H
#define KOCRKADMOS_H

#include <kdialogbase.h>
#include <qmap.h>

#include "kocrbase.h"
/**
  *@author Klaas Freitag
  */



class KScanCombo;
class QWidget;
class QButtonGroup;
class KConfig;
class QCheckBox;
class KSpellConfig;
class QRadioButton;

class KadmosClassifier   /* Not yet used FIXME */
{
public:
   KadmosClassifier( QString lang, QString filename );
   QString getCmplFilename() const { return path+filename; }
   QString getFilename()     const { return filename; }
   QString language()        const { return languagesName; }

   void setPath( const QString& p ) { path=p; }
private:

   QString filename;
   QString path;
   QString languagesName;
};


class KadmosDialog: public KOCRBase
{
    Q_OBJECT
public:
    KadmosDialog( QWidget *, KSpellConfig *spellConfig );
    ~KadmosDialog();

    typedef QMap<QString, QString> StrMap;

    EngineError setupGui();
    bool getAutoScale();
    bool getNoiseReduction();
    bool getSelClassifier(QString&) const;
    QString getSelClassifierName() const;

    QString ocrEngineName() const;
    QString ocrEngineDesc() const;
    QString ocrEngineLogo() const;

public slots:
    void enableFields(bool);

protected:
    void writeConfig();

    void setupPreprocessing( QVBox *box );
    void setupSegmentation(  QVBox *box );
    void setupClassification( QVBox *box );

    EngineError findClassifiers();
    EngineError findClassifierPath();
private slots:

    void slFontChanged( int id );

private:
    StrMap                m_classifierTranslate;

    QCheckBox             *m_cbNoise;
    QCheckBox             *m_cbAutoscale;
    QString                m_customClassifierPath;

    QButtonGroup	  *m_bbFont;

    QRadioButton          *m_rbMachine;
    QRadioButton          *m_rbHand;
    QRadioButton          *m_rbNorm;

    QGroupBox      	  *m_gbLang;

    QComboBox             *m_cbLang;

    QStringList            m_ttfClassifier;
    QStringList            m_handClassifier;
    QStringList            m_classifierPath;

    bool                   m_haveNorm;

    typedef QMap<QString, QString> StringMap;
    StringMap m_longCountry2short;
};

#endif
