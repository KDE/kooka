#ifndef SCAN_H
#define SCAN_H

#include <kscandevice.h>
#include <qimage.h>
#include <kscan.h>

class ScanParams;
class KScanDevice;
class Previewer;

class ScanDialog : public KScanDialog
{
    Q_OBJECT

public:
    ScanDialog( QWidget *parent=0, const char *name=0, bool modal=false );
    ~ScanDialog();

    virtual bool setup();
    
private slots:
    void slotFinalImage(QImage *);
   void slotNewPreview( QImage *);
private:
    ScanParams  *m_scanParams;
    KScanDevice *m_device;
    Previewer    *m_previewer;
    QImage m_previewImage;
   bool good_scan_connect;
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
