/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2021      Jonathan Marten <jjm@keelhaul.me.uk>	*
 *  Based on the KIPI interface for Spectacle, which is:		*
 *  Copyright (C) 2015 Boudhayan Gupta <me@BaloneyGeek.com>		*
 *  Copyright (C) 2010 Pau Garcia i Quiles <pgquiles@elpauer.org>	*
 *  Essentially a rip-off of code for Kamoso by:			*
 *  Copyright (C) 2008-2009 by Aleix Pol <aleixpol@kde.org>		*
 *  Copyright (C) 2008-2009 by Alex Fiestas <alex@eyeos.org>		*
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

#include <iostream>

#include <qapplication.h>
#include <qcommandlineparser.h>
#include <qaction.h>
#include <qdir.h>
#include <qfileinfo.h>

#include <kaboutdata.h>
#include <klocalizedstring.h>
#include <klocalizedstring.h>

#include <kipi/plugin.h>
#include <kipi/pluginloader.h>

#include "exportkipiinterface.h"
#include "vcsversion.h"
#include "destination_logging.h"


static const char shortDesc[] =
    I18N_NOOP("Helper command for the 'Export to Online Service' plugin");

static const char copyright[] =
    I18N_NOOP("(C) 2021, the Kooka developers and contributors");


static void report(bool isError, const QString &message)
{
    std::cerr << qPrintable(QCoreApplication::applicationName());
    std::cerr << " (" << (isError ? "error" : "warning") << "): ";
    std::cerr << qPrintable(message) << std::endl;
    if (isError) std::exit(EXIT_FAILURE);
}


int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("kooka");

    KAboutData about("kookakipihelper",					// componentName
                     i18n("Kooka KIPI Helper"),				// displayName
#if VCS_AVAILABLE
                     (VERSION " (" VCS_TYPE " " VCS_REVISION ")"),
#else
                     "VERSION",                         		// version
#endif
                     i18n(shortDesc),					// shortDescription
                     KAboutLicense::GPL_V2,				// licenseType
                     i18n(copyright),					// copyrightStatement
                     QString(),						// otherText
                     "http://techbase.kde.org/Projects/Kooka");		// homePageAddress

    about.addAuthor(i18n("Jonathan Marten"), i18n("Current maintainer, KDE4/KF5 port"), "jjm@keelhaul.me.uk");
    KAboutData::setApplicationData(about);

    QCommandLineParser parser;
    parser.setApplicationDescription(about.shortDescription());
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption opt = QCommandLineOption("l", i18n("List all of the available KIPI plugin actions"));
    parser.addOption(opt);

    opt = QCommandLineOption("s", i18n("The KIPI plugin action to export to"), i18n("action"));
    parser.addOption(opt);

    parser.addPositionalArgument("urls", i18n("URLs of the image files to export"), "[urls...]");
    parser.process(app);

    const bool listmode = parser.isSet("l");
    const QString sharetype = parser.value("s");
    const QStringList args = parser.positionalArguments();

    if (listmode)
    {
        if (!sharetype.isEmpty()) report(true, i18n("Cannot specify both list ('-l') and share ('-s') mode"));
        if (!args.isEmpty()) report(false, i18n("Arguments ignored in list ('-l') mode"));
    }
    else if (!sharetype.isEmpty())
    {
        if (args.isEmpty()) report(true, i18n("Arguments missing for share ('-s') mode"));
    }
    else report(true, i18n("One of either list ('-l') or share ('-s') mode must be specified"));

    QWidget *dummyWidget = new QWidget;
    ExportKipiInterface *kipiInterface = new ExportKipiInterface;
    KIPI::PluginLoader *loader = new KIPI::PluginLoader;
    loader->setInterface(kipiInterface);
    loader->init();

    // Based on ExportMenu::loadKipiItems() in spectacle/src/Gui/ExportMenu.cpp
    QList<QAction *> exportActions;
    const KIPI::PluginLoader::PluginList pluginList = loader->pluginList();
    qCDebug(DESTINATION_LOG) << "have" << pluginList.count() << "plugins";
    for (const auto &pluginInfo : pluginList)
    {
        if (!pluginInfo->shouldLoad()) continue;

        KIPI::Plugin *plugin = pluginInfo->plugin();
        if (plugin==nullptr)
        {
            qCWarning(DESTINATION_LOG) << "Failed to load KIPI plugin from library" << pluginInfo->library();
            continue;
        }

        plugin->setup(dummyWidget);
        const QList<QAction *> actions = plugin->actions();
        for (QAction *action : actions)
        {
            KIPI::Category category = plugin->category(action);
            if (category==KIPI::ExportPlugin)
            {
                exportActions += action;
            }
            else if (category==KIPI::ImagesPlugin && pluginInfo->library().contains(QLatin1String("kipiplugin_sendimages")))
            {
                exportActions += action;
            }
        }
    }
    qCDebug(DESTINATION_LOG) << "have" << exportActions.count() << "export actions";

    if (listmode)					// list mode
    {
        int maxnamelen = -1;
        int maxiconlen = -1;
        for (QAction *action : qAsConst(exportActions))
        {
            maxnamelen = qMax(maxnamelen, action->objectName().length());
            maxiconlen = qMax(maxiconlen, action->icon().name().length());
        }

        const QString dots = i18nc("as added to action text", "...");
        for (QAction *action : qAsConst(exportActions))
        {
            QString text = KLocalizedString::removeAcceleratorMarker(action->text());
            if (text.endsWith(dots)) text.chop(dots.length());

            std::cout << qPrintable(action->objectName().leftJustified(maxnamelen+3));
            std::cout << qPrintable(action->icon().name().leftJustified(maxiconlen+3));
            std::cout << qPrintable(text);
            std::cout << std::endl;
        }

        return (EXIT_SUCCESS);
    }
    else						// share mode
    {
        QList<QUrl> fileUrls;
        for (const QString &arg : args)
        {
            QUrl u = QUrl::fromUserInput(arg, QDir::currentPath(), QUrl::AssumeLocalFile);
            if (!u.isLocalFile())
            {
                report(false, i18n("Argument '%1' is not a local file, ignored", u.toDisplayString()));
                continue;
            }

            QFileInfo fi(u.toLocalFile());
            if (!fi.exists() || !fi.isReadable())
            {
                report(false, i18n("File '%1' not found or is unreadable, ignored", fi.canonicalFilePath()));
                continue;
            }

            fileUrls.append(u);				// this file is usable
        }

        if (fileUrls.isEmpty()) report(true, i18n("No usable files to share"));
        qCDebug(DESTINATION_LOG) << "have" << fileUrls.count() << "files to export";
        for (const QUrl &url : qAsConst(fileUrls))
        {
            kipiInterface->addUrl(url);
        }

        for (QAction *action : qAsConst(exportActions))
        {
            if (sharetype!=action->objectName()) continue;
            qCDebug(DESTINATION_LOG) << "sharing with action" << action->text();

            action->trigger();				// do the export action
            return (app.exec());			// let that do its work
        }

        report(true, i18n("Unknown plugin action '%1'", sharetype));
    }

    std::exit(EXIT_FAILURE);				// no action taken
}
