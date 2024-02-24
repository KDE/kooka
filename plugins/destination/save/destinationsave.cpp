/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2021      Jonathan Marten <jjm@keelhaul.me.uk>	*
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

#include "destinationsave.h"

#include <qfiledialog.h>
#include <qmimetype.h>
#include <qmimedatabase.h>

#include <kpluginfactory.h>
#include <klocalizedstring.h>
#include <kmessagebox.h>
#include <kprotocolinfo.h>

#include <kio/statjob.h>
#include <kio/copyjob.h>
#include <kio/jobuidelegatefactory.h>

#include "scanparamspage.h"
#include "kookasettings.h"
#include "imageformat.h"
#include "formatdialog.h"
#include "imgsaver.h"
#include "destination_logging.h"


K_PLUGIN_FACTORY_WITH_JSON(DestinationSaveFactory, "kookadestination-save.json", registerPlugin<DestinationSave>();)
#include "destinationsave.moc"


DestinationSave::DestinationSave(QObject *pnt, const QVariantList &args)
    : AbstractDestination(pnt, "DestinationSave")
{
}


bool DestinationSave::scanStarting(ScanImage::ImageType type)
{
    mSaveUrl.clear();					// reset for a new scan
    mSaveMime.clear();

    // Now the standard setting is to always as ask for the file name
    // before starting a scan.
    return (getSaveLocation(type));
}


void DestinationSave::imageScanned(ScanImage::Ptr img)
{
    qCDebug(DESTINATION_LOG) << "received image size" << img->size();

    ImageFormat fmt = getSaveFormat(mSaveMime, img);
    qCDebug(DESTINATION_LOG) << "to" << mSaveUrl << "mime" << mSaveMime << "format" << fmt;
    // These should now never happen, because getSaveLocation() was
    // called unconditionally above.
    if (!fmt.isValid()) return;
    if (!mSaveUrl.isValid()) return;

    // TODO: move from here on into ImgSaver, so that it can save to
    // remote locations.
    QUrl url = mSaveUrl;
    if (!url.isLocalFile())
    {
        if (KProtocolInfo::protocolClass(url.scheme())==QLatin1String(":local"))
        {
            qCDebug(DESTINATION_LOG) << "maybe local, running a StatJob";

            // Even though the purpose of the StatJob is just to get information,
            // an error is displayed if the destination file does not exist (as
            // it is assumed it does not at this stage).  To avoid this situation
            // but to still get errors and/or warnings for other problems (e.g.
            // an unreachable host, authentication failiure etc), give the StatJob
            // the URL of the containing directory and add the file name back
            // afterwards.
            KIO::StatJob *job = KIO::mostLocalUrl(url.adjusted(QUrl::RemoveFilename|QUrl::StripTrailingSlash));
            job->setSide(KIO::StatJob::DestinationSide);
            job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, parentWidget()));
            if (job->exec())
            {
                QString fileName = url.fileName();
                url = job->mostLocalUrl();
                url.setPath(url.path()+'/'+fileName);
                qDebug() << "StatJob result" << url;
            }
        }
    }

    if (url.isValid() && url.isLocalFile())		// did we find a local path?
    {
        // The save location is local, so we can use ImgSaver directly.
        qCDebug(DESTINATION_LOG) << "save to local" << url.toLocalFile();

        ImgSaver saver(url.adjusted(QUrl::RemoveFilename));
        ImgSaver::ImageSaveStatus status = saver.saveImage(img, url, fmt);
        if (status!=ImgSaver::SaveStatusOk)
        {
            KMessageBox::error(parentWidget(),
                               xi18nc("@info", "Cannot save image to <filename>%1</filename><nl/><message>%2</message>",
                                      url.toDisplayString(), saver.errorString(status)),
                               i18n("Cannot Save Image"));
        }
    }
    else						// the location is not local
    {
        qCDebug(DESTINATION_LOG) << "save to remote";

        // The save location is remote.  Save a temporary image,
        // then use KIO to copy it to the destination location.
        QUrl url = saveTempImage(fmt, img);		// save a temporary image
        if (!url.isValid()) return;			// temp image save failed

        KIO::CopyJob *job = KIO::copy(url, mSaveUrl);
        job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, parentWidget()));
        job->exec();					// do the remote copy

        QFile::remove(url.toLocalFile());		// remove the temporary file
    }

    // Remember the values used, so that starting a new scan and then
    // cancelling it will not clear them.  See scanStarting() and
    // saveSettings().
    mLastSaveUrl = mSaveUrl.adjusted(QUrl::RemoveFilename);
    mLastSaveMime = mSaveMime;
}


KLocalizedString DestinationSave::scanDestinationString()
{
    if (!mSaveUrl.isValid()) return (ki18n("Saving image"));
    return (kxi18n("Saving to <filename>%1</filename>").subs(mSaveUrl.toDisplayString()));
}


void DestinationSave::saveSettings() const
{
    if (mLastSaveUrl.isValid()) KookaSettings::setDestinationSaveDest(mLastSaveUrl);
    if (!mLastSaveMime.isEmpty()) KookaSettings::setDestinationSaveMime(mLastSaveMime);
}


bool DestinationSave::getSaveLocation(ScanImage::ImageType type)
{
    qCDebug(DESTINATION_LOG) << "for type" << type;

    // Using the non-static QFileDialog so as to be able to
    // set MIME type filters.
    QFileDialog dlg(parentWidget(), i18n("Save Scan"));
    dlg.setAcceptMode(QFileDialog::AcceptSave);

    // Check whether to offer only recommended save formats for
    // the image type.
    const bool recOnly = KookaSettings::saverOnlyRecommendedTypes();

    QStringList filters;
    const QList<QMimeType> *mimeTypes = ImageFormat::mimeTypes();
    for (const QMimeType &mimeType : *mimeTypes)
    {
        ImageFormat fmt = ImageFormat::formatForMime(mimeType);
        if (!fmt.isValid()) continue;			// format for that MIME type
							// see if it should be used
        if (!FormatDialog::isCompatible(mimeType, type, recOnly)) continue;
        filters << mimeType.name();			// yes, add to filter list
    }

    // To allow the user to save to a format for which there may not
    // be a filter entry, the "All files" type is added.  If this filter is
    // selected when the dialogue completes, the applicable MIME type is
    // deduced from the full save URL.
    filters << "application/octet-stream";

    dlg.setMimeTypeFilters(filters);

    QString saveMime = mLastSaveMime;
    if (saveMime.isEmpty()) saveMime = KookaSettings::destinationSaveMime();
    if (!saveMime.isEmpty())
    {
        if (filters.contains(saveMime)) dlg.selectMimeTypeFilter(saveMime);
        else dlg.selectMimeTypeFilter(filters.last());
    }

    QUrl saveUrl = mLastSaveUrl;
    if (!saveUrl.isValid()) saveUrl = KookaSettings::destinationSaveDest();
    if (saveUrl.isValid()) dlg.setDirectoryUrl(saveUrl);

    if (!dlg.exec()) return (false);

    const QList<QUrl> urls = dlg.selectedUrls();
    if (urls.isEmpty()) return (false);

    mSaveUrl = urls.first();

    mSaveMime = dlg.selectedMimeTypeFilter();
    // If the "All files" filter is selected, deduce the applicable MIME type
    // from the save URL.
    if (mSaveMime==filters.last())
    {
        QMimeDatabase db;
        QMimeType mimeType = db.mimeTypeForUrl(mSaveUrl);
        ImageFormat fmt = ImageFormat::formatForMime(mimeType);
        if (!fmt.isValid())
        {
            KMessageBox::error(parentWidget(),
                               xi18nc("@info", "Cannot save to <filename>%2</filename><nl/>The image format <resource>%1</resource> is not supported.",
                                      mimeType.name(), mSaveUrl.toDisplayString()),
                               i18n("Cannot Save Image"));
            return (false);
        }

        mSaveMime = mimeType.name();			// use resolved MIME type
    }

    qCDebug(DESTINATION_LOG) << "url" << mSaveUrl << "mime" << mSaveMime;
    return (true);
}
