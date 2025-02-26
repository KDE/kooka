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
#include <qpagesetupdialog.h>
#include <qgroupbox.h>

#include <kmessagebox.h>
#include <kpluginfactory.h>
#include <klocalizedstring.h>

#include <kio/filecopyjob.h>
#include <kio/jobuidelegatefactory.h>

#include "scanparamspage.h"
#include "recentsaver.h"
#include "kookaprint.h"
#include "kookasettings.h"
#include "destination_logging.h"


K_PLUGIN_FACTORY_WITH_JSON(DestinationMultipageFactory, "kookadestination-multipage.json", registerPlugin<DestinationMultipage>();)
#include "destinationmultipage.moc"


DestinationMultipage::DestinationMultipage(QObject *pnt, const QVariantList &args)
    : AbstractDestination(pnt, "DestinationMultipage")
{
    mSaveFile = nullptr;
    mPdfPrinter = nullptr;

    mReferencePrinter = new QPrinter;
    // TODO: load settings into that
}


DestinationMultipage::~DestinationMultipage()
{
    if (mSaveFile!=nullptr) delete mSaveFile;
    if (mPdfPrinter!=nullptr) delete mPdfPrinter;
    delete mReferencePrinter;
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
    mSaveFile = new QTemporaryFile(QDir::tempPath()+'/'+QCoreApplication::applicationName()+"XXXXXX."+mimeType.preferredSuffix());
    mSaveFile->setAutoRemove(false);
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

    if (mPdfPrinter==nullptr)
    {
        // Create the PDF writer.  Delayed until now so that the image
        // size and resolution, which are assumed not to change during a
        // scan batch, are available from the first scanned image.
        //
        // Even if rotation is in use, this uses the size of the first
        // image as the reference.  Therefore the only sensible rotation
        // combinations are no rotation, either or both rotated 180, or
        // both rotated either 90 or 270.
        mPdfPrinter = new KookaPrint();
        mPdfPrinter->setPageLayout(mReferencePrinter->pageLayout());
        mPdfPrinter->setBaseImage(img.data());
        mPdfPrinter->setPdfMode(mSaveFile->fileName());	// must be after setPageLayout()
        mPdfPrinter->startPrint();
    }

    mPdfPrinter->printImage(img.data());
    return (true);
}


void DestinationMultipage::batchEnd(bool ok)
{
    // Need to check all pointers here because, if the scan failed to
    // start or was cancelled by the user in batchStart(), either or
    // both of mSaveFile and mPdfPrinter could be NULL.
    if (mPdfPrinter!=nullptr)
    {
        mPdfPrinter->endPrint();
        delete mPdfPrinter; mPdfPrinter = nullptr;	// flush the final print data

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
    }

    if (mSaveFile!=nullptr) mSaveFile->setAutoRemove(true);
    delete mSaveFile; mSaveFile = nullptr;
}


void DestinationMultipage::createGUI(ScanParamsPage *page)
{
    // The MIME types that can be selected for the output format.
    QStringList mimeTypes;
    mimeTypes << "application/pdf";
    // TODO: also TIFF
    //mimeTypes << "application/pdf" << "image/tiff";
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
    // The mPdfPrinter will not be available until the scan of the
    // first page has finished.
    const int p = (mPdfPrinter==nullptr) ? 1 : mPdfPrinter->totalPages()+1;

    // KookaPrint::pageCount() starts at 0 and is updated after the
    // image has been received and printed.  Therefore adding 1 here
    // for the user-visible count.
    return (kxi18n("Saving page %1 to <filename>%2</filename>").subs(QString::number(p)).subs(mSaveUrl.toDisplayString()));
}


void DestinationMultipage::saveSettings() const
{
    if (!mSaveMime.isEmpty()) KookaSettings::setDestinationMultipageMime(mSaveMime);
    // TODO: page settings from mReferencePrinter
}


MultiScanOptions::Capabilities DestinationMultipage::capabilities() const
{
    return (MultiScanOptions::AcceptBatch|MultiScanOptions::AlwaysBatch);
}


void DestinationMultipage::slotPageSetup()
{
    qDebug() << "orig rect" << mReferencePrinter->pageRect(QPrinter::Millimeter);

    // TODO: we only really want half of the options in this dialogue,
    // it is not possible to set it to the currently configured paper
    // size, and there may be a need for other PDF generation options
    // such as "fit to page".  Implement our own dialogue (with paper
    // sizes from libpaper) instead?

    QPageSetupDialog d(mReferencePrinter, parentWidget());

    // Unfortunately the QPageSetupDialog does not take account of
    // the QPrinter's configured page size, but always selects the
    // default platform page size - in Unix taken from CUPS - when
    // the dialogue is opened.  See QPageSetupWidget::initPageSizes()
    // in qtbase/src/printsupport/dialogs/qpagesetupdialog_unix.cpp
    //
    // TODO: may be able to work around by finding the combo box
    // and examining its item data, which is a QVariant containg a
    // QPageSize.

    // The dialogue "Page Layout" settings are not applicable here.
    // Setting the group box to disabled would be easier and give a
    // better visual effect, but QPageSetupDialog reenables that
    // group box whenever the paper size is changed.  So find and
    // disable the two combo boxes within it.
    QGroupBox *layoutGroup = d.findChild<QGroupBox *>("pagesPerSheetButtonGroup");
    if (layoutGroup!=nullptr)
    {
        QList<QComboBox *> combos = layoutGroup->findChildren<QComboBox *>();
        for (QComboBox *combo : std::as_const(combos)) combo->setEnabled(false);
    }

    if (!d.exec()) return;

    QPrinter *printer = d.printer();
    qDebug() << "new rect" << printer->pageRect(QPrinter::Millimeter);
}


void DestinationMultipage::slotFormatChanged()
{
    const QString fmt = mFormatCombo->currentData().toString();
    mPageSetupButton->setEnabled(fmt=="application/pdf");
}
