/***************************************************************************
                          kookaview.h  -  Main view
                             -------------------
    begin                : Sun Jan 16 2000
    copyright            : (C) 2000 by Klaas Freitag
    email                : freitag@suse.de
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
#ifndef KOOKAVIEW_H
#define KOOKAVIEW_H

#include <qwidget.h>
#include <kopenwith.h>
#include "kookaiface.h"
#include <kdockwidget.h>
#include <qtabwidget.h>
#include <qlayout.h>
#include <qimage.h>
#include <qsplitter.h>

#include <kparts/dockmainwindow.h>
#include <kparts/part.h>

// application specific includes
#include "kscandevice.h"
#include "previewer.h"
#include "scanpackager.h"
#include "scanparams.h"
#include "img_canvas.h"

class KDockWidget;
class QPainter;
class KSANEOCR;
class KConfig;
class KPrinter;
class KComboBox;
class KAction;
class KActionCollection;
class ThumbView;
class KookaImage;
class QPixmap;
class ocrResEdit;
/**
 * This is the main view class for Kooka.  Most of the non-menu,
 * non-toolbar, and non-statusbar (e.g., non frame) GUI code should go
 * here.
 *
 * @short Main view
 * @author Klaas Freitag <freitag@suse.de>
 * @version 0.1
 */
class KookaView : public QObject
{
    Q_OBJECT
public:
    typedef enum { MirrorVertical, MirrorHorizontal, MirrorBoth } MirrorType;
    typedef enum { StatusTemp, StatusImage } StatusBarIDs;

    /**
     * Default constructor
     */
    KookaView(KParts::DockMainWindow *parent, const QCString& deviceToUse);

    /**
     * Destructor
     */
    virtual ~KookaView();

    /**
     * Print this view to any medium -- paper or not
     */
    void print( );

    bool ToggleVisibility( int );
    void loadStartupImage( void );
    KDockWidget *mainDockWidget( ) { return m_mainDock; }

    void createDockMenu( KActionCollection*, KDockMainWindow *, const char *);

    ScanPackager *gallery() { return packager; }

    // KParts::Part* ocrResultPart() { return m_textEdit; }

    ImageCanvas *getImageViewer() { return img_canvas; }
public slots:
    void slShowPreview()  {  }
    void slShowPackager() {  }
    void slNewPreview( QImage *, ImgScanInfo * );

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
    void slIVShowZoomDialog( )
        { if( img_canvas ) img_canvas->handle_popup(ImageCanvas::ID_POP_ZOOM ); }

    void slOpenCurrInGraphApp( void );

    void slSaveOCRResult();

    void slLoadScanParams( );
    void slSaveScanParams( );

    void slOCRResultImage( const QPixmap& );

    void slShowThumbnails( KFileTreeViewItem *dirKfi = 0, bool forceRedraw=false);
     void slFreshUpThumbView();

    /**
     * slot that show the image viewer
     */
    void slStartLoading( const KURL& url );
    /**
     * starts ocr on the image the parameter is pointing to
     **/
    void startOCR( KookaImage* );

    void  slCloseScanDevice();
    void saveProperties( KConfig* );

    /**
     * slot to select the scanner device. Does all the work with selection
     * of scanner, disconnection of the old device and connecting the new.
     */
    bool slSelectDevice(const QCString& useDevice=QCString());

    void connectViewerAction( KAction *action );
    void connectGalleryAction( KAction *action );

    void slScanStart();
    void slScanFinished( KScanStat stat );
    void slAcquireStart();


protected slots:

    void  slShowAImage( KookaImage* );
    void  slUnloadAImage( KookaImage* );

    /**
     * called from the scandevice if a new Image was successfully scanned.
     * Needs to convert the one-page-QImage to a KookaImage
     */
    void slNewImageScanned(QImage*, ImgScanInfo*);

    /**
     * called if an viewer image was set to read only or back to read write state.
     */
    void slViewerReadOnly( bool ro );
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
    QImage rotate180  ( QImage* );
    QCString userDeviceSelection( ) const;

    void updateCurrImage( QImage& ) ;

    ImageCanvas  *img_canvas;
    ThumbView    *m_thumbview;

    Previewer    *preview_canvas;
    ScanPackager *packager;
    ScanParams   *scan_params;

    KScanDevice  *sane;
    KComboBox    *recentFolder;

    QCString     connectedDevice;

    QImage       *m_ocrResultImg;
    int          image_pool_id;
    int 	 preview_id;

    KSANEOCR *ocrFabric;

    KDockWidget *m_mainDock;
    KDockWidget *m_dockScanParam;
    KDockWidget *m_dockThumbs;
    KDockWidget *m_dockPackager;
    KDockWidget *m_dockRecent;
    KDockWidget *m_dockPreview;
    KDockWidget *m_dockOCRText;

    KMainWindow *m_mainWindow;

    ocrResEdit  *m_ocrResEdit;
};

#endif // KOOKAVIEW_H
