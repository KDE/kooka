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
#include <kopenwith.h>
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
#include "img_canvas.h"

class QPainter;
class KSANEOCR;
class KConfig;
class KPrinter;

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
   typedef enum { MirrorVertical, MirrorHorizontal, MirrorBoth } MirrorType;

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
   void print(QPainter *, KPrinter*, QPaintDeviceMetrics& );

   bool ToggleVisibility( int );
   void loadStartupImage( void );
   
public slots:
   void slShowPreview()  { tabw->showPage( preview_canvas ); }
   void slShowPackager() { tabw->showPage( packager ); }
   void slNewPreview( QImage * );

   void slSetScanParamsVisible( bool v )
      { if( v ) scan_params->show(); else scan_params->hide(); }
   void slSetTabWVisible( bool v )
      { if( v ) preview_canvas->show(); else preview_canvas->hide(); }
   
   void doOCR( void );	
   void doOCRonSelection( void );
   
   void slStartPreview() { if( scan_params ) scan_params->slAcquirePreview(); }
   void slStartFinalScan() { if( scan_params ) scan_params->slStartScan(); }

   void slCreateNewImgFromSelection( void );

   void slRotateImage( int );

   void slMirrorImage( MirrorType );

   void slIVScaleToWidth( void )
      { if( img_canvas ) img_canvas->handle_popup(ImageCanvas::ID_FIT_WIDTH );}
   void slIVScaleToHeight( void )
      { if( img_canvas ) img_canvas->handle_popup(ImageCanvas::ID_FIT_HEIGHT );}
   void slIVScaleOriginal( void )
      { if( img_canvas ) img_canvas->handle_popup(ImageCanvas::ID_ORIG_SIZE ); }

   void slOpenCurrInGraphApp( void );

   void slSaveScanParams( void );
    /**
    * starts ocr on the image the parameter is pointing to
    **/
   void startOCR( const QImage* );

   void saveProperties( KConfig* );
   
signals:
   /**
    * Use this signal to change the content of the statusbar
    */
   void signalChangeStatusbar(const QString& text);

   /**
    * Use this signal to clean up the statusbar
    */
   void signalCleanStatusbar( void );
   
   /**
    * Use this signal to change the content of the caption
    */
   void signalChangeCaption(const QString& text);


private:
   QImage rotateRight( QImage* );
   QImage rotateLeft ( QImage* );
   
#if 0 
   KParts::ReadOnlyPart *m_html;
#endif
   void updateCurrImage( QImage& ) ;


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
