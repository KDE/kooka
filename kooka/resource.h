/***************************************************************************
                          resource.h  -  description
                             -------------------
    begin                : Thu Dec  9 20:16:54 MET 1999

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

#ifndef RESSOURCE_H
#define RESSOURCE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


///////////////////////////////////////////////////////////////////
// resource.h  -- contains macros used for commands


///////////////////////////////////////////////////////////////////
// COMMAND VALUES FOR MENUBAR AND TOOLBAR ENTRIES


///////////////////////////////////////////////////////////////////
// File-menu entries
#define ID_FILE_NEW                 10020
#define ID_FILE_OPEN                10030

#define ID_FILE_SAVE                10050
#define ID_FILE_SAVE_AS             10060
#define ID_FILE_CLOSE               10070

#define ID_FILE_PRINT               10080

#define ID_FILE_QUIT                10100


///////////////////////////////////////////////////////////////////
// Edit-menu entries
#define ID_EDIT_UNDO                11010
#define ID_EDIT_REDO                11020
#define ID_EDIT_COPY                11030
#define ID_EDIT_CUT                 11040
#define ID_EDIT_PASTE               11050
#define ID_EDIT_SELECT_ALL          11060


///////////////////////////////////////////////////////////////////
// View-menu entries
#define ID_VIEW_TOOLBAR             12010
#define ID_VIEW_STATUSBAR           12020
#define ID_VIEW_PREVIEW		    12021
#define ID_VIEW_POOL		    12022
#define ID_VIEW_SCANPARAMS          12023


///////////////////////////////////////////////////////////////////
// View-menu entries
#define ID_SCAN_PREVIEW             13010
#define ID_SCAN_FINAL               13020

///////////////////////////////////////////////////////////////////
// Help-menu entries
#define ID_HELP_ABOUT                     1002

///////////////////////////////////////////////////////////////////
// General application values
#define IDS_APP_ABOUT "Ksanetest\n Version " VERSION

#define IDS_DEFAULT                 "Ready."

#endif // RESOURCE_H


