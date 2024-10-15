/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2021-2024    Jonathan Marten <jjm@keelhaul.me.uk>	*
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
#include <kio/filecopyjob.h>
#include <kio/jobuidelegatefactory.h>

#include "scanparamspage.h"
#include "kookasettings.h"
#include "imageformat.h"
#include "formatdialog.h"
#include "imgsaver.h"
#include "destination_logging.h"


K_PLUGIN_FACTORY_WITH_JSON(DestinationSaveFactory, "kookadestination-save.json", registerPlugin<DestinationSave>();)
#include "destinationsave.moc"


// TODO: should default numbering start at 0 or 1? Maybe a preference option?
static const int START_NUM = 1;


DestinationSave::DestinationSave(QObject *pnt, const QVariantList &args)
    : AbstractDestination(pnt, "DestinationSave")
{
    mLastUsedNumber = START_NUM;
}


void DestinationSave::batchStart(const MultiScanOptions *opts)
{
    mBatchFirst = true;					// note this is a new batch
    mMultiOptions = opts;				// save options for reference
    mSaveMime.clear();					// reset for start of batch

    AbstractDestination::batchStart(opts);
}


bool DestinationSave::scanStarting(ScanImage::ImageType type)
{
    const MultiScanOptions::Flags f = mMultiOptions->flags();
    QUrl saveUrl;

    // If this is the subsequent scan in a batch (which implies that
    // a multiple batch scan is in progress), then it may not be
    // necessary to prompt at all, or only with limited filters.
    if (!mBatchFirst && mSaveLocation.isValid() && !mSaveMime.isEmpty())
    {
        // Only limit or suppress the prompting if multiple scan
        // batching is requested.
        if (f.testFlag(MultiScanOptions::BatchMultiple))
        {
            // If automatic generation of file names is enabled, then no
            // prompting is required at all.  imageScanned() will generate
            // the next file name using the previous one as a template.
            if (f.testFlag(MultiScanOptions::AutoGenerate)) return (true);

            // If automatic file name generation is disabled, then prompt
            // again for a file name but with the filter preset to that
            // previously used.
            saveUrl = getSaveLocation(type, false);
            if (!saveUrl.isValid()) return (false);
        }
    }

    // The first scan of a batch, or a single scan.  Ask for the save
    // location and file type.  The standard setting is now to always
    // ask for these before starting a scan.
    if (!saveUrl.isValid())				// if not requested above
    {
        saveUrl = getSaveLocation(type, true);
        if (!saveUrl.isValid()) return (false);
    }

    // The file dialogue will have encoded any '#' characters to "%23".
    // Obviously they need to be recognised as themselves here, so any
    // use of QUrl::fileName() or QUrl::path() from here on must use
    // QUrl::FullyDecoded.  Consequently, any use of QUrl::setPath()
    // must use QUrl::DecodedMode to reverse the decoding.
    QString fileName = saveUrl.fileName(QUrl::FullyDecoded);
    if (!fileName.contains('.')) return (false);	// must have a file extension

    // Reset the numbering sequence if the save location or file name
    // has changed.
    if (saveUrl!=mLastUserUrl)
    {
        mLastUsedNumber = START_NUM;
        qCDebug(DESTINATION_LOG) << "resetting number sequence to" << mLastUsedNumber;
    }

    mLastUserUrl = saveUrl;				// remember this as entered
    //qCDebug(DESTINATION_LOG) << "set mLastUserUrl to" << mLastUserUrl;
    mAutoLength = -1;					// assume do not generate

    // See whether the entered URL could potentially be used for auto file
    // name generation if required, and if so save the parsed template
    // information.
    if (f.testFlags(MultiScanOptions::MultiScan|MultiScanOptions::BatchMultiple|MultiScanOptions::AutoGenerate))
    {
        // Loop until that information has been obtained or until it has been
        // decided that it does not apply, although in practice this loop will
        // be executed at most twice.
        while (true)
        {
            qCDebug(DESTINATION_LOG) << "checking" << fileName << "for auto increment";
            int autoIndex = -1;

            // First see whether the file name contains a series of '#' characters.
            // If it does, this marks an auto incrementing number.  If the user is
            // being really perverse there may be more than one such sequence, so
            // arbitrarily choose the rightmost one.
            //
            // Match a captured sequence of '#' characters, followed by an arbitrary
            // sequence with no '#' characters, reaching to the end of the string.
            // Assuming that extracting substrings will be more efficient than an
            // anchored regexp with 3 capture groups, but this code path is not really
            // critical anyway.
            const QRegularExpression rx1("(#+)[^#]*$");
            const QRegularExpressionMatch match1 = rx1.match(fileName);
            if (match1.hasMatch())
            {
                autoIndex = match1.capturedStart(1);
                mAutoLength = match1.capturedLength(1);
            }
            else
            {
                // Match a captured sequence of digits, followed by a required
                // literal '.' and a file extension.  This matching is more
                // rigourous than the matching for '#' above because digits are
                // more likely to be intended within the file name, so we only
                // want to match a numeric suffix.  The literal '.' is required
                // so that, for example, the last digit in a ".jp2" extension is
                // not matched.
                const QRegularExpression rx2("(\\d+)\\.[^\\.]+$");
                const QRegularExpressionMatch match2 = rx2.match(fileName);

                if (match2.hasMatch())
                {
                    autoIndex = match2.capturedStart(1);
                    mAutoLength = match2.capturedLength(1);
                    // Explicitly specified by the user, so start at that number.
                    mLastUsedNumber = match2.captured(1).toInt();
                }
            }

            // If either of those patterns matched, then record the invariant
            // prefix and suffix before and after the substitution respectively.
            if (autoIndex!=-1)
            {
                mAutoPrefix = fileName.left(autoIndex);
                mAutoSuffix = fileName.mid(autoIndex+mAutoLength);
                qCDebug(DESTINATION_LOG) << "auto found, before" << mAutoPrefix << "len" << mAutoLength << "after" << mAutoSuffix;
                break;
            }
            else
            {
                // At this point there was no match for either possibility, but
                // automatically generating file names requires a file pattern.
                // Modify the file name to be able to be automatically generated,
                // by making the simplest possible transformation - adding a
                // single numeric digit at the end of the base name.
                const int idx = fileName.lastIndexOf('.');
                if (idx==-1)				// there should always be one
                {
                    // This error should never happen, because the file name must
                    // include a file extension at this stage.
                    KMessageBox::error(parentWidget(),
                                       xi18nc("@info", "<para>Automatically generating file names requires a template name.</para>"
                                              "<para>The file name <filename>%1</filename> should either contain a sequence<nl/>"
                                              "of '<icode>#</icode>' characters as a placeholder, or end with a sequence<nl/>"
                                              "of numeric digits before the file extension.</para>", fileName),
                                       i18n("Cannot Auto Generate"));
                    return (false);
                }

                fileName.insert(idx, QString::number(START_NUM));
                qCDebug(DESTINATION_LOG) << "no auto found, modified to" << fileName;
            }
        }
    }

    mSaveFileName = fileName;				// the possibly modified file name
    qCDebug(DESTINATION_LOG) << "save file name" << mSaveFileName;

    // Find out whether the save destination is local and, if so, its true
    // destination.  Because the destination file can be assumed not to exist
    // yet, run the KIO::StatJob on its containing directory.  This also means
    // that an automatic file name does not need to be generated until later on
    // when the local/remote status is known.
    mSaveLocation = saveUrl.adjusted(QUrl::RemoveFilename);
    qCDebug(DESTINATION_LOG) << "initial parent" << mSaveLocation.toDisplayString() << "local?" << mSaveLocation.isLocalFile();
    if (!mSaveLocation.isLocalFile())
    {
        if (KProtocolInfo::protocolClass(mSaveLocation.scheme())==QLatin1String(":local"))
        {
            qCDebug(DESTINATION_LOG) << "maybe local, running a StatJob";

            // Even though the purpose of the StatJob is just to get information,
            // an error is displayed if the destination file does not exist (as
            // it is assumed it does not at this stage).  To avoid this situation
            // but to still get errors and/or warnings for other problems (e.g.
            // an unreachable host, authentication failure etc), give the StatJob
            // the URL of the containing directory;  the file name will be added
            // back afterwards.
            KIO::StatJob *job = KIO::mostLocalUrl(mSaveLocation);
            job->setSide(KIO::StatJob::DestinationSide);
            job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, parentWidget()));
            if (job->exec())
            {
                const QUrl u = job->mostLocalUrl();
                if (!u.isValid())
                {
                    KMessageBox::error(parentWidget(),
                                       xi18nc("@info", "Cannot access remote location <filename>%1</filename>",
                                              mSaveLocation.toDisplayString()),
                                       i18n("Cannot Save Image"));
                    return (false);
                }

                mSaveLocation = u;			// the most local location
            }
        }
    }
    qCDebug(DESTINATION_LOG) << "resolved parent" << mSaveLocation.toDisplayString() << "local?" << mSaveLocation.isLocalFile();

    // At this point the resolved save information is:
    //
    //   mLastUserUrl		The save URL entered by the user
    //   mSaveLocation		Its parent directory, resolved to the most local form
    //   mSaveFileName		The save file name entered by the user, but possibly
    //				modified if a numeric suffix had to be added
    //   mAutoLength		The length of the auto generated number placeholder,
    //				or -1 if there is to be no automatic generation
    //   mAutoPrefix		The prefix part before the placeholder
    //   mAutoSuffix		The suffix part after the placeholder
    //   mLastUsedNumber	The initial number that should be substituted
    //   mSaveMime		The MIME type to be saved
    //
    // These will be used when the scanned image arrives, but for now
    // there is no more to do.
    return (true);
}


void DestinationSave::imageScanned(ScanImage::Ptr img)
{
    qCDebug(DESTINATION_LOG) << "received image size" << img->size();

    ImageFormat fmt = getSaveFormat(mSaveMime, img);
    qCDebug(DESTINATION_LOG) << "saveLocation=" << mSaveLocation.toDisplayString()
                             << "islocal?" << mSaveLocation.isLocalFile()
                             << "saveFileName" << mSaveFileName
                             << "saveMime" << mSaveMime
                             << "autoLength" << mAutoLength
                             << "lastUsedNumber" << mLastUsedNumber
                             << "format" << fmt;

    // These should now never happen, because getSaveLocation() was
    // called unconditionally above.
    if (!fmt.isValid()) return;
    if (!mSaveLocation.isValid()) return;

    const QString parentPath = mSaveLocation.adjusted(QUrl::StripTrailingSlash).path(QUrl::FullyDecoded);

    if (mAutoLength!=-1)
    {
        // If a different file name or save location has been entered
        // this time, then then mLastUsedNumber will have been reset in
        // scanStarting() above.
        int num = mLastUsedNumber;
        qCDebug(DESTINATION_LOG) << "starting number" << num;

        // If an automatic file name is to be generated, then loop to
        // find a generated name that does not already exist.
        //
        // Remote files (which could not be resolved to a local location
        // above) are handled in the same way as local ones.  It could be
        // argued that it would be more efficient to list the remote
        // directory using just one ListJob job and then use the resulting
        // list of file names to find an unused one, but it is expected
        // that most of the time the prospective file will not exist.
        // Therefore at most a single StatJob will be needed anyway.
        for (;;)
        {
            QString fileName = mAutoPrefix;
            fileName += QString("%1").arg(num, mAutoLength, 10, QLatin1Char('0'));
            fileName += mAutoSuffix;
            QString path = parentPath+'/'+fileName;

            bool exists;
            if (mSaveLocation.isLocalFile())		// local
            {
                qCDebug(DESTINATION_LOG) << "prospective path" << path;
                exists = QFile::exists(path);
            }
            else					// remote
            {
                QUrl u = mSaveLocation;
                u.setPath(path, QUrl::DecodedMode);
                qCDebug(DESTINATION_LOG) << "prospective url" << u.toDisplayString();
                KIO::StatJob *job = KIO::stat(u, KIO::StatJob::DestinationSide, KIO::StatNoDetails);
                // Don't want any automatic error reporting, because the file
                // is expected not to exist.  If the file does not exist an
                // error is reported even for StatJob::DestinationSide as
                // above.  The parent path will have already been checked and
                // an error reported if it is not accessible.
                job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingDisabled, parentWidget()));

                exists = job->exec();
            }

            if (!exists)
            {
                qCDebug(DESTINATION_LOG) << "does not exist, using" << fileName;
                mSaveFileName = fileName;

                // Assume that the next numbered file will be at least one more
                // than this one.
                mLastUsedNumber = num+1;
                break;
            }

            // The prospective name already exists, so increment the number
            // to generate a new file name and check again.
            qCDebug(DESTINATION_LOG) << "already exists";
            ++num;
        }
    }

    // Use the file name either as generated or as entered literally,
    // but in both cases use the most local path as found above.
    QUrl saveUrl = mSaveLocation;
    QString path = parentPath+'/'+mSaveFileName;
    // How many times have we wished for QUrl::setFileName()?
    saveUrl.setPath(path, QUrl::DecodedMode);

    if (mSaveLocation.isLocalFile())			// did we find a local path?
    {
        // The save location is local, so we can use ImgSaver directly.
        // This assumes that if 'mSaveLocation', the parent of the file
        // to save, is a local file then the file to save 'saveUrl' is also.
        qCDebug(DESTINATION_LOG) << "save to local" << saveUrl.toLocalFile();

        ImgSaver saver(saveUrl.adjusted(QUrl::RemoveFilename));
        ImgSaver::ImageSaveStatus status = saver.saveImage(img, saveUrl, fmt);
        if (status!=ImgSaver::SaveStatusOk)
        {
            KMessageBox::error(parentWidget(),
                               xi18nc("@info", "Cannot save image to <filename>%1</filename><nl/><message>%2</message>",
                                      saveUrl.toDisplayString(), saver.errorString(status)),
                               i18n("Cannot Save Image"));
        }
    }
    else						// save location is not local
    {
        qCDebug(DESTINATION_LOG) << "save to remote" << saveUrl.toDisplayString();

        // The save location is remote.  Save a temporary image,
	// then use KIO to move it to the destination location.
        QUrl tempUrl = saveTempImage(fmt, img);		// save a temporary image
        if (!tempUrl.isValid()) return;			// temp image save failed

        KIO::FileCopyJob *job = KIO::file_move(tempUrl, saveUrl);
        job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, parentWidget()));
        job->exec();					// move temp file to remote
    }

    // Remember the values used, so that starting a new scan and then
    // cancelling it will not clear them.  See scanStarting() and
    // saveSettings().
    mLastSaveMime = mSaveMime;

    mBatchFirst = false;				// continuing with batch
}


KLocalizedString DestinationSave::scanDestinationString()
{
    // Unfortunately this cannot report the full URL for auto generated
    // file names because this is called by KookaView::slotScanStart()
    // immediately after scanStarting(), but the actual file name being
    // saved is not available until imageScanned().
    if (!mSaveLocation.isValid()) return (ki18n("Saving image"));
    QUrl u = mSaveLocation;
    if (mAutoLength==-1) u.setPath(u.path(QUrl::FullyDecoded)+mSaveFileName, QUrl::DecodedMode);
    return (kxi18n("Saving to <filename>%1</filename>").subs(u.toDisplayString()));
}


void DestinationSave::saveSettings() const
{
    if (mLastUserUrl.isValid()) KookaSettings::setDestinationSaveDest(mLastUserUrl.adjusted(QUrl::RemoveFilename));
    if (!mLastSaveMime.isEmpty()) KookaSettings::setDestinationSaveMime(mLastSaveMime);
}


QUrl DestinationSave::getSaveLocation(ScanImage::ImageType type, bool allFilters)
{
    qCDebug(DESTINATION_LOG) << "for type" << type;

    // Using the non-static QFileDialog so as to be able to
    // set MIME type filters.
    QFileDialog dlg(parentWidget(), i18n("Save Scan"));
    dlg.setAcceptMode(QFileDialog::AcceptSave);

    QStringList filters;
    if (!allFilters && !mLastSaveMime.isEmpty())
    {
        // If this is a subsequent scan in a batch but automatic file
        // naming is disabled, then prompt for a file name to save to.
        // In this case the only filter offered will be the MIME type
        // selected for the previous scan, although there is nothing to
        // prevent the user from entering a different extension.
        qCDebug(DESTINATION_LOG) << "using previous filter" << mLastSaveMime;
        filters.append(mLastSaveMime);
    }
    else
    {
        // Allow the full filter list.  First check whether to only
        // offer recommended save formats for the image type.
        const bool recOnly = KookaSettings::saverOnlyRecommendedTypes();

        const QList<QMimeType> *mimeTypes = ImageFormat::mimeTypes();
        for (const QMimeType &mimeType : *mimeTypes)
        {
            ImageFormat fmt = ImageFormat::formatForMime(mimeType);
            if (!fmt.isValid()) continue;		// format for that MIME type
							// see if it should be used
            if (!FormatDialog::isCompatible(mimeType, type, recOnly)) continue;
            filters.append(mimeType.name());		// yes, add to filter list
        }

        // To allow the user to save to a format for which there may not
        // be a filter entry, the "All files" type is added.  If this filter is
        // selected when the dialogue completes, the applicable MIME type is
        // deduced from the full save URL.
        filters << "application/octet-stream";
    }
    dlg.setMimeTypeFilters(filters);

    QString saveMime = mLastSaveMime;
    if (saveMime.isEmpty()) saveMime = KookaSettings::destinationSaveMime();
    if (!saveMime.isEmpty())
    {
        if (filters.contains(saveMime)) dlg.selectMimeTypeFilter(saveMime);
        else dlg.selectMimeTypeFilter(filters.last());
    }

    // The save location as entered by the user, even if it was resolved
    // to a more local location for saving.
    QUrl saveUrl = mLastUserUrl.adjusted(QUrl::RemoveFilename);
    if (!saveUrl.isValid()) saveUrl = KookaSettings::destinationSaveDest();
    if (saveUrl.isValid()) dlg.setDirectoryUrl(saveUrl);

    // If the last entered file name was a '#' pattern, then offer the
    // same pattern as the default file name.  The numbering will start
    // at the next in sequence, as intended.
    //
    // This is the only sensible situation in which to preset a file name.
    const QString lastFile = mLastUserUrl.fileName();
    if (lastFile.contains('#')) dlg.selectFile(lastFile);

    if (!dlg.exec()) return (QUrl());

    const QList<QUrl> urls = dlg.selectedUrls();
    if (urls.isEmpty()) return (QUrl());
    saveUrl = urls.first();

    mSaveMime = dlg.selectedMimeTypeFilter();
    // If the "All files" filter is selected, try to deduce the applicable
    // MIME type from the save URL.
    if (mSaveMime==filters.last())
    {
        QMimeDatabase db;
        const QMimeType mimeType = db.mimeTypeForUrl(saveUrl);
        if (!ImageFormat::formatForMime(mimeType).isValid())
        {
            QString msg;
            if (mimeType.isDefault())			// application/octet-stream
            {
                msg = xi18nc("@info", "Cannot save to <filename>%2</filename><nl/>Could not find an image format for the file name <filename>%1</filename>.",
                             saveUrl.fileName(), saveUrl.toDisplayString());
            }
            else					// known but not an image format
            {
                msg = xi18nc("@info", "Cannot save to <filename>%2</filename><nl/>The image format <resource>%1</resource> is not supported.",
                             mimeType.name(), saveUrl.toDisplayString());
            }

            KMessageBox::error(parentWidget(), msg, i18n("Cannot Save Image"));
            return (QUrl());
        }

        mSaveMime = mimeType.name();			// use resolved MIME type
    }

    qCDebug(DESTINATION_LOG) << "->" << saveUrl.toDisplayString() << "mime" << mSaveMime;
    return (saveUrl);
}


MultiScanOptions::Capabilities DestinationSave::capabilities() const
{
    return (MultiScanOptions::AcceptBatch|MultiScanOptions::FileNames);
}
