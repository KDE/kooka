/***************************************************************************
                          ksaneocr.h  -  description                              
                             -------------------                                         
    begin                : Fri Jun 30 2000                                           
    copyright            : (C) 2000 by Klaas Freitag                         
    email                :                                      
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   * 
 *                                                                         *
 ***************************************************************************/


#ifndef KSANEOCR_H
#define KSANEOCR_H

#include <qobject.h>
#include <qimage.h>

/**
  *@author Klaas Freitag
  */

class KSANEOCR : public QObject  {
public: 
	KSANEOCR(const QImage *image);
	~KSANEOCR();
};

#endif
