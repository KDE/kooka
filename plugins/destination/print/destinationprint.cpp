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


bool DestinationPrint::scanStarting(ScanImage::ImageType type)
{
    return (true);					// no preparation, can always start
}


void DestinationPrint::imageScanned(ScanImage::Ptr img)
{
    qCDebug(DESTINATION_LOG) << "received image size" << img->size();

    if (mPrinter==nullptr)				// create KookaPrint if needed
    {
        mPrinter = new KookaPrint;
        mPrinter->setCopyMode(true);
    }
    mPrinter->setImage(img.data());			// pass image to printer

    if (!mImmediateCheck->isChecked())			// want the print dialogue
    {
        // TODO: need access to a widget parent from plugin
        QPrintDialog d(mPrinter, nullptr);
        d.setWindowTitle(i18nc("@title:window", "Print Image"));
        d.setOptions(QAbstractPrintDialog::PrintToFile|QAbstractPrintDialog::PrintShowPageSize);

        ImgPrintDialog imgTab(img, mPrinter);		// create tab for our options
        d.setOptionTabs(QList<QWidget *>() << &imgTab);	// add tab to print dialogue

        if (!d.exec()) return;				// open the dialogue
        QString msg = imgTab.checkValid();		// check that settings are valid
        if (!msg.isEmpty())				// if not, display error message
        {
        // TODO: need access to a widget parent
            KMessageBox::sorry(nullptr,
                               i18nc("@info", "Invalid print options were specified:\n\n%1", msg),
                               i18nc("@title:window", "Cannot Print"));
            return;
        }

        imgTab.updatePrintParameters();			// set final printer options
    }

    qDebug() << "printing to" << mPrinter->printerName();
    mPrinter->printImage();				// print the image

    mImmediateCheck->setEnabled(true);			// now can skip dialogue next time
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
