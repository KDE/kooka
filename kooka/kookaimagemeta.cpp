/***************************************************************************
                          kookaimage.cpp  - Kooka's Image
                             -------------------
    begin                : Thu Nov 20 2001
    copyright            : (C) 1999 by Klaas Freitag
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

int  KookaImageMeta::getScanResolutionX() const
{
    return m_scanResolution;
}

int  KookaImageMeta::getScanResolutionY() const
{
    return m_scanResolutionY;
}
