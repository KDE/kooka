/***************************************************************************
                          massscandialog.h  -  description                              
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


#ifndef MASSSCANDIALOG_H
#define MASSSCANDIALOG_H

#include <qlabel.h>
#include <qstring.h>
#include <qsemimodal.h>
#include <qprogressbar.h>


/**
  *@author Klaas Freitag
  */

class MassScanDialog : public QSemiModal
{
   Q_OBJECT
public: 
   MassScanDialog( QWidget *parent);
   ~MassScanDialog();
	
public slots:

   void slStartScan( void );
   void slStopScan( void );
   void slFinished( void );	

   void setPageProgress( int p )
      {
	 progressbar->setProgress( p );
      }
		
private:
   QString     scanopts;
   QLabel      *l_scanopts;	

   QString     tofolder;
   QLabel      *l_tofolder;	

   QString     progress;
   QLabel      *l_progress;	

   QProgressBar *progressbar;

   class MassScanDialogPrivate;
   MassScanDialogPrivate *d;
};

#endif
