/***************************************************************************
                kocrkadmos.h - ocr dialog for KADMOS ocr engine
                             -------------------
    begin                : Sun Jun 11 2000
    copyright            : (C) 2000 by Klaas Freitag
    email                : freitag@suse.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef KOCRKADMOS_H
#define KOCRKADMOS_H

#include <kdialogbase.h>
#include <qimage.h>
#include <qstring.h>
#include <qmap.h>
#include <kscanslider.h>
#include <kanimwidget.h>

#include "kocrbase.h"
/**
  *@author Klaas Freitag
  */



class KScanCombo;
class QWidget;
class KConfig;
class QCheckBox;




class KadmosDialog: public KOCRBase
{
    Q_OBJECT
public:
    KadmosDialog( QWidget *);
    ~KadmosDialog();

    typedef QMap<QString, QString> StrMap;

    void setupGui();
    bool getAutoScale();
    bool getNoiseReduction();
    QString getSelClassifier() const;
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

    void addClassifierCombo( QWidget *addTo, KConfig *conf);

private slots:

private:
    StrMap                m_classifierTranslate;

    KScanCombo            *m_classifierCombo;

    QCheckBox             *m_cbNoise;
    QCheckBox             *m_cbAutoscale;
    QString                m_classifierPath;
};

#endif
