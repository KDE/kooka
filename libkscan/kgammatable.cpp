#include <math.h>

#include <kdebug.h>
#include "kgammatable.h"

KGammaTable::KGammaTable( int gamma, int brightness, int contrast  ) 
    : QObject()
{
   g = gamma < 1 ? 1 : gamma;
   b = brightness;
   c = contrast;
   gt.resize(256);
   calcTable();
}

const KGammaTable& KGammaTable::operator=(const KGammaTable& gt)
{
   if( this != &gt )
   {
      g = gt.g;
      b = gt.b;
      c = gt.c;

      calcTable();
   }

   return( *this );
}

inline SANE_Word adjust( SANE_Word x, int gl,int bl,int cl)
{
   //printf("x : %d, x/256 : %lg, g : %d, 100/g : %lg",
   //       x,(double)x/256.0,g,100.0/(double)g);
   x = ( SANE_Int )(256*pow((double)x/256.0,100.0/(double)gl));
   x = ((65536/(128-cl)-256)*(x-128)>>8)+128+bl;
   if(x<0) x = 0; else if(x>255) x = 255;
   //printf(" -> %d\n",x);
   return x;
}

void KGammaTable::calcTable( )
{
   int br = (b<<8)/(128-c);
   int gr = g;
   int cr = c;

   if( gr == 0 )
   {
      kdDebug(29000) << "Cant calc table -> would raise div. by zero !" << endl;
      return;
   }
   
   for( SANE_Word i = 0; i<256; i++)
      gt[i] = adjust(i, gr, br, cr );

   dirty = false;
}

void KGammaTable::setAll( int gamma, int brightness, int contrast )
{
    g = gamma < 1 ? 1 : gamma;
    b = brightness;
    c = contrast;

    dirty = true;
}


SANE_Word* KGammaTable::getTable()
{
    if( dirty ) calcTable();
    return( gt.data());
}
#include "kgammatable.moc"
