/***************************************************************************
                  sizeindicator.h - size visualation widget
                             -------------------
    begin                : Thu 2001-08-16
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


#ifndef SIZEINDICATOR_H
#define SIZEINDICATOR_H

#define DEFAULT_CRITICAL (3*1024*1024)
#define DEFAULT_THRESHOLD (1*1024*1024)

#include <qlabel.h>
#include <qpainter.h>

/**
 *  @short  small size indication widget for file sizes
 *  @author Klaas Freitag
 *
 *  the size indicator is a small widget that displays a file size in a small
 *  frame. The unit (currently kB and MB) is selected automagically.
 *  If the file size grows bigger than the threshold set in the constructor,
 *  the widget starts to change its background color to visualise to the
 *  user that he is doing something obvious.
 */

class SizeIndicator: public QLabel
{
   Q_OBJECT
   // Q_PROPERTY( KGammaTable *gt READ getGt WRITE setGt )
      
public:
   /**
    *  Creates a size indicator widget.
    *  @param thres: Threshold, value on that the widget starts to become red.
    *  @param crit: Critical value, not yet used.

    */
   SizeIndicator( QWidget *parent, long thres = DEFAULT_THRESHOLD,
		  long crit = DEFAULT_CRITICAL );
   /**
    *  destructor does not really do much yet.
    */
   ~SizeIndicator();

public slots:

   /**
    * is the slot that sets the file size to display. The widget gets
    * updated.
    * @param sizeInByte: the size to set.
    */
   void setSizeInByte( long );

   /**
    * sets the critical size.
    * @param crit: the critical value
    */
   void setCritical( long );

   /**
    * sets the threshold value.
    * @param thres: the threshold bytesize
    */
   void setThreshold( long );
   
    
protected:
   /**
    *  reimplemented to display the color
    */
   virtual void drawContents( QPainter* );

private:

   long sizeInByte;
   long critical, threshold;

   double devider;

   class sizeIndicatorPrivate;
   sizeIndicatorPrivate *d;
};

#endif
