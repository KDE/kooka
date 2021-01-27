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

#include <kpluginfactory.h>
#include <klocalizedstring.h>
#include <kmessagebox.h>
#include <kprotocolinfo.h>

#include <kio/statjob.h>
#include <kio/copyjob.h>
#include <kio/jobuidelegate.h>

#include "scanparamspage.h"
#include "kookasettings.h"
#include "imageformat.h"
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

    // Honour the "Ask for filename before/after scan" setting
    if (KookaSettings::saverAskBeforeScan())
    {
        if (!getSaveLocation()) return (false);
    }

    return (true);
}


void DestinationSave::imageScanned(ScanImage::Ptr img)
{
    qCDebug(DESTINATION_LOG) << "received image size" << img->size();

    if (!mSaveUrl.isValid())				// if we didn't ask before,
    {							// then do it now
        if (!getSaveLocation()) return;
    }
    if (!mSaveUrl.isValid()) return;			// nowhere to save to

    ImageFormat fmt = getSaveFormat(mSaveMime, img);
    qCDebug(DESTINATION_LOG) << "to" << mSaveUrl << "mime" << mSaveMime << "format" << fmt;
    if (!fmt.isValid()) return;				// should never happen

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
            job->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, parentWidget()));
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
            KMessageBox::sorry(parentWidget(),
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
        job->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, parentWidget()));
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


bool DestinationSave::getSaveLocation()
{
    // Using the non-static QFileDialog so as to be able to
    // set MIME type filters.
    QFileDialog dlg(parentWidget(), i18n("Save Scan"));
    dlg.setAcceptMode(QFileDialog::AcceptSave);

    // TODO: if KookaSettings::saverOnlyRecommendedTypes() is set,
    // ask FormatDialog whether the MIME type is recommended
    // and ignore it if it is not

    QStringList filters;
    const QList<QMimeType> *mimeTypes = ImageFormat::mimeTypes();
    for (const QMimeType &mimeType : *mimeTypes)
    {
        filters << mimeType.name();
    }
    qDebug() << "filters" << filters;
    dlg.setMimeTypeFilters(filters);

    const QString saveMime = KookaSettings::destinationSaveMime();
    if (!saveMime.isEmpty()) dlg.selectMimeTypeFilter(saveMime);
    const QUrl saveUrl = KookaSettings::destinationSaveDest();
    if (saveUrl.isValid()) dlg.setDirectoryUrl(saveUrl);

    if (!dlg.exec()) return (false);

    const QList<QUrl> urls = dlg.selectedUrls();
    if (urls.isEmpty()) return (false);

    mSaveUrl = urls.first();
    mSaveMime = dlg.selectedMimeTypeFilter();
    qCDebug(DESTINATION_LOG) << "url" << mSaveUrl << "mime" << mSaveMime;
    return (true);
}
