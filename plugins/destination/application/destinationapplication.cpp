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

#include <qdebug.h>
#include <qmimetype.h>
#include <qmimedatabase.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qtemporaryfile.h>
#include <qdir.h>

#include <kpluginfactory.h>
#include <kservice.h>
#include <kservicetypetrader.h>
#include <klocalizedstring.h>
#include <kmessagebox.h>
#include <kio/applicationlauncherjob.h>
#include <kio/jobuidelegate.h>

#include "scanparamspage.h"
#include "imageformat.h"
#include "formatdialog.h"
#include "kookasettings.h"


K_PLUGIN_FACTORY_WITH_JSON(DestinationApplicationFactory, "kookadestination-application.json", registerPlugin<DestinationApplication>();)
#include "destinationapplication.moc"


DestinationApplication::DestinationApplication(QObject *pnt, const QVariantList &args)
    : AbstractDestination(pnt, "DestinationApplication")
{
    qDebug();
}


bool DestinationApplication::scanStarting(ScanImage::ImageType type)
{
    // TODO: this may not be called if "Ask after scan" is set.
    // See KookaView::slotScanStart().
    mImageType = type;
    return (true);					// no preparation, can always start
}


void DestinationApplication::imageScanned(ScanImage::Ptr img)
{
    const QString appService = mAppsCombo->currentData().toString();
    const QString mimeName = mFormatCombo->currentData().toString();
    qDebug() << "app" << appService << "mime" << mimeName;

    // Get a format to save the scanned image.  If a valid one is selected,
    // the use that.  If "Other" is selected or the selected format is not
    // valid for saving (this shouldn't happen), then use the FormatDialog
    // to prompt for a format.
    ImageFormat fmt("");
    if (!mimeName.isEmpty())				// have selected a MIME type
    {
        QMimeDatabase db;				// get image format to use
        fmt = ImageFormat::formatForMime(db.mimeTypeForName(mimeName));
        if (!fmt.isValid()) qWarning() << "No MIME type or format for" << mimeName;
    }

    // If the format is "Other", or there was an error finding the MIME type,
    // then prompt for a format.
    if (!fmt.isValid())
    {
        FormatDialog fd(nullptr,			// parent
                        mImageType,			// type
                        true,				// askForFormat
                        fmt,				// default format
                        false,				// askForFilename
                        QString());			// filename
        if (!fd.exec()) return;				// dialogue cancelled
        // TODO: check meaning of "Always use this format"
        // not remembered (that done when called from ImgSaver)
        // retrieve by fd.alwaysUseFormat()
        // save format internally, pass to constructor above
        fmt = fd.getFormat();
    }

    qDebug() << "format" << fmt << "ext" << fmt.extension();
    if (!fmt.isValid()) return;				// must have this now

    // Save the image to a temporary file, in the format specified.
    QTemporaryFile temp(QDir::tempPath()+"/"+QCoreApplication::applicationName()+"XXXXXX."+fmt.extension());
    temp.setAutoRemove(false);
    temp.open();
    QUrl saveUrl = QUrl::fromLocalFile(temp.fileName());
    temp.close();					// now have name, but do not remove
    qDebug() << "save to" << saveUrl;			// temporary file location

    ImgSaver saver;					// save the image
    ImgSaver::ImageSaveStatus status = saver.saveImage(img.data(), saveUrl, fmt);
    if (status!=ImgSaver::SaveStatusOk)			// image save failed
    {
        KMessageBox::sorry(nullptr, xi18nc("@info", "Cannot save image file<nl/><filename>%1</filename><nl/>%2",
                                           saveUrl.toDisplayString(), saver.errorString(status)));
        temp.setAutoRemove(true);			// clean up temporary file
        return;
    }

    // Open the temporary file with the selected application service.
    // If the service is "Other" (appService is empty), or if there is
    // a problem finding the service, then leave 'service' as null and
    // the ApplicationLauncherJob will prompt for an application.
    // The temporary file will eventually be removed by KIO.
    KService::Ptr service;
    if (!appService.isEmpty())
    {
        service = KService::serviceByDesktopName(appService);
        if (service==nullptr) qWarning() << "Cannot find service" << appService;
    }

    KIO::ApplicationLauncherJob *job = new KIO::ApplicationLauncherJob(service);
    job->setUrls(QList<QUrl>() << saveUrl);
    job->setRunFlags(KIO::ApplicationLauncherJob::DeleteTemporaryFiles);
    job->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, nullptr));
    job->start();					// all done
}


QString DestinationApplication::scanDestinationString()
{
    const QString appService = mAppsCombo->currentData().toString();
    if (appService.isEmpty()) return (i18n("Sending to application"));
							// selected "Other"
    // TODO: KUIT here
    return (i18n("Sending to '%1'", mAppsCombo->currentText()));
}							// application name


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

    const KService::List allServices = KServiceTypeTrader::self()->query("Application");
    qDebug() << "have" << allServices.count() << "services";
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
            qDebug() << "accept" << service->desktopEntryName() << "by name, pref" << service->initialPreference();
            validServices.append(service);
            continue;
        }

        if (service->noDisplay()) continue;		// ignore hidden services
        if (service->mimeTypes().isEmpty()) continue;	// ignore those with no MIME types
        //qDebug() << "  " << service->mimeTypes();

        for (const QString &mimeType : service->mimeTypes())
        {
            if (!mimeType.startsWith("image/")) continue;
            for (const QMimeType &imt : *imageMimeTypes)
            {						// supports a MIME type that we also do
                if (imt.inherits(mimeType)) goto found;
            }
        }
        continue;					// service not accepted

found:  qDebug() << "accept" << service->desktopEntryName() << "by MIME, pref" << service->initialPreference();
        validServices.append(service);
    }

    // Now all of the applications that accept file formats that can be
    // saved by Kooka are listed.  Fortunately the original trader query
    // returned them in priority order, so there is no need to sort them.
    qDebug() << "have" << validServices.count() << "valid services";

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
    page->addRow(new QLabel(i18n("Application:"), page), mAppsCombo);

    // For the image format combo, again we do not yet know the format
    // of the scanned image.  Therefore the approach taken here, trying
    // to balance versatility against not confusing the user with a
    // long list of obscure image file formats, is to offer a small
    // predetermined list of popular formats in the combo, followed by
    // an "Other..." option.  The last option uses the Save Assistant to
    // select an image type (but without the option to enter the file name).
    //
    // It is the user's responsibility to make sure that the selected
    // application accepts the selected format, so the explicit formats
    // should be ones that are accepted by most common applications.
    mFormatCombo = new QComboBox;


    const QString configuredMime = KookaSettings::destinationApplicationMime();
    configuredIndex = -1;

    QMimeDatabase db;
    for (const QString &mimeName : {"image/png",
                                    "image/jpeg",
                                    "image/tiff",
                                    "image/x-eps",
                                    "image/bmp"})
    {
        const QMimeType mimeType = db.mimeTypeForName(mimeName);
        const ImageFormat fmt = ImageFormat::formatForMime(mimeType);
        if (!fmt.isValid()) continue;			// this format not supported

        if (mimeName==configuredMime) configuredIndex = mFormatCombo->count();
        mFormatCombo->addItem(QIcon::fromTheme(mimeType.iconName()), mimeType.comment(), mimeType.name());
    }
    if (configuredMime=="") configuredIndex = mFormatCombo->count();
    mFormatCombo->addItem(QIcon::fromTheme("system-run"), i18n("Other..."));
    if (configuredIndex!=-1) mFormatCombo->setCurrentIndex(configuredIndex);
    page->addRow(new QLabel(i18n("Image format:"), page), mFormatCombo);
}


void DestinationApplication::saveSettings() const
{
    KookaSettings::setDestinationApplicationApp(mAppsCombo->currentData().toString());
    KookaSettings::setDestinationApplicationMime(mFormatCombo->currentData().toString());
}
