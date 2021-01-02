/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 1999-2016 Klaas Freitag <freitag@suse.de>		*
 *                          Jonathan Marten <jjm@keelhaul.me.uk>	*
 *									*
 *  Kooka is free software; you can redistribute it and/or modify it	*
 *  under the terms of the GNU Library General Public License as	*
 *  published by the Free Software Foundation and appearing in the	*
 *  file COPYING included in the packaging of this file;  either	*
 *  version 2 of the License, or (at your option) any later version.	*
 *									*
 *  As a special exception, permission is given to link this program	*
 *  with any version of the KADMOS OCR/ICR engine (a product of		*
 *  reRecognition GmbH, Kreuzlingen), and distribute the resulting	*
 *  executable without including the source code for KADMOS in the	*
 *  source distribution.						*
 *									*
 *  This program is distributed in the hope that it will be useful,	*
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of	*
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*
 *  GNU General Public License for more details.			*
 *									*
 *  You should have received a copy of the GNU General Public		*
 *  License along with this program;  see the file COPYING.  If		*
 *  not, see <http://www.gnu.org/licenses/>.				*
 *									*
 ************************************************************************/

#include <qapplication.h>
#include <qcommandlineparser.h>

#include <kaboutdata.h>
#include <klocalizedstring.h>

#include "kooka.h"
#include "vcsversion.h"
#include "kooka_logging.h"


static const char shortDesc[] =
    I18N_NOOP("Scanning, image gallery and OCR");

static const char longDesc[] =
    I18N_NOOP("Kooka provides access to scanner hardware using the "
              "<a href=\"http://www.sane-project.org/\">SANE</a> library."
              "\n\n"
              "Kooka allows you to scan, save and view in any image format that KDE supports, "
              "and can perform Optical Character Recognition using the open source "
              "<a href=\"http://jocr.sourceforge.net/\">GOCR</a> or "
              "<a href=\"http://www.gnu.org/software/ocrad/ocrad.html\">OCRAD</a> programs, "
              "or the commercial <a href=\"http://www.rerecognition.com/\">KADMOS</a> library.");

static const char addLicense[] =
    I18N_NOOP("This program is distributed under the terms of the GPL v2 as published\n"
              "by the Free Software Foundation.\n"
              "\n"
              "As a special exception, permission is given to link this program\n"
              "with any version of the KADMOS OCR/ICR engine of reRecognition GmbH\n"
              "(http://www.rerecognition.com), and distribute the resulting executable\n"
              "without including the source code for KADMOS in the source distribution.\n"
              "\n"
              "Note that linking against KADMOS is not permitted under the terms of\n"
              "the Qt GPL licence, so if you wish to do this then you must have a\n"
              "commercial Qt development licence.\n");

static const char copyright[] =
    I18N_NOOP("(C) 2000-2018, the Kooka developers and contributors");


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);			// first of all, so that i18n() works
    KLocalizedString::setApplicationDomain("kooka");
    KAboutData about("kooka",					// componentName
                     i18n("Kooka"),				// displayName
#if VCS_AVAILABLE
                     (VERSION " (" VCS_TYPE " " VCS_REVISION ")"),
#else
                     "VERSION",				// version
#endif
                     i18n(shortDesc),				// shortDescription
                     KAboutLicense::GPL_V2,			// licenseType
                     i18n(copyright),				// copyrightStatement
                     i18n(longDesc),				// otherText
                     "http://techbase.kde.org/Projects/Kooka");	// homePageAddress

    about.addAuthor(i18n("Jonathan Marten"), i18n("Current maintainer, KDE4 port"), "jjm@keelhaul.me.uk");
    about.addAuthor(i18n("Montel Laurent"), i18n("Initial KF5 port"), "montel@kde.org");
    about.addAuthor(i18n("Klaas Freitag"), i18n("Developer"), "freitag@suse.de");
    about.addCredit(i18n("Mat Colton"), i18n("Graphics, web"), "mat@colton.de");
    about.addCredit(i18n("Ivan Shvedunov"), i18n("Original kscan application"), "ivan@rf-hp.npi.msu.su");
    about.addCredit(i18n("Alex Kempshall"), i18n("Photocopy facility"), "alexkempshall@btinternet.com");
    about.addLicenseText(i18n(addLicense));

    KAboutData::setApplicationData(about);

    QCommandLineParser parser;
    parser.setApplicationDescription(about.shortDescription());
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption opt = QCommandLineOption("g", i18n("Gallery mode - do not connect to scanner"));
    parser.addOption(opt);

    opt = QCommandLineOption("d", i18n("The SANE device specification (e.g. 'umax:/dev/sg0')"), i18n("device"));
    parser.addOption(opt);

    parser.process(app);

    QString devToUse = parser.value("d");
    if (parser.isSet("g")) devToUse = "gallery";
    qCDebug(KOOKA_LOG) << "device to use" << devToUse;

    // TODO: try ScanGlobal::init(), if that fails no point in carrying on
    // so show an error box and give up (or can we carry on and run in
    // gallery mode only?)

    Kooka *kooka = new Kooka(devToUse.toLocal8Bit());
    kooka->show();
    app.processEvents();
    kooka->startup();

    return (app.exec());
}
