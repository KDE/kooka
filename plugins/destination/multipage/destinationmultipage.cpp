/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2025      Jonathan Marten <jjm@keelhaul.me.uk>	*
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

#include "destinationmultipage.h"

#include <qcombobox.h>
#include <qpushbutton.h>
#include <qmimedatabase.h>
#include <qmimetype.h>
#include <qfiledialog.h>
#include <qtemporaryfile.h>
#include <qcoreapplication.h>
#include <qdir.h>

#include <kmessagebox.h>
#include <kpluginfactory.h>
#include <klocalizedstring.h>

#include <kio/filecopyjob.h>
#include <kio/jobuidelegatefactory.h>

#include "scanparamspage.h"
#include "recentsaver.h"
#include "kookaprint.h"
#include "kookasettings.h"
#include "papersizes.h"
#include "multipageoptionsdialog.h"
#include "abstractmultipagewriter.h"
#include "destination_logging.h"

#ifdef HAVE_TIFF
#include <qstandardpaths.h>
#include <qprocess.h>
#endif

K_PLUGIN_FACTORY_WITH_JSON(DestinationMultipageFactory, "kookadestination-multipage.json", registerPlugin<DestinationMultipage>();)
#include "destinationmultipage.moc"


//////////////////////////////////////////////////////////////////////////
//									//
//  MultipagePdfWriter							//
//									//
//////////////////////////////////////////////////////////////////////////

class MultipagePdfWriter : public AbstractMultipageWriter
{
public:
    MultipagePdfWriter();
    ~MultipagePdfWriter();

    void setPageSize(const QPageSize &pageSize) override;
    void setBaseImage(const QImage *img) override;
    void setOutputFile(const QString &fileName) override;
    bool startPrint() override;
    void endPrint() override;

protected:
    bool printImage(const QImage *img) override;

private:
    KookaPrint *mPdfPrinter;
};


MultipagePdfWriter::MultipagePdfWriter()
    : AbstractMultipageWriter()
{
    qCDebug(DESTINATION_LOG);
    mPdfPrinter = new KookaPrint();
}


MultipagePdfWriter::~MultipagePdfWriter()
{
    delete mPdfPrinter;
}


void MultipagePdfWriter::setPageSize(const QPageSize &pageSize)
{
    mPdfPrinter->setPageSize(pageSize);
}


void MultipagePdfWriter::setBaseImage(const QImage *img)
{
    mPdfPrinter->setBaseImage(img);
}

void MultipagePdfWriter::setOutputFile(const QString &fileName)
{
    mPdfPrinter->setPdfMode(fileName);
}


bool MultipagePdfWriter::startPrint()
{
    mPdfPrinter->startPrint();
    return (true);
}


bool MultipagePdfWriter::printImage(const QImage *img)
{
    mPdfPrinter->printImage(img);
    return (true);
}


void MultipagePdfWriter::endPrint()
{
    mPdfPrinter->endPrint();
}

//////////////////////////////////////////////////////////////////////////
//									//
//  MultipageTiffWriter							//
//									//
//////////////////////////////////////////////////////////////////////////

#ifdef HAVE_TIFF

class MultipageTiffWriter : public AbstractMultipageWriter
{
public:
    MultipageTiffWriter();
    ~MultipageTiffWriter() = default;

    void setPageSize(const QPageSize &pageSize) override;
    void setBaseImage(const QImage *img) override;
    void setOutputFile(const QString &fileName) override;
    bool startPrint() override;
    void endPrint() override;

protected:
    bool printImage(const QImage *img) override;

private:
    QSize mBaseSize;
    int mBaseResX;
    int mBaseResY;

    QString mFileName;
    QString mTiffcpCommand;
};


#ifndef TIFFCP_COMMAND
#define TIFFCP_COMMAND		"tiffcp"
#endif


MultipageTiffWriter::MultipageTiffWriter()
    : AbstractMultipageWriter()
{
    qCDebug(DESTINATION_LOG);
}


void MultipageTiffWriter::setPageSize(const QPageSize &pageSize)
{
    // Ignored, size taken from each scanned image
}


void MultipageTiffWriter::setBaseImage(const QImage *img)
{
    if (img==nullptr) mBaseSize = QSize();		// unset the reference image
    else
    {
        mBaseSize = img->size();
        mBaseResX = DPM_TO_DPI(img->dotsPerMeterX());
        mBaseResY = DPM_TO_DPI(img->dotsPerMeterY());
        qCDebug(DESTINATION_LOG) << "size (pix)" << mBaseSize << "dpi X" << mBaseResX << "dpi Y" << mBaseResY;
    }
}


void MultipageTiffWriter::setOutputFile(const QString &fileName)
{
    mFileName = fileName;
}


bool MultipageTiffWriter::startPrint()
{
    if (mBaseSize.isNull()) return (false);		// no reference image

    if (mTiffcpCommand.isEmpty())			// command not yet found
    {
        mTiffcpCommand = QStandardPaths::findExecutable(TIFFCP_COMMAND);
        if (mTiffcpCommand.isEmpty())			// that didn't find the command
        {
            KMessageBox::error(nullptr, xi18nc("@info",
                                               "Cannot locate the <command section=\"1\">tiffcp</command> command,<nl/>"
                                               "ensure that it is available on <envvar>PATH</envvar>.<nl/><nl/>"
                                               "The command is part of the <application>libtiff</application> package,<nl/>"
                                               "but it may need to be installed separately."),
                               i18n("Cannot find TIFF command"));

            return (false);				// user can try again next time
        }

        qCDebug(DESTINATION_LOG) << "found command" << mTiffcpCommand;
    }

    // The 'mFileName' will be a QTemporaryFile which has been open()'ed
    // and then immediately close()'d by the caller.  So we can assume that
    // the file already exists, is writeable, and is currently zero size.
    // tiffcp(1) handles appending to an empty file correctly, so there is
    // nothing else to do to that file here.
    return (true);
}


bool MultipageTiffWriter::printImage(const QImage *img)
{
    qCDebug(DESTINATION_LOG) << "format" << img->format();

    // The QTemporaryFile created here is the current scanned page.
    // It is immediately appended to the 'mFileName' output file and
    // then discarded, so there is no need to keep the temporary file
    // any longer or to leave autoRemove() disabled.
    const QString ext = mFileName.mid(mFileName.lastIndexOf('.'));
    const QString dir = mFileName.left(mFileName.lastIndexOf('/'));
    QTemporaryFile tempPage(dir+"/pageXXXXXX"+ext);	// reuse the same template
    if (!tempPage.open())
    {
fail:   KMessageBox::error(nullptr,
                           xi18nc("@info", "Cannot save page to file <filename>%1</filename><nl/><message>%2</message>",
                                  tempPage.fileName(), tempPage.errorString()),
                           i18n("Cannot Save File"));
        return (false);
    }

    tempPage.close();					// only want the file name

    qDebug() << "saving TIFF to" << tempPage.fileName();
    if (!img->save(tempPage.fileName(), "TIFF")) goto fail;

    // No image format or conversion options are given to tiffcp(1),
    // so each page of the resulting file will have the same format as
    // the corresponding scanned page.
    const QStringList args = { "-a", tempPage.fileName(), mFileName };
    qDebug() << "executing command" << mTiffcpCommand << "args" << args;
    int s = QProcess::execute(mTiffcpCommand, args);
    if (s!=0)						// problem with tiffcp(1) command
    {
        KMessageBox::error(nullptr,
                           xi18nc("@info", "Cannot append page to file <filename>%1</filename>, status %2<nl/>(see standard error for any message)",
                                  mFileName, QString::number(s)),
                            i18n("Cannot Append Page"));
        return (false);
    }

    return (true);
}


void MultipageTiffWriter::endPrint()
{
}

#endif							// HAVE_TIFF

//////////////////////////////////////////////////////////////////////////
//									//
//  DestinationMultipage						//
//									//
//////////////////////////////////////////////////////////////////////////

DestinationMultipage::DestinationMultipage(QObject *pnt, const QVariantList &args)
    : AbstractDestination(pnt, "DestinationMultipage")
{
    mSaveFile = nullptr;
    mWriter = nullptr;

    mPageSize = QSizeF(KookaSettings::destinationMultipageSizeWidth(), KookaSettings::destinationMultipageSizeHeight());
}


DestinationMultipage::~DestinationMultipage()
{
    if (mSaveFile!=nullptr) delete mSaveFile;
    if (mWriter!=nullptr) delete mWriter;
}


void DestinationMultipage::batchStart(const MultiScanOptions *opts)
{
    AbstractDestination::batchStart(opts);

    const QString mimeName = mFormatCombo->currentData().toString();
    const QMimeDatabase db;
    const QMimeType mimeType = db.mimeTypeForName(mimeName);
    if (!mimeType.isValid())
    {
        qCWarning(DESTINATION_LOG) << "unknown format" << mimeName;
        return;
    }

    mSaveFile = nullptr;				// save file not ready yet
    RecentSaver saver("saveMulti");
    mSaveUrl = QFileDialog::getSaveFileUrl(parentWidget(), i18n("Save Multipage"),
                                           saver.recentUrl(mSaveUrl.fileName()), mimeType.filterString());

    // If the user cancels the file dialogue we cannot do anything
    // to cancel the batch at this stage, but mSaveFile being NULL
    // will be checked when the first image is about to be scanned in
    // scanStarting() above.
    if (!mSaveUrl.isValid()) return;
    saver.save(mSaveUrl);

    // Using a temporary file regardless of whether the destination is
    // local or remote.  This is so that both cases can be treated the
    // same until the PDF generation is finished, when it can be moved
    // to the intended local or remote destination.
    mSaveFile = new QTemporaryFile(QDir::tempPath()+'/'+QCoreApplication::applicationName()+"saveXXXXXX."+mimeType.preferredSuffix());
    if (!mSaveFile->open())
    {
        KMessageBox::error(parentWidget(),
                           xi18nc("@info", "Cannot save to file <filename>%1</filename><nl/><message>%2</message>",
                                  mSaveUrl.toDisplayString(), mSaveFile->errorString()),
                            i18n("Cannot Save File"));
        delete mSaveFile; mSaveFile = nullptr;
        return;
    }

    mSaveFile->close();					// only want the file name
    mSaveMime = mimeType.name();
    qCDebug(DESTINATION_LOG) << "save" << mSaveMime << "to temp" << mSaveFile->fileName();
}


bool DestinationMultipage::scanStarting(ScanImage::ImageType type)
{
    return (mSaveFile!=nullptr);			// stop batch if cancelled above
}


bool DestinationMultipage::imageScanned(ScanImage::Ptr img)
{
    qCDebug(DESTINATION_LOG) << "received image size" << img->size();

    if (mWriter==nullptr)
    {
        // Create the appropriate writer for the selected format.
        // Delayed until now so that the image size and resolution,
        // which are assumed not to change during a scan batch, are
        // available from the first scanned image.
        if (mSaveMime=="application/pdf")
        {
            mWriter = new MultipagePdfWriter();
        }
#ifdef HAVE_TIFF
        else if (mSaveMime=="image/tiff")
        {
            mWriter = new MultipageTiffWriter();
        }
#endif
        else
        {
            qCWarning(DESTINATION_LOG) << "Unknown MIME type" << mSaveMime;
            return (false);
        }

        // Because the list of paper sizes supported by QPageSize and those
        // provided by libpaper do not correspond in any way, the PDF page
        // size is always set as a custom size.
        const QPageSize pageSize(mPageSize, QPageSize::Millimeter, QString(), QPageSize::ExactMatch);
        mWriter->setPageSize(pageSize);
        // This must happen after a setPageLayout() or setPageSize()
        // for a QPrinter.
        mWriter->setOutputFile(mSaveFile->fileName());

        // Even if rotation is in use, this uses the size of the first
        // image as the reference.  Therefore the only sensible rotation
        // combinations are no rotation, either or both rotated 180, or
        // both rotated either 90 or 270.
        mWriter->setBaseImage(img.data());
        bool ok = mWriter->startPrint();
        if (!ok) return (false);
    }

    mWriter->outputImage(img.data());
    return (true);
}


void DestinationMultipage::batchEnd(bool ok)
{
    // Need to check all pointers here because, if the scan failed to
    // start or was cancelled by the user in batchStart(), either or
    // both of mSaveFile and mWriter could be NULL.
    if (mWriter!=nullptr)
    {
        mWriter->endPrint();
        delete mWriter; mWriter = nullptr;	// flush the final print data
    }

    if (ok && mSaveFile!=nullptr)
    {
        // For simplicity, use KIO to move the temporary file to the destination
        // regardless of whether it is local or remote.  Whether the destination
        // already exists will have been checked and confirmed by the QFileDialog
        // back at the start of the scan job, so set the Overwrite flag to ensure
        // that it does get overwritten without confirmation.
        KIO::FileCopyJob *job = KIO::file_move(QUrl::fromLocalFile(mSaveFile->fileName()), mSaveUrl, -1, KIO::Overwrite);
        job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, parentWidget()));
        job->exec();
    }

    if (mSaveFile!=nullptr) mSaveFile->setAutoRemove(true);
    delete mSaveFile; mSaveFile = nullptr;
}


void DestinationMultipage::createGUI(ScanParamsPage *page)
{
    // The MIME types that can be selected for the output format.
    QStringList mimeTypes;
    mimeTypes << "application/pdf";
#ifdef HAVE_TIFF
    mimeTypes << "image/tiff";
#endif
    mFormatCombo = createFormatCombo(mimeTypes, KookaSettings::destinationMultipageMime(), false);
    connect(mFormatCombo, &QComboBox::currentIndexChanged, this, &DestinationMultipage::slotFormatChanged);
    page->addRow(i18n("Output format:"), mFormatCombo);

    mPageSetupButton = new QPushButton(i18n("Page Setup..."), page);
    mPageSetupButton->setIcon(QIcon::fromTheme("document-print"));
    mPageSetupButton->setToolTip(i18nc("@info:tootip", "Set page size and layout"));
    connect(mPageSetupButton, &QAbstractButton::clicked, this, &DestinationMultipage::slotPageSetup);
    page->addRow(nullptr, mPageSetupButton, nullptr, Qt::AlignRight);

    slotFormatChanged();
}


KLocalizedString DestinationMultipage::scanDestinationString()
{
    // The mWriter will not have been created until the scan of the
    // first page has finished.
    const int p = (mWriter==nullptr) ? 1 : mWriter->totalPages()+1;

    // KookaPrint::pageCount() starts at 0 and is updated after the
    // image has been received and printed.  Therefore adding 1 here
    // for the user-visible count.
    return (kxi18n("Saving page %1 to <filename>%2</filename>").subs(QString::number(p)).subs(mSaveUrl.toDisplayString()));
}


void DestinationMultipage::saveSettings() const
{
    if (!mSaveMime.isEmpty()) KookaSettings::setDestinationMultipageMime(mSaveMime);
    KookaSettings::setDestinationMultipageSizeWidth(mPageSize.width());
    KookaSettings::setDestinationMultipageSizeHeight(mPageSize.height());
}


MultiScanOptions::Capabilities DestinationMultipage::capabilities() const
{
    return (MultiScanOptions::AcceptBatch|MultiScanOptions::AlwaysBatch);
}


void DestinationMultipage::slotPageSetup()
{
    MultipageOptionsDialog d(mPageSize, parentWidget());
    if (!d.exec()) return;

    mPageSize = d.pageSize();
    qCDebug(DESTINATION_LOG) << "new size" << mPageSize;
}


void DestinationMultipage::slotFormatChanged()
{
    const QString fmt = mFormatCombo->currentData().toString();
    mPageSetupButton->setEnabled(fmt=="application/pdf");
}
