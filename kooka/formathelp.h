/***************************************************************************
                          formathelp.h  -  description
                             -------------------
    begin                : Mon Dec 27 1999
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

#include <klocale.h>

static const char *HELP_BMP = I18N_NOOP(
        "The <big>bitmap-format</big> is a well known format,\n" \
        "often used for 256 color images under "  \
        "MS Windows.\n Suitable for color and " \
        "<bold>lineart-pictures</bold>\n" );

static const char *HELP_PNM = I18N_NOOP(
        "Portable Anymap\n" \
        ""\
        "" );

static const char *HELP_JPG = I18N_NOOP(
        "JPEG is a high compression,\nquality " \
        "losing format for color\npictures " \
        "with many different colors." \
        "" );

static const char *HELP_EPS = I18N_NOOP(
        "EPS is Encapsulated Postscript,\n " \
        "initially a printer definition\n " \
        "language. Use this format if you\n" \
        "want to print the image or use\n" \
        "it with e.g. TeX" );
