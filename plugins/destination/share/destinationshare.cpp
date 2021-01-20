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

#include "destinationshare.h"

#include <qdir.h>
#include <qcombobox.h>
#include <qmimetype.h>
#include <qmimedatabase.h>
#include <qtemporaryfile.h>
#include <qjsonarray.h>

#include <kpluginfactory.h>
#include <klocalizedstring.h>
#include <kmessagebox.h>

#include <kio/global.h>

#include <purpose/alternativesmodel.h>
#include <purposewidgets/menu.h>

#include "scanparamspage.h"
#include "kookasettings.h"
#include "imageformat.h"
#include "formatdialog.h"
#include "imgsaver.h"
#include "destination_logging.h"


K_PLUGIN_FACTORY_WITH_JSON(DestinationShareFactory, "kookadestination-share.json", registerPlugin<DestinationShare>();)
#include "destinationshare.moc"


DestinationShare::DestinationShare(QObject *pnt, const QVariantList &args)
    : AbstractDestination(pnt, "DestinationShare")
{
    // The list of available share destinations can be obtained simply from
    // an AlternativesModel, but we create a Purpose::Menu so as to be able
    // to also use it for launching the share job.  Otherwise the whole of
    // Purpose::MenuPrivate::trigger() would need to be duplicated in
    // imageScanned().
    mMenu = new Purpose::Menu(parentWidget());
    connect(mMenu, &Purpose::Menu::finished, this, &DestinationShare::slotShareFinished);

    // This is the Purpose::AlternativesModel created by the Purpose::Menu.
    mModel = mMenu->model();
}


void DestinationShare::imageScanned(ScanImage::Ptr img)
{
    qCDebug(DESTINATION_LOG) << "received image size" << img->size();
    const QString shareService = mShareCombo->currentData().toString();
    const QString mimeName = mFormatCombo->currentData().toString();
    qCDebug(DESTINATION_LOG) << "share" << shareService << "mime" << mimeName;

    // Get a format to save the scanned image.  If a valid one is selected,
    // the use that.  If "Other" is selected or the selected format is not
    // valid for saving (this shouldn't happen), then use the FormatDialog
    // to prompt for a format.
    ImageFormat fmt("");
    if (!mimeName.isEmpty())				// have selected a MIME type
    {
        QMimeDatabase db;				// get image format to use
        fmt = ImageFormat::formatForMime(db.mimeTypeForName(mimeName));
        if (!fmt.isValid()) qCWarning(DESTINATION_LOG) << "No MIME type or format for" << mimeName;
    }

    // If the format is "Other", or there was an error finding the MIME type,
    // then prompt for a format.
    if (!fmt.isValid())
    {
        FormatDialog fd(parentWidget(),			// parent
                        img->imageType(),		// type
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

    // TODO: even if there is no need to ask for a format then ask for a file name,
    // so that it will be shown at the other end of e.g. a Bluetooth share.
    // May need a temporary directory so as to be able to save with the requested
    // file name.

    qCDebug(DESTINATION_LOG) << "format" << fmt << "ext" << fmt.extension();
    if (!fmt.isValid()) return;				// must have this now

    // Save the image to a temporary file, in the format specified.
    QTemporaryFile temp(QDir::tempPath()+"/"+QCoreApplication::applicationName()+"XXXXXX."+fmt.extension());
    temp.setAutoRemove(false);
    temp.open();
    QUrl saveUrl = QUrl::fromLocalFile(temp.fileName());
    temp.close();					// now have name, but do not remove
    qCDebug(DESTINATION_LOG) << "save to" << saveUrl;	// temporary file location

    ImgSaver saver;					// save the image
    ImgSaver::ImageSaveStatus status = saver.saveImage(img, saveUrl, fmt);
    if (status!=ImgSaver::SaveStatusOk)			// image save failed
    {
        KMessageBox::sorry(parentWidget(),
                           xi18nc("@info", "Cannot save image file<nl/><filename>%1</filename><nl/>%2",
                                  saveUrl.toDisplayString(), saver.errorString(status)));
        temp.setAutoRemove(true);			// clean up temporary file
        return;
    }

    // Because we did not know the specific MIME type at the time, the
    // original menu and hence the share destination combo box will have
    // been filled with actions for the generic "image/*" MIME type.
    // Now the MIME type is known and we need to specify it.
    // 
    // Hopefully the list of shared destinations will not change as a
    // result of the more specific MIME type.  Just in case it does,
    // note the selected share ID before setting the MIME type and
    // find it again before triggering the menu action.

    QJsonObject dataObject;
    dataObject.insert("mimeType", QJsonValue(mimeName));
    QJsonArray dataUrls;
    dataUrls.append(saveUrl.url());
    dataObject.insert("urls", dataUrls);

    mModel->setInputData(dataObject);			// set MIME type and URL
    mMenu->reload();					// regenerate the menu

    int foundRow = -1;
    for (int i = 0; i<mModel->rowCount(); ++i)		// search through new model
    {
        QModelIndex idx = mModel->index(i, 0);
        const QString key = mModel->data(idx, Purpose::AlternativesModel::PluginIdRole).toString();
        if (key==shareService)
        {
            foundRow = i;
            break;
        }
    }

    if (foundRow==-1)					// couldn't find share in new menu
    {
        qCWarning(DESTINATION_LOG) << "Cannot find service for updated MIME type, count" << mModel->rowCount();
        return;
    }

    QAction *act = mMenu->actions().value(foundRow);	// get action from menu
    Q_ASSERT(act!=nullptr);
    act->trigger();					// do the share action

    // Nothing more to do now, the temporary file will get deleted
    // in slotShareFinished().
}


void DestinationShare::createGUI(ScanParamsPage *page)
{
    qCDebug(DESTINATION_LOG);

    // TODO: move to destination plugin library
    // See the "Application" plugin for comments
    mFormatCombo = new QComboBox;

    const QString configuredMime = KookaSettings::destinationShareMime();
    int configuredIndex = -1;

    QMimeDatabase db;
    for (const QString &mimeName : {"image/png",
                                    "image/jpeg",
                                    "image/tiff"})
    {
        const QMimeType mimeType = db.mimeTypeForName(mimeName);
        const ImageFormat fmt = ImageFormat::formatForMime(mimeType);
        if (!fmt.isValid()) continue;                   // this format not supported

        if (mimeName==configuredMime) configuredIndex = mFormatCombo->count();
        mFormatCombo->addItem(QIcon::fromTheme(mimeType.iconName()), mimeType.comment(), mimeType.name());
    }
    if (configuredMime=="") configuredIndex = mFormatCombo->count();
    mFormatCombo->addItem(QIcon::fromTheme("system-run"), i18n("Other..."));
    if (configuredIndex!=-1) mFormatCombo->setCurrentIndex(configuredIndex);
    connect(mFormatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DestinationShare::slotUpdateShareCombo);
    page->addRow(i18n("Image format:"), mFormatCombo);

    // The share destinations that are available depend on the MIME type
    // as selected above.
    mShareCombo = new QComboBox;
    slotUpdateShareCombo();
    page->addRow(i18n("Share to:"), mShareCombo);
}


KLocalizedString DestinationShare::scanDestinationString()
{
    // TODO: name the destination
    return (ki18n("Sharing image"));
}


void DestinationShare::saveSettings() const
{
    KookaSettings::setDestinationShareDest(mShareCombo->currentData().toString());
    KookaSettings::setDestinationShareMime(mFormatCombo->currentData().toString());
}


void DestinationShare::slotUpdateShareCombo()
{
    QString mimeType = mFormatCombo->currentData().toString();
    // If the MIME type is "Other" then we do not have the type
    // available at this stage, so accept plugins that can share
    // any image type.
    if (mimeType.isEmpty()) mimeType = "image/*";
    qCDebug(DESTINATION_LOG) << "for MIME" << mimeType;

    QString configuredShare = mShareCombo->currentData().toString();
    if (configuredShare.isEmpty()) configuredShare = KookaSettings::destinationShareDest();
    int configuredIndex = -1;
    qCDebug(DESTINATION_LOG) << "current" << configuredShare;

    QJsonObject dataObject;
    dataObject.insert("mimeType", QJsonValue(mimeType));
    dataObject.insert("urls", QJsonArray());		// not relevant at this stage
    mModel->setInputData(dataObject);
    mModel->setPluginType(QStringLiteral("Export"));
    qCDebug(DESTINATION_LOG) << "have" << mModel->rowCount() << "share destinations";

    mShareCombo->clear();
    for (int i = 0; i<mModel->rowCount(); ++i)
    {
        QModelIndex idx = mModel->index(i, 0);
        const QString key = mModel->data(idx, Purpose::AlternativesModel::PluginIdRole).toString();
        qCDebug(DESTINATION_LOG) << "  " << i << key << mModel->data(idx, Qt::DisplayRole).toString();

        if (!configuredShare.isEmpty() && key==configuredShare) configuredIndex = mShareCombo->count();

        mShareCombo->addItem(mModel->data(idx, Qt::DecorationRole).value<QIcon>(),
                             mModel->data(idx, Qt::DisplayRole).toString(), key);
    }

    if (configuredIndex!=-1) mShareCombo->setCurrentIndex(configuredIndex);
}


// copied from lambda in ShareFileItemAction::ShareFileItemAction()
void DestinationShare::slotShareFinished(const QJsonObject &output, int error, const QString &errorMessage)
{
    qCDebug(DESTINATION_LOG) << "error" << error << "output" << output;
        if (error == 0 || error == KIO::ERR_USER_CANCELED)
    {
        // TODO: report the URL (test case: Imgur)
//        if (output.contains(QLatin1String("url")))
//            QDesktopServices::openUrl(QUrl(output.value(QLatin1String("url")).toString()));
    }
    else
    {
        qWarning() << "job failed with error" << error << errorMessage << output;
        KMessageBox::sorry(parentWidget(),
                           xi18nc("@info", "Cannot share image<nl/><nl/><message>%1</message>", errorMessage));
    }

// TODO: the share job does not delete the temporary file, but it may need
// it to be present until the share is complete.  Delete the temp file here,
// calling libexec/kf5/kioexec with --tempfiles and '/bin/true %f' in a detached
// process so that Kooka can exit even if waiting to delete the file.
//
// This will work even if the temporary file is placed in a subdirectory, because
// kioexec does:
//
//            qDebug() << sleepSecs << "seconds have passed, deleting" << info.filePath();
//            QFile(src).remove();
//            // NOTE: this is not necessarily a temporary directory.
//            if (QDir().rmdir(parentDir)) {
//                qDebug() << "Removed empty parent directory" << parentDir;
//            }

}
