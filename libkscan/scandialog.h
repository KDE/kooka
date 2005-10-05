/* This file is part of the KDE Project
   Copyright (C)2001 Nikolas Zimmermann <wildfox@kde.org>
                     Klaas Freitag <freitag@suse.de>

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

#ifndef SCAN_H
#define SCAN_H

#include <qimage.h>
#include <kscan.h>

class ScanParams;
class KScanDevice;
class Previewer;
class QSplitter;

class ScanDialog : public KScanDialog
{
   Q_OBJECT

public:
   ScanDialog( QWidget *parent=0, const char *name=0, bool modal=false );
   ~ScanDialog();

   virtual bool setup();

private:
   void createOptionsTab( void );

protected slots:
   void slotFinalImage( QImage *, ImgScanInfo * );
   void slotNewPreview( QImage * );
   void slotScanStart( );
   void slotScanFinished( KScanStat status );
   void slotAcquireStart();

private slots:
   void slotAskOnStartToggle(bool state);
   void slotNetworkToggle( bool state);


   void slotClose();
private:
   ScanParams   *m_scanParams;
   KScanDevice  *m_device;
   Previewer    *m_previewer;
   QImage       m_previewImage;
   bool         good_scan_connect;
   QCheckBox    *cb_askOnStart;
   QCheckBox    *cb_network;
   QSplitter    *splitter;
   class ScanDialogPrivate;
   ScanDialogPrivate *d;
};

class ScanDialogFactory : public KScanDialogFactory
{
public:
   ScanDialogFactory( QObject *parent=0, const char *name=0 );

protected:
   virtual KScanDialog * createDialog( QWidget *parent=0, const char *name=0,
				       bool modal=false );


};

#endif // SCAN_H
