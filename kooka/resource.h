/***************************************************************************
                          resource.h  -  description                              
                             -------------------                                         
    begin                : Thu Dec  9 20:16:54 MET 1999
                                           
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


