/***************************************************************************
                          kookaview.h  -  Main view
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
#ifndef KOOKAVIEW_H
#define KOOKAVIEW_H

#include <qwidget.h>
#include <kparts/part.h>
#include <kookaiface.h>
#include <qtabwidget.h>
#include <qlayout.h>
#include <qimage.h>
#include <qsplitter.h>


// application specific includes
#include "kscandevice.h"
#include "previewer.h"
#include "scanpackager.h"
#include "scanparams.h"


class QPainter;
class KSANEOCR;

/**
 * This is the main view class for Kooka.  Most of the non-menu,
 * non-toolbar, and non-statusbar (e.g., non frame) GUI code should go
 * here.
 *
 * @short Main view
 * @author Klaas Freitag <freitag@suse.de>
 * @version 0.1
 */
class KookaView : public QSplitter
{
   Q_OBJECT
public:
   /**
    * Default constructor
    */
   KookaView(QWidget *parent);

   /**
    * Destructor
    */
   virtual ~KookaView();

   /**
    * Print this view to any medium -- paper or not
    */
   void print(QPainter *, int height, int width);

   bool ToggleVisibility( int );
   
public slots:
   void slShowPreview()  { tabw->showPage( preview_canvas ); }
   void slShowPackager() { tabw->showPage( packager ); }
   void slNewPreview( QImage * );

   void slSetScanParamsVisible( bool v )
      { if( v ) scan_params->show(); else scan_params->hide(); }
   void slSetTabWVisible( bool v )
      { if( v ) preview_canvas->show(); else preview_canvas->hide(); }
   
   void doOCR( void );	
   void slStartPreview() { if( scan_params ) scan_params->slAcquirePreview(); }
   void slStartFinalScan() { if( scan_params ) scan_params->slStartScan(); }

   signals:
   /**
    * Use this signal to change the content of the statusbar
    */
   void signalChangeStatusbar(const QString& text);

   /**
    * Use this signal to change the content of the caption
    */
   void signalChangeCaption(const QString& text);


private:
#if 0 
   KParts::ReadOnlyPart *m_html;
#endif
   ImageCanvas  *img_canvas;
   Previewer    *preview_canvas;
   QImage       *preview_img;
	
   ScanPackager *packager;
   ScanParams   *scan_params;

   KScanDevice  *sane;
   
   QVBoxLayout  *vlay_left;
   QHBoxLayout  *hlay_bigdad;
   QTabWidget   *tabw;

   int 		image_pool_id;
   int 		preview_id;

   KSANEOCR *ocrFabric;
   
};

#endif // KOOKAVIEW_H
