/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2020       Jonathan Marten <jjm@keelhaul.me.uk>	*
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

#include "destinationapplication.h"

#include <qmimetype.h>
#include <qcombobox.h>
#include <qlabel.h>

#include <kapplicationtrader.h>
#include <kpluginfactory.h>
#include <kservice.h>
#include <klocalizedstring.h>
#include <kio/applicationlauncherjob.h>
#include <kio/jobuidelegate.h>

#include "scanparamspage.h"
#include "kookasettings.h"
#include "destination_logging.h"


K_PLUGIN_FACTORY_WITH_JSON(DestinationApplicationFactory, "kookadestination-application.json", registerPlugin<DestinationApplication>();)
#include "destinationapplication.moc"


DestinationApplication::DestinationApplication(QObject *pnt, const QVariantList &args)
    : AbstractDestination(pnt, "DestinationApplication")
{
}


void DestinationApplication::imageScanned(ScanImage::Ptr img)
{
    qCDebug(DESTINATION_LOG) << "received image size" << img->size() << "type" << img->imageType();
    const QString appService = mAppsCombo->currentData().toString();
    const QString mimeName = mFormatCombo->currentData().toString();
    qCDebug(DESTINATION_LOG) << "app" << appService << "mime" << mimeName;

    ImageFormat fmt = getSaveFormat(mimeName, img);	// get format for saving image
    if (!fmt.isValid()) return;				// must have this now
    const QUrl saveUrl = saveTempImage(fmt, img);	// save to temporary file
    if (!saveUrl.isValid()) return;			// could not save image

    // Open the temporary file with the selected application service.
    // If the service is "Other" (appService is empty), or if there is
    // a problem finding the service, then leave 'service' as null and
    // the ApplicationLauncherJob will prompt for an application.
    // The temporary file will eventually be removed by KIO.
    KService::Ptr service;
    if (!appService.isEmpty())
    {
        service = KService::serviceByDesktopName(appService);
        if (service==nullptr) qCWarning(DESTINATION_LOG) << "Cannot find service" << appService;
    }

    KIO::ApplicationLauncherJob *job = new KIO::ApplicationLauncherJob(service);
    job->setUrls(QList<QUrl>() << saveUrl);
    job->setRunFlags(KIO::ApplicationLauncherJob::DeleteTemporaryFiles);
    job->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, parentWidget()));
    job->start();					// all done
}


KLocalizedString DestinationApplication::scanDestinationString()
{
    const QString appService = mAppsCombo->currentData().toString();
    if (appService.isEmpty()) return (ki18n("Sending to application"));
							// selected "Other"
    return (kxi18n("Sending to <application>%1</application>").subs(mAppsCombo->currentText()));
}							// with application name


void DestinationApplication::createGUI(ScanParamsPage *page)
{
    // We do not yet know the eventual image format of the scanned image.
    // Therefore we would like, here in the GUI, to offer all of the known
    // applications that can handle any image type.  However, it does not seem
    // to be possible to express this in the trader query language;  according
    // to https://techbase.kde.org/Development/Tutorials/Services/Traders a
    // query such as
    //
    //   'image/' subin ServiceTypes
    //
    // should perform a substring match on all of the list entries.  However,
    // this sort of query appears to return nothing.
    //
    // Instead we query all of the application service types and then examine
    // their supported MIME types.  The criterion for including an application
    // is that it supports an image MIME type (starting with "image/") which is
    // also supported as a QImageWriter format (that is, a format that Kooka can
    // save to).

    const KService::List allServices = KApplicationTrader::query([](const KService::Ptr &){
        return true;
    });
    qCDebug(DESTINATION_LOG) << "have" << allServices.count() << "services";
    const QList<QMimeType> *imageMimeTypes = ImageFormat::mimeTypes();

    KService::List validServices;
    for (const KService::Ptr service : allServices)
    {
        // Okular is an odd case.  For whatever reason, the application does
        // not have just one desktop file listing all of the MIME types that
        // it supports, but a number of such files each listing one or a
        // closely related set of MIME types.  I suspect that this is so that
        // there can be one desktop file for each generator with the MIME
        // types that the generator supports.  There was a similar hack for
        // Ark (https://git.reviewboard.kde.org/r/129617 from 2011) that
        // collected a list of MIME types supported by each plugin and passed
        // them up to the top level CMakeLists.txt, then using the collected
        // list to generate the desktop files, but it was remarked at the time
        // that it would be more complicated to do this for Okular.
        //
        // Since the criteria used to select an application for including in
        // the "preferrred applications" list is that it supports at least one
        // image MIME type that we also support, this would result in multiple
        // entries for Okular.  Because the command used to start Okular is the
        // same in every case, we special case detect the main Okular application
        // here and ignore all of the others (along with any other NoDisplay
        // services).
        if (service->desktopEntryName()=="org.kde.okular")
        {
            qCDebug(DESTINATION_LOG) << "accept" << service->desktopEntryName() << "by name, pref" << service->initialPreference();
            validServices.append(service);
            continue;
        }

        if (service->noDisplay()) continue;		// ignore hidden services
        if (service->mimeTypes().isEmpty()) continue;	// ignore those with no MIME types
        //qCDebug(DESTINATION_LOG) << "  " << service->mimeTypes();

        for (const QString &mimeType : service->mimeTypes())
        {
            if (!mimeType.startsWith("image/")) continue;
            for (const QMimeType &imt : *imageMimeTypes)
            {						// supports a MIME type that we also do
                if (imt.inherits(mimeType)) goto found;
            }
        }
        continue;					// service not accepted

found:  qCDebug(DESTINATION_LOG) << "accept" << service->desktopEntryName() << "by MIME, pref" << service->initialPreference();
        validServices.append(service);
    }

    // Now all of the applications that accept file formats that can be
    // saved by Kooka are listed.  Fortunately the original trader query
    // returned them in priority order, so there is no need to sort them.
    qCDebug(DESTINATION_LOG) << "have" << validServices.count() << "valid services";

    mAppsCombo = new QComboBox;

    const QString configuredApp = KookaSettings::destinationApplicationApp();
    int configuredIndex = -1;
    for (const KService::Ptr service : validServices)
    {
        const QString key = service->desktopEntryName();
        if (key==configuredApp) configuredIndex = mAppsCombo->count();
        mAppsCombo->addItem(QIcon::fromTheme(service->icon()), service->name(), key);
    }
    if (configuredApp=="") configuredIndex = mAppsCombo->count();
    mAppsCombo->addItem(QIcon::fromTheme("system-run"), i18n("Other..."));
    if (configuredIndex!=-1) mAppsCombo->setCurrentIndex(configuredIndex);
    page->addRow(i18n("Application:"), mAppsCombo);

    // The MIME types that can be selected for sending the image.
    QStringList mimeTypes;
    mimeTypes << "image/png" << "image/jpeg" << "image/tiff" << "image/x-eps" << "image/bmp";
    mFormatCombo = createFormatCombo(mimeTypes, KookaSettings::destinationApplicationMime());
    page->addRow(i18n("Image format:"), mFormatCombo);
}


void DestinationApplication::saveSettings() const
{
    KookaSettings::setDestinationApplicationApp(mAppsCombo->currentData().toString());
    KookaSettings::setDestinationApplicationMime(mFormatCombo->currentData().toString());
}
