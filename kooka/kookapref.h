/***************************************************************************
                          kookapref.h  - Preferences
                             -------------------                                         
    begin                : Sun Jan 16 2000                                           
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
#ifndef KOOKAPREF_H
#define KOOKAPREF_H

#include <kdialogbase.h>
#include <qframe.h>
#include <qcheckbox.h>
#include <qlabel.h>

class KConfig;
class QLabel;
class KIntNumInput;
class KColorButton;
class ImageSelectLine;

#define STARTUP_READ_IMAGE "ReadImageOnStart"


class KookaPreferences : public KDialogBase
{
   Q_OBJECT
public:
   KookaPreferences();

public slots:
   void slotOk( void );
   void slotApply( void );
   void slotDefault( void );

signals:
   void dataSaved();
   
private:
   void setupStartupPage();
   void setupSaveFormatPage();
   void setupThumbnailPage();
   
   QCheckBox *cbNetQuery;
   QCheckBox *cbSkipFormatAsk;
   QCheckBox *cbShowScannerSelection;
   KConfig   *konf;
   QCheckBox *cbReadStartupImage;

   KIntNumInput *m_thumbWidth;
   KIntNumInput *m_thumbHeight;
   KIntNumInput *m_frameWidth;
   ImageSelectLine *m_tileSelector;
   KColorButton *m_colButt1;
   KColorButton *m_colButt2;
};


#endif // KOOKAPREF_H
