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
#include <qimage.h>
#include <qstring.h>

#include <kscanslider.h>
#include <kanimwidget.h>
/**
  *@author Klaas Freitag
  */


class KookaImage;

class KOCRBase: public KDialogBase
{
    Q_OBJECT
public:
    KOCRBase( QWidget *, KDialogBase::DialogType face = KDialogBase::Plain );
    ~KOCRBase();

    virtual void setupGui(){};
public slots:
    virtual void stopAnimation();
    virtual void startAnimation();
    virtual void enableFields(bool);

    virtual void introduceImage( KookaImage* );

protected slots:
    virtual KAnimWidget* getAnimation(QWidget*);
    virtual void writeConfig();
    virtual void startOCR();

private:
    KAnimWidget *m_animation;
};

#endif
