/***************************************************************************
                  kocrbase.h  - base dialog for OCR
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


#ifndef KOCRBASE_H
#define KOCRBASE_H

#include <kdialogbase.h>
#include <kio/previewjob.h>
#include <qimage.h>
#include <qstring.h>

#include <kscanslider.h>
#include <kanimwidget.h>
/**
  *@author Klaas Freitag
  */


class KookaImage;
class QHBox;
class QVBox;
class QLabel;
class QSize;

class KOCRBase: public KDialogBase
{
    Q_OBJECT
public:
    KOCRBase( QWidget *, KDialogBase::DialogType face = KDialogBase::Plain );
    ~KOCRBase();

    virtual void setupGui();

    /**
     * @return the name of the ocr engine
     */
    virtual QString ocrEngineName() const { return QString(); }

    /**
     * @return the filename (without path) of the logo of the ocr engine.
     * the logo needs to be installed in $KDEDIR/share/apps/kooka/pics
     */
    virtual QString ocrEngineLogo() const { return QString(); }

    /**
     * @return a description string of the ocr engine
     */
    virtual QString ocrEngineDesc() const { return QString(); }

    QVBox* ocrPage(){ return m_ocrPage; }
    QVBox* imagePage(){ return m_imgPage; }

public slots:
    virtual void stopAnimation();
    virtual void startAnimation();

    virtual void introduceImage( KookaImage* );

    virtual void startOCR();
    virtual void stopOCR();

protected:
    /**
     * This creates a a tab OCR in the dialog and creates a small intro about the
     * ocr engine used.
     * It calls the virtual subs ocrEngineName, ocrEngineLogo and ocrEngineDesc which
     * must return the approbiate values for the engines.
     * @return a pointer to a VBox in which further elements can be layouted
     */
    virtual void ocrIntro();

    /**
     * This creates a a tab Image Info in the dialog and creates a image description
     * about the current image to ocr.
     */
    virtual void imgIntro();

protected slots:
    virtual KAnimWidget* getAnimation(QWidget*);
    virtual void writeConfig();

private slots:
    virtual void slPreviewResult( KIO::Job* );
    virtual void slGotPreview( const KFileItem*, const QPixmap& );

private:
    KAnimWidget  *m_animation;
    QVBox        *m_ocrPage;
    QVBox        *m_imgPage;
    QVBox        *m_metaBox;
    QHBox        *m_imgHBox;
    QLabel       *m_previewPix;
    KookaImage   *m_currImg;
    QSize         m_previewSize;
};

#endif
