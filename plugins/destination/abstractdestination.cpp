/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2000-2016 Klaas Freitag <freitag@suse.de>		*
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

#include "abstractdestination.h"

#include <qdir.h>
#include <qmimetype.h>
#include <qmimedatabase.h>
#include <qtemporaryfile.h>
#include <qcoreapplication.h>
#include <qcombobox.h>
#include <qstandardpaths.h>
#include <qcheckbox.h>
#include <qprocess.h>

#include <kmessagebox.h>

#include "formatdialog.h"
#include "destination_logging.h"


//  Constructor/destructor and plugin creation
//  ------------------------------------------

AbstractDestination::AbstractDestination(QObject *pnt, const char *name)
    : AbstractPlugin(pnt),
      mLastUsedFormat(ImageFormat(""))
{
    setObjectName(name);
    qCDebug(DESTINATION_LOG) << objectName();

    mGallery = nullptr;
    mMultiOptions = nullptr;
}


ImageFormat AbstractDestination::getSaveFormat(const QString &mimeName, ScanImage::Ptr img)
{
    ImageFormat fmt("");
    if (!mimeName.isEmpty())				// have selected a MIME type
    {
        QMimeDatabase db;				// get image format to use
        fmt = ImageFormat::formatForMime(db.mimeTypeForName(mimeName));
        if (!fmt.isValid()) qCWarning(DESTINATION_LOG) << "No MIME type or format for" << mimeName;
    }

    // If the format is "Other", or there was an error finding the MIME type,
    // then prompt for a format.
    if (!fmt.isValid()) fmt = mLastUsedFormat;		// try the remembered format
    if (!fmt.isValid())					// no remembered format
    {
        FormatDialog fd(parentWidget(),			// parent
                        img->imageType(),		// type
                        true,				// askForFormat
                        fmt,				// default format
                        false,				// askForFilename
                        QString());			// filename
							// special text for check box
        fd.alwaysUseFormatCheck()->setText(i18n("Remember this format"));

        if (!fd.exec()) return (fmt);			// dialogue cancelled
        fmt = fd.getFormat();				// get the selected format

        // If the "Remember" check box is ticked, then note the format for
        // next time.  The format is only remembered for the lifetime of this
        // plugin, and does not affect the main Kooka setting.
        if (fd.alwaysUseFormat()) mLastUsedFormat = fmt;
    }

    // TODO: even if there is no need to ask for a format then maybe ask
    // for a file name, so that it will be shown at the other end of, e.g.
    // a Bluetooth share.

    qCDebug(DESTINATION_LOG) << "format" << fmt << "ext" << fmt.extension();
    return (fmt);
}


QUrl AbstractDestination::saveTempImage(const ImageFormat &fmt, ScanImage::Ptr img)
{
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
        KMessageBox::error(parentWidget(), xi18nc("@info", "Cannot save temporary image file<nl/><filename>%1</filename><nl/><message>%2</message>",
                                                  saveUrl.toDisplayString(), saver.errorString(status)));
        temp.setAutoRemove(true);			// clean up temporary file
        return (QUrl());				// indicate could not save
    }

    return (saveUrl);					// of the temporary file
}


QComboBox *AbstractDestination::createFormatCombo(const QStringList &mimeTypes,
                                                  const QString &configuredMime)
{
    // For the image format combo, we do not yet know the format of the
    // scanned image.  Therefore the approach taken here, trying to balance
    // versatility against not confusing the user with a long list of obscure
    // image file formats, is to offer a small predetermined list of popular
    // formats in the combo, followed by an "Other..." option.  The last
    // option uses the Save Assistant to select an image type (but without the
    // option to enter the file name).
    //
    // It is the user's responsibility to make sure that the selected
    // application accepts the selected format, so the explicit formats should
    // be ones that are accepted by most common applications.

    QComboBox *combo = new QComboBox;
    int configuredIndex = -1;

    QMimeDatabase db;
    for (const QString &mimeName : mimeTypes)
    {
        const QMimeType mimeType = db.mimeTypeForName(mimeName);
        const ImageFormat fmt = ImageFormat::formatForMime(mimeType);
        if (!fmt.isValid()) continue;			// this format not supported

        if (mimeName==configuredMime) configuredIndex = combo->count();
        combo->addItem(QIcon::fromTheme(mimeType.iconName()), mimeType.comment(), mimeType.name());
    }

    if (configuredMime=="") configuredIndex = combo->count();
    combo->addItem(QIcon::fromTheme("system-run"), i18n("Other..."));
    if (configuredIndex!=-1) combo->setCurrentIndex(configuredIndex);

    return (combo);					// the created combo box
}


void AbstractDestination::delayedDelete(const QUrl &url)
{
    delayedDelete(QList<QUrl>() << url);
}


void AbstractDestination::delayedDelete(const QList<QUrl> &urls)
{
    // A destination may need the scanned image to persist until a share or
    // export is complete, and it may not even be able to tell when an image
    // is no longer required.  So as to be absolutely sure that the file
    // stays around for enough time, even if the application is quit, use the
    // kioexec utility (normally used to open a remote file in an application
    // that does not support KIO) to not do anything with the file but delete
    // it after three minutes have passed.  Calling this in a detached process
    // allows Kooka to exit cleanly even if waiting for that time delay.
    //
    // If the temporary file is in a subdirectory, then kioexec will also
    // delete the containing directory if it is empty.

    // from second test in kio/src/core/desktopexecparser.cpp
    const QString kioexec = (LIBEXEC_DIR "/kioexec");
    QStringList args;
    args << "--tempfiles";					// delete temporary files
    args << QStandardPaths::findExecutable("true")+" %F";	// do nothing with the files
    for (const QUrl &url : qAsConst(urls))
    {
        if (url.isLocalFile()) args << url.url();		// should always be the case
    }

    qCDebug(DESTINATION_LOG) << "running" << kioexec << args;
    if (!QProcess::startDetached(kioexec, args)) qCWarning(DESTINATION_LOG) << "Cannot start detached process";
}
