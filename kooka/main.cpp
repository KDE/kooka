/***************************************************************************
                          main.cpp  -  description
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

#include <kapplication.h>
#include <K4AboutData>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kglobal.h>
#include <kdebug.h>

#include "kooka.h"
//QT5 #include "vcsversion.h"

static const char shortDesc[] = I18N_NOOP("Scanning, image gallery and OCR");

static const char longDesc[] = I18N_NOOP("Kooka provides access to scanner hardware using the "
                               "<a href=\"http://www.sane-project.org/\">SANE</a> library."
                               "\n\n"
                               "Kooka allows you to scan, save and view in any image format that KDE supports, "
                               "and can perform Optical Character Recognition using the open source "
                               "<a href=\"http://jocr.sourceforge.net/\">GOCR</a> or "
                               "<a href=\"http://www.gnu.org/software/ocrad/ocrad.html\">OCRAD</a> programs, "
                               "or the commercial <a href=\"http://www.rerecognition.com/\">KADMOS</a> library.");

// TODO: is the second paragraph of this licence needed with GPL Qt?
static const char addLicense[] =
    "This program is distributed under the terms of the GPL v2 as published\n"
    "by the Free Software Foundation.\n"
    "\n"
    "As a special exception, permission is given to link this program\n"
    "with any edition of Qt, and distribute the resulting executable\n"
    "without including the source code for Qt in the source distribution.\n"
    "\n"
    "As a special exception, permission is given to link this program\n"
    "with any version of the KADMOS OCR/ICR engine of reRecognition GmbH\n"
    "(http://www.rerecognition.com), and distribute the resulting executable\n"
    "without including the source code for KADMOS in the source distribution.\n"
    "\n"
    "Note that linking against KADMOS is not permitted under the terms of\n"
    "the Qt GPL licence, so if you wish to do this then you must have a\n"
    "commercial Qt development licence.\n";

static const char copyright[] =
    I18N_NOOP("(C) 2000-2010, the Kooka developers and contributors");

int main(int argc, char *argv[])
{
    K4AboutData about("kooka",              // appName
                      "",                // catalogName
                      ki18n("Kooka"),            // programName
#if VCS_AVAILABLE
                      (VERSION " (" VCS_TYPE " " VCS_REVISION ")"),
#else
//QT5 fix me
                      "VERSION",             // version
#endif
                      ki18n(shortDesc),          // shortDescription
                      K4AboutData::License_GPL_V2,   // licenseType
                      ki18n(copyright),          // copyrightStatement
                      ki18n(longDesc),           // text
                      "http://techbase.kde.org/Projects/Kooka");

    about.addAuthor(ki18n("Jonathan Marten"), ki18n("Current maintainer, KDE4 port"), "jjm@keelhaul.me.uk");
    about.addAuthor(ki18n("Klaas Freitag"), ki18n("Developer"), "freitag@suse.de");
    about.addCredit(ki18n("Mat Colton"), ki18n("Graphics, web"), "mat@colton.de");
    about.addCredit(ki18n("Ivan Shvedunov"), ki18n("Original kscan application"), "ivan@rf-hp.npi.msu.su");
    about.addCredit(ki18n("Alex Kempshall"), ki18n("Photocopy facility"), "alexkempshall@btinternet.com");
    about.addLicenseText(ki18n(addLicense));

    KCmdLineArgs::init(argc, argv, &about);

    KCmdLineOptions options;
    options.add("d <device>", ki18n("The SANE device specification (e.g. 'umax:/dev/sg0')"));
    options.add("g", ki18n("Gallery mode - do not connect to scanner"));
    KCmdLineArgs::addCmdLineOptions(options);       // Add my own options

    KApplication app;
    //KF5 port: remove this line and define TRANSLATION_DOMAIN in CMakeLists.txt instead
//KLocale::global()->insertCatalog("libkscan");

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    QString devToUse = args->getOption("d");
    if (args->isSet("g")) {
        devToUse = "gallery";
    }
    kDebug() << "DevToUse is" << devToUse;

// TODO: not sure what this did
//    if (args->count()==1)
//    {
//        args->usage();
//        // exit(-1);
//    }

    // TODO: try ScanGlobal::init(), if that fails no point in carrying on
    // so show an error box and give up (or can we carry on and run in
    // gallery mode only?)

    Kooka *kooka = new Kooka(devToUse.toLocal8Bit());
    kooka->show();
    app.processEvents();
    kooka->startup();
    args->clear();

    return (app.exec());
}
