/***************************************************************************
                          kocrfindia.h  -  Dialogclass after scanning
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


#ifndef KOCRFINDIA_H
#define KOCRFINDIA_H


#include <qstring.h>
#include <kdialogbase.h>
#include <qimage.h>
#include <qstring.h>

#include <keditcl.h>

/**
  *@author Klaas Freitag
*/

class KOCRFinalDialog: public KDialogBase
{
   Q_OBJECT
public: 
   KOCRFinalDialog( QWidget *, QString );
   ~KOCRFinalDialog();

public slots:
   void fillText( QString );
   void openTextResult( void );
   
protected:

private slots:
   void writeConfig( void );
   
private:
   KEdit *textEdit;
   QImage *resultImg;
   QString ocrTextFile;
};

#endif
