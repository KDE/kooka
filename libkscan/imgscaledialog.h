/* This file is part of the KDE Project
   Copyright (C) 1999 Klaas Freitag <freitag@suse.de>  

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

#ifndef __IMGSCALEDIALOG_H__
#define __IMGSCALEDIALOG_H__

#include <qlineedit.h>
#include <kdialogbase.h>


/* ----------------------------------------------------------------------
 * The ImgScaleDialg is a small dialog to be used by the image canvas. It
 * allows the user to select a zoom factor in percent, either in steps
 * or as a custom value.
 */
class ImgScaleDialog : public KDialogBase
{
   Q_OBJECT
   Q_PROPERTY( int selected READ getSelected WRITE setSelValue )
      
public:
   ImgScaleDialog( QWidget *parent, int curr_sel = 100,
		   const char *name = 0 );

public slots:
   void enableAndFocus( bool b )
   {
      leCust->setEnabled( b ); leCust->setFocus();
   }

   void setSelValue( int val );
   int  getSelected() const;

signals:
   void customScaleChange( int );
public slots:
   void customChanged( const QString& );
private:
   QLineEdit *leCust;
   int selected;

   class ImgScaleDialogPrivate;
   ImgScaleDialogPrivate *d;
};

#endif
