#ifndef KGAMMATABLE_H
#define KGAMMATABLE_H

#include <qarray.h>
#include <qobject.h>

extern "C" {
#include <sane.h>
}

class KGammaTable: public QObject
{
   Q_OBJECT

   Q_PROPERTY( int g READ getGamma WRITE setGamma )
   Q_PROPERTY( int c READ getContrast WRITE setContrast )
   Q_PROPERTY( int b READ getBrightness WRITE setBrightness )
      
public:
   KGammaTable ( int gamma = 100, int brightness = 0,
		 int contrast = 0 );
   void setAll ( int gamma, int brightness, int contrast );
   QArray<SANE_Word> *getArrayPtr( void ) { return &gt; }

   int  getGamma( ) const      { return g; }
   int  getBrightness( ) const { return b; }
   int  getContrast( ) const   { return c; }

   const KGammaTable& operator=(const KGammaTable&);

public slots:
   void       setContrast   ( int con )   { c = con; dirty = true; emit( tableChanged() );}
   void       setBrightness ( int bri )   { b = bri; dirty = true; emit( tableChanged() );}
   void       setGamma      ( int gam )   { g = gam; dirty = true; emit( tableChanged() );}

   int        tableSize()      { return gt.size(); }
   SANE_Word  *getTable();

signals:
   void tableChanged(void);

private:
   void       calcTable( );
   int        g, b, c;
   bool       dirty;
   QArray<SANE_Word> gt;

   class KGammaTablePrivate;
   KGammaTablePrivate *d;
};

#endif // KGAMMATABLE_H
