#ifndef SCAN_H
#define SCAN_H

#include <kscan.h>
#include <qimage.h>

class ScanParams;
class KScanDevice;

class ScanDialog : public KScanDialog
{
    Q_OBJECT

public:
    ScanDialog( QWidget *parent=0, const char *name=0, bool modal=false );
    ~ScanDialog();

private slots:
    void slCreateTempFile(QImage *img);
    
private:
    ScanParams  *m_scanParams;
    KScanDevice *m_device;
    
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
