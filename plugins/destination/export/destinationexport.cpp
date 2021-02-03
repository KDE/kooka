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

#include "destinationexport.h"

#include <qcombobox.h>
#include <qaction.h>
#include <qprocess.h>
#include <qregularexpression.h>

#include <kpluginfactory.h>
#include <klocalizedstring.h>

#include "scanparamspage.h"
#include "kookasettings.h"
#include "destination_logging.h"


K_PLUGIN_FACTORY_WITH_JSON(DestinationExportFactory, "kookadestination-export.json", registerPlugin<DestinationExport>();)
#include "destinationexport.moc"


DestinationExport::DestinationExport(QObject *pnt, const QVariantList &args)
    : AbstractDestination(pnt, "DestinationExport")
{
    QStringList paths;
    paths << BUILDBIN_DIR << LIBEXEC_DIR;
    mHelper = QStandardPaths::findExecutable("kookakipihelper", paths);
    if (!mHelper.isEmpty()) qCDebug(DESTINATION_LOG) << "KIPI helper found at" << mHelper;
    else qCWarning(DESTINATION_LOG) << "KIPI helper executable not found!";
}


DestinationExport::~DestinationExport()
{
    qCDebug(DESTINATION_LOG) << "have saved" << mSavedFiles.count() << "temporary files";
    for (const QUrl &savedFile : mSavedFiles)
    {
        delayedDelete(savedFile);
    }
}


void DestinationExport::imageScanned(ScanImage::Ptr img)
{
    qCDebug(DESTINATION_LOG) << "received image size" << img->size();
    if (mHelper.isEmpty()) return;			// can't really do anything

    const QString exportName = mExportCombo->currentData().toString();
    const QString mimeName = mFormatCombo->currentData().toString();
    qCDebug(DESTINATION_LOG) << "export" << exportName << "mime" << mimeName;

    ImageFormat fmt = getSaveFormat(mimeName, img);	// get format for saving image
    if (!fmt.isValid()) return;				// must have this now
    mSaveUrl = saveTempImage(fmt, img);			// save to temporary file
    if (!mSaveUrl.isValid()) return;			// could not save image

    // Use the helper to perform the sharing.
    QProcess proc(this);
    proc.setProgram(mHelper);
    proc.setArguments(QStringList() << "-s" << exportName << mSaveUrl.url());
    proc.setProcessChannelMode(QProcess::ForwardedErrorChannel);

    qCDebug(DESTINATION_LOG) << "executing" << proc.program() << proc.arguments();
    // The export helper process may take an indeterminate time,
    // and will most likely need to put up a GUI.  So start it
    // detached and hope.
    if (!proc.startDetached()) qCWarning(DESTINATION_LOG) << "Cannot execute KIPI helper";

    // Record the temporary file.  There is nothing more to do here,
    // all temporary files will eventually be deleted by the destructor.
    mSavedFiles.append(mSaveUrl);
}


void DestinationExport::createGUI(ScanParamsPage *page)
{
    qCDebug(DESTINATION_LOG);

    // The MIME types that can be selected for sharing the image.
    QStringList mimeTypes;
    mimeTypes << "image/png" << "image/jpeg";
    mFormatCombo = createFormatCombo(mimeTypes, KookaSettings::destinationExportMime());
    page->addRow(i18n("Image format:"), mFormatCombo);

    // The available export destinations.
    mExportCombo = new QComboBox;
    page->addRow(i18n("Online service:"), mExportCombo);

    if (mHelper.isEmpty())				// if helper was not found,
    {							// can't really do anything
fail:   mFormatCombo->setEnabled(false);
        mExportCombo->setEnabled(false);
        return;
    }

    const QString configuredExport = KookaSettings::destinationExportDest();
    int configuredIndex = -1;

    // Use the helper to list the available destinations.
    QProcess proc(this);
    proc.setProgram(mHelper);
    proc.setArguments(QStringList() << "-l");
    proc.setProcessChannelMode(QProcess::ForwardedErrorChannel);
    qCDebug(DESTINATION_LOG) << "executing" << proc.program() << proc.arguments();
    proc.start();
    if (!proc.waitForFinished(10000))
    {
        qCWarning(DESTINATION_LOG) << "Cannot get list of actions from KIPI helper";
        // TODO: error with a KMessageWidget
        goto fail;
    }

    const QByteArray result = proc.readAllStandardOutput();
    const QList<QByteArray> lines = result.split('\n');
    const QRegularExpression rx("\\s{2,}");		// split on 2 or more spaces

    for (const QByteArray &line : lines)
    {
        //qCDebug(DESTINATION_LOG) << "read line" << line;

        // QString::split() is more versatile than QByteArray::split(),
        // which can only split on a single character.
        const QStringList splits = QString::fromLocal8Bit(line).split(rx, Qt::SkipEmptyParts);
        if (splits.count()<3) continue;			// not enough fields

        QStringList::const_iterator it = splits.begin();
        const QString actionName = (*it++);
        const QString actionIcon = (*it++);
        const QString actionText = QStringList(it, splits.constEnd()).join(' ');

        // The check on the KIPI::Category will already have been done by the
        // helper, so always accept the action.
        if (actionName==configuredExport) configuredIndex = mExportCombo->count();
        mExportCombo->addItem(QIcon::fromTheme(actionIcon), actionText, actionName);
    }

    qCDebug(DESTINATION_LOG) << "have" << mExportCombo->count() << "export actions";
    if (configuredIndex!=-1) mExportCombo->setCurrentIndex(configuredIndex);
}


KLocalizedString DestinationExport::scanDestinationString()
{
    return (kxi18n("<application>%1</application>").subs(mExportCombo->currentText()));
}


void DestinationExport::saveSettings() const
{
    if (!mFormatCombo->isEnabled()) return;		// error condition, don't save
    KookaSettings::setDestinationExportDest(mExportCombo->currentData().toString());
    KookaSettings::setDestinationExportMime(mFormatCombo->currentData().toString());
}
