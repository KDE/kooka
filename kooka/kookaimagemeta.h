/***************************************************************************
                kookaimagemeta.h  - Kooka's Image Meta Data
                             -------------------
    begin                : Thu Nov 20 2001
    copyright            : (C) 1999 by Klaas Freitag
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


#ifndef KOOKAIMAGEMETA_H
#define KOOKAIMAGEMETA_H


/**
  * @author Klaas Freitag
  *
  */


class KookaImageMeta
{
public:

    KookaImageMeta( );
    ~KookaImageMeta() { ;}

    void setScanResolution( int x, int y=-1);
    int  getScanResolutionX();
    int  getScanResolutionY();

private:
    int 	m_scanResolution;
    int         m_scanResolutionY;

};

#endif
