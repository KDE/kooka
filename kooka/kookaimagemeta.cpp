/***************************************************************************
                          kookaimage.cpp  - Kooka's Image
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

#include "kookaimagemeta.h"

KookaImageMeta::KookaImageMeta( ) :
    m_scanResolution(-1),
    m_scanResolutionY(-1)
{

}

void KookaImageMeta::setScanResolution( int x, int y)
{
    m_scanResolutionY = y;
    m_scanResolution = x;

}

int  KookaImageMeta::getScanResolutionX()
{
    return m_scanResolution;
}

int  KookaImageMeta::getScanResolutionY()
{
    return m_scanResolutionY;
}
