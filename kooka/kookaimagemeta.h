/***************************************************************************
                kookaimagemeta.h  - Kooka's Image Meta Data
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
    int  getScanResolutionX() const;
    int  getScanResolutionY() const;

private:
    int 	m_scanResolution;
    int         m_scanResolutionY;

};

#endif
