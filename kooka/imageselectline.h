/***************************************************************************
                imageselectline.h - select a background image.
                             -------------------
    begin                : ?
    copyright            : (C) 2002 by Klaas Freitag
    email                : freitag@suse.de

    $Id$
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#ifndef __IMGSELECTLINE_H__
#define __IMGSELECTLINE_H__

#include <qhbox.h>

/**
 *
 */

class KURL;
class KURLComboBox;
class QPushButton;
class QStringList;

class ImageSelectLine:public QHBox
{
   Q_OBJECT
public:
   ImageSelectLine( QWidget *parent, const QString& text );

   KURL selectedURL() const;
   void setURL( const KURL& );
   void setURLs( const QStringList& );

protected slots:
   void slSelectFile();
   void slUrlActivated( const KURL& );

private:

   KURL m_currUrl;
   KURLComboBox *m_urlCombo;
   QPushButton  *m_buttFileSelect;

};


#endif
