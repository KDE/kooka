/***************************************************************************
                          devselector.h  -  description                              
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


#ifndef DEVSELECTOR_H
#define DEVSELECTOR_H

#include <qlabel.h>
#include <qstring.h>
#include <qstrlist.h>
#include <qstringlist.h>
#include <qcheckbox.h>

#include <qsemimodal.h>
#include <qprogressbar.h>
#include <kdialogbase.h>
#include <qradiobutton.h>
#include <qgroupbox.h>
/**
  *@author Klaas Freitag
  */

/* Configuration-file definitions */
#define GROUP_STARTUP    "Startup"
#define STARTUP_SCANDEV  "ScanDevice"
#define STARTUP_SKIP_ASK "SkipStartupAsk"



class DeviceSelector: public KDialogBase
{
   Q_OBJECT
public: 
   DeviceSelector( QWidget *parent, QStrList&, QStringList& );
   ~DeviceSelector();

   QString getSelectedDevice( void ) const;
   bool    getShouldSkip( void ) const;
   
public slots:

   void setScanSources( QStrList&, QStringList& );
   
private:
   QButtonGroup *selectBox;
   QStringList devices;
   QCheckBox   *cbSkipDialog;
};

#endif
