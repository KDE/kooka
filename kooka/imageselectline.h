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
 *  This file may be distributed and/or modified under the terms of the    *
 *  GNU General Public License version 2 as published by the Free Software *
 *  Foundation and appearing in the file COPYING included in the           *
 *  packaging of this file.                                                *
 *
 *  As a special exception, permission is given to link this program       *
 *  with any version of the KADMOS ocr/icr engine of reRecognition GmbH,   *
 *  Kreuzlingen and distribute the resulting executable without            *
 *  including the source code for KADMOS in the source distribution.       *
 *
 *  As a special exception, permission is given to link this program       *
 *  with any edition of Qt, and distribute the resulting executable,       *
 *  without including the source code for Qt in the source distribution.   *
 *                                                                         *
 ***************************************************************************/

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
