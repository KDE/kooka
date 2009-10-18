/* This file is part of the KDE Project
   Copyright (C) 2000 Klaas Freitag <freitag@suse.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef SCANSOURCEDIALOG_H
#define SCANSOURCEDIALOG_H

#include "libkscanexport.h"

#include <kdialog.h>

/**
  *@author Klaas Freitag
  */


// TODO: into class
typedef enum { ADF_OFF, ADF_SCAN_ALONG, ADF_SCAN_ONCE } AdfBehaviour;

class QRadioButton;
class Q3ButtonGroup;

class KScanCombo;


class KSCAN_EXPORT ScanSourceDialog : public KDialog
{
    Q_OBJECT

public:
    ScanSourceDialog(QWidget *parent, const QList<QByteArray> list, AdfBehaviour adfBehave);
   ~ScanSourceDialog();

   // void 	fillWithSources( QStrList *list );
   QString 	getText( void ) const;

   AdfBehaviour 	getAdfBehave( void ) const { return( adf ); }
   int          sourceAdfEntry() const;


public slots:
   void         slotSetSource( const QString &source );

protected slots:
   void        	slotNotifyADF( int );
   void    	slotChangeSource( int );

private:
   KScanCombo    *sources;
   Q3ButtonGroup  *bgroup;
   QRadioButton  *rb0, *rb1;
   AdfBehaviour    adf;
   bool          adf_enabled;
};

#endif							// SCANSOURCEDIALOG_H
