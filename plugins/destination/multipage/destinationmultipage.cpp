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

#include "scanparamspage.h"
#include "recentsaver.h"
#include "kookaprint.h"
#include "destination_logging.h"


K_PLUGIN_FACTORY_WITH_JSON(DestinationMultipageFactory, "kookadestination-multipage.json", registerPlugin<DestinationMultipage>();)
#include "destinationmultipage.moc"


DestinationMultipage::DestinationMultipage(QObject *pnt, const QVariantList &args)
    : AbstractDestination(pnt, "DestinationMultipage")
{
    mSaveFile = nullptr;
    mPdfPrinter = nullptr;
}


DestinationMultipage::~DestinationMultipage()
{
    if (mSaveFile!=nullptr) delete mSaveFile;
    if (mPdfPrinter!=nullptr) delete mPdfPrinter;
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
    qCDebug(DESTINATION_LOG) << "save to temp" << mSaveFile->fileName();
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
        mPdfPrinter->setPdfMode(mSaveFile->fileName());
        mPdfPrinter->setBaseImage(img.data());
        mPdfPrinter->startPrint();
    }

    mPdfPrinter->printImage(img.data());
    return (true);
}


void DestinationMultipage::batchEnd(bool ok)
{
    mPdfPrinter->endPrint();

    if (ok)
    {
        // TODO: set file modes according to umask
        // TODO: move temp file to destination
        // TODO: allow remote files
    }

    delete mPdfPrinter; mPdfPrinter = nullptr;
    delete mSaveFile; mSaveFile = nullptr;
}


void DestinationMultipage::createGUI(ScanParamsPage *page)
{
    // The MIME types that can be selected for the output format.
    QStringList mimeTypes;
    mimeTypes << "application/pdf";
    // TODO: also TIFF
    //mimeTypes << "application/pdf" << "image/tiff";
    mFormatCombo = createFormatCombo(mimeTypes,
                                     // TODO: save setting
                                     "", //, KookaSettings::destinationMultipageMime()
                                     false);
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
    // TODO: destination file name and page count
    return (ki18n("Multipage Image"));
}


void DestinationMultipage::saveSettings() const
{
    // TODO: may be needed
}


MultiScanOptions::Capabilities DestinationMultipage::capabilities() const
{
    return (MultiScanOptions::AcceptBatch|MultiScanOptions::AlwaysBatch);
}


void DestinationMultipage::slotPageSetup()
{
    // TODO: QPageSetupDialog
}


void DestinationMultipage::slotFormatChanged()
{
    const QString fmt = mFormatCombo->currentData().toString();
    mPageSetupButton->setEnabled(fmt=="application/pdf");
}
