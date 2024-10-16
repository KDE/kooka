/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2020      Jonathan Marten <jjm@keelhaul.me.uk>	*
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

#include "destinationprint.h"

#include <qcheckbox.h>
#include <qprintdialog.h>

#include <kpluginfactory.h>
#include <klocalizedstring.h>
#include <kmessagebox.h>

#include "scanparamspage.h"
#include "kookaprint.h"
#include "imgprintdialog.h"
#include "destination_logging.h"


K_PLUGIN_FACTORY_WITH_JSON(DestinationPrintFactory, "kookadestination-print.json", registerPlugin<DestinationPrint>();)
#include "destinationprint.moc"


DestinationPrint::DestinationPrint(QObject *pnt, const QVariantList &args)
    : AbstractDestination(pnt, "DestinationPrint")
{
    mImmediateCheck = nullptr;
    mPrinter = nullptr;
}


DestinationPrint::~DestinationPrint()
{
    delete mPrinter;
}


void DestinationPrint::batchStart(const MultiScanOptions *opts)
{
    AbstractDestination::batchStart(opts);
    mBatchFirst = true;

    if (mPrinter==nullptr)				// create KookaPrint if needed
    {
        mPrinter = new KookaPrint;
        qCDebug(DESTINATION_LOG) << "printing to" << mPrinter->printerName();
        mPrinter->setCopyMode(true);
    }
}


bool DestinationPrint::imageScanned(ScanImage::Ptr img)
{
    qCDebug(DESTINATION_LOG) << "received image size" << img->size();

    if (mBatchFirst)					// first image in a batch
    {
        mPrinter->setBaseImage(img.data());		// use as reference image

        if (!mImmediateCheck->isChecked())		// want the print dialogue
        {
            QPrintDialog d(mPrinter, parentWidget());
            d.setWindowTitle(i18nc("@title:window", "Print Image"));
            d.setOptions(QAbstractPrintDialog::PrintToFile|QAbstractPrintDialog::PrintShowPageSize);

            ImgPrintDialog imgTab(img, mPrinter);		// create tab for our options
            d.setOptionTabs(QList<QWidget *>() << &imgTab);	// add tab to print dialogue

            if (!d.exec()) return (false);			// open the dialogue
            QString msg = imgTab.checkValid();			// check that settings are valid
            if (!msg.isEmpty())					// if not, display error message
            {
                KMessageBox::error(parentWidget(),
                                   i18nc("@info", "Invalid print options were specified:\n\n%1", msg),
                                   i18nc("@title:window", "Cannot Print"));
                return (false);
            }

            imgTab.updatePrintParameters();		// set final printer options
        }

        mPrinter->startPrint();				// start the print job
        mBatchFirst = false;				// note now subsequent in batch
    }

    mPrinter->printImage(img.data());			// print the image

    mImmediateCheck->setEnabled(true);			// now can skip dialogue next time
    return (true);
}


void DestinationPrint::batchEnd(bool ok)
{
    if (!ok) mPrinter->abort();				// try to cancel the job
    mPrinter->endPrint();				// clean up anyway
}


void DestinationPrint::createGUI(ScanParamsPage *page)
{
    mImmediateCheck = new QCheckBox(i18n("Print immediately with same settings"));
    // The option is turned off and not available until
    // one scan/print cycle has been done.
    mImmediateCheck->setEnabled(false);
    page->addRow(nullptr, mImmediateCheck);
}


KLocalizedString DestinationPrint::scanDestinationString()
{
    return (ki18n("Printing image"));
}


void DestinationPrint::saveSettings() const
{
    // TODO: save settings from mPrinter
}


MultiScanOptions::Capabilities DestinationPrint::capabilities() const
{
    return (MultiScanOptions::AcceptBatch|MultiScanOptions::AlwaysBatch);
}
