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

public:
   KGammaTable ( int gamma = 100, int brightness = 0,
		 int contrast = 0 );
   void setAll ( int gamma, int brightness, int contrast );
   QArray<SANE_Word> *getArrayPtr( void ) { return &gt; }

   int  getGamma( void )      { return g; }
   int  getBrightness( void ) { return b; }
   int  getContrast( void )   { return c; }

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
};

#endif // KGAMMATABLE_H
