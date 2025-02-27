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
#include <qgroupbox.h>

#include <qformlayout.h>
#include <qboxlayout.h>
#include <qspinbox.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>

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
#include "destination_logging.h"


K_PLUGIN_FACTORY_WITH_JSON(DestinationMultipageFactory, "kookadestination-multipage.json", registerPlugin<DestinationMultipage>();)
#include "destinationmultipage.moc"

//////////////////////////////////////////////////////////////////////////
//									//
//  MultipageOptionsDialog - A dialogue to replace QPageSetupDialog	//
//  containing only the options that are needed and allowing for the	//
//  possibility of more that may be needed later.			//
//									//
//////////////////////////////////////////////////////////////////////////

MultipageOptionsDialog::MultipageOptionsDialog(const QSizeF &pageSize, QWidget *pnt)
    : DialogBase(pnt)
{
    setObjectName("MultipageOptionsDialog");

    qCDebug(DESTINATION_LOG) << "page size" << pageSize;

    setButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    setWindowTitle(i18n("PDF Page Setup"));
    setModal(true);

    QWidget *w = new QWidget(this);
    setMainWidget(w);
    QFormLayout *fl = new QFormLayout(w);

    // Row 0: Page size combo box
    mPageSizeCombo = new QComboBox(w);
    mPageSizeCombo->setSizePolicy(QSizePolicy::MinimumExpanding, mPageSizeCombo->sizePolicy().verticalPolicy());

    int sizeIndex = -1;
    Qt::Orientation orient;

    const PaperSize *sizes = PaperSizes::self()->papers();
    for (int i = 0; sizes[i].name!=nullptr; ++i)
    {
        const double sw = sizes[i].width;
        const double sh = sizes[i].height;
        mPageSizeCombo->addItem(sizes[i].name, QSizeF(sw, sh));
        if (!pageSize.isValid()) continue;

        if (qFuzzyCompare(sw, pageSize.width()) && qFuzzyCompare(sh, pageSize.height()))
        {						// portrait orientation matches
            sizeIndex = i;
            orient = Qt::Vertical;
            qDebug() << "found portrait size" << sizes[i].name;
        }
        else if (qFuzzyCompare(sh, pageSize.width()) && qFuzzyCompare(sw, pageSize.height()))
        {						// landscape orientation matches
            sizeIndex = i;
            orient = Qt::Horizontal;
            qDebug() << "found landscape size" << sizes[i].name;
        }
    }
    mPageSizeCombo->addItem(i18n("Custom..."));

    fl->addRow(i18n("Page size:"), mPageSizeCombo);

    // Row 1: Page size display and custom entry
    QHBoxLayout *hb = new QHBoxLayout;
    hb->setContentsMargins(0, 0, 0, 0);

    mCustomWidthSpinbox = new QDoubleSpinBox(w);
    mCustomWidthSpinbox->setDecimals(1);
    mCustomWidthSpinbox->setRange(1.0, 2000.0);
    mCustomWidthSpinbox->setStepType(QAbstractSpinBox::AdaptiveDecimalStepType);
    mCustomWidthSpinbox->setSuffix(i18nc("unit suffix for millimetres", " mm"));
    mCustomWidthSpinbox->setEnabled(false);
    hb->addWidget(mCustomWidthSpinbox);

    QLabel *l = new QLabel(i18nc("centered 'x' separator for height/width", "\303\227"), w);
    hb->addWidget(l);

    mCustomHeightSpinbox = new QDoubleSpinBox(w);
    mCustomHeightSpinbox->setDecimals(1);
    mCustomHeightSpinbox->setRange(1.0, 2000.0);
    mCustomHeightSpinbox->setStepType(QAbstractSpinBox::AdaptiveDecimalStepType);
    mCustomHeightSpinbox->setSuffix(i18nc("unit suffix for millimetres", " mm"));
    mCustomHeightSpinbox->setEnabled(false);
    hb->addWidget(mCustomHeightSpinbox);

    hb->addStretch();
    fl->addRow("", hb);

    // Row 2: Spacing
    fl->setItem(fl->rowCount(), QFormLayout::SpanningRole, DialogBase::verticalSpacerItem());

    // Row 3: Portrait/Landscape radio buttons
    QButtonGroup *bg = new QButtonGroup(w);

    hb = new QHBoxLayout;
    hb->setContentsMargins(0, 0, 0, 0);

    mPortraitRadio = new QRadioButton(i18nc("@option:radio paper orientation", "Portrait"), w);
    bg->addButton(mPortraitRadio);
    hb->addWidget(mPortraitRadio);

    mLandscapeRadio = new QRadioButton(i18nc("@option:radio paper orientation", "Landscape"), w);
    bg->addButton(mLandscapeRadio);
    hb->addWidget(mLandscapeRadio);

    hb->addStretch();
    fl->addRow(i18n("Orientation:"), hb);

    // Set initial GUI values from the provided page size, if a match
    // was found above.
    if (sizeIndex!=-1)					// a preset size matched
    {
        mPageSizeCombo->setCurrentIndex(sizeIndex);
        mPortraitRadio->setChecked(orient==Qt::Vertical);
        mLandscapeRadio->setChecked(orient==Qt::Horizontal);
    }
    else if (pageSize.isValid())			// provided but not known
    {
        mPageSizeCombo->setCurrentIndex(mPageSizeCombo->count()-1);
        mCustomWidthSpinbox->setValue(pageSize.width());
        mCustomHeightSpinbox->setValue(pageSize.height());
    }
    else mPageSizeCombo->setCurrentIndex(0);		// default first in list

    slotValueChanged();
    slotSettingChanged();

    // Connect all signals now, after the settings have been adjusted
    connect(mPageSizeCombo, &QComboBox::currentIndexChanged, this, &MultipageOptionsDialog::slotSettingChanged);
    connect(bg, &QButtonGroup::buttonClicked, this, &MultipageOptionsDialog::slotSettingChanged);
    connect(mCustomWidthSpinbox, &QDoubleSpinBox::valueChanged, this, &MultipageOptionsDialog::slotValueChanged);
    connect(mCustomHeightSpinbox, &QDoubleSpinBox::valueChanged, this, &MultipageOptionsDialog::slotValueChanged);
}


void MultipageOptionsDialog::slotSettingChanged()
{
    const QSize pageSize = mPageSizeCombo->currentData().value<QSize>();
    const Qt::Orientation orient = (mLandscapeRadio->isChecked() ? Qt::Horizontal : Qt::Vertical);
    qDebug() << "selected size" << pageSize << "orient" << orient;

    QSignalBlocker block1(mCustomWidthSpinbox);		// don't want a signal from setValue()
    QSignalBlocker block2(mCustomHeightSpinbox);

    if (pageSize.isValid())				// preset paper size
    {
        if (orient==Qt::Vertical)			// portrait orientation
        {
            mCustomWidthSpinbox->setValue(pageSize.width());
            mCustomHeightSpinbox->setValue(pageSize.height());
        }
        else						// landscape orientation
        {
            mCustomWidthSpinbox->setValue(pageSize.height());
            mCustomHeightSpinbox->setValue(pageSize.width());
        }

        mCustomWidthSpinbox->setEnabled(false);
        mCustomHeightSpinbox->setEnabled(false);
    }
    else						// custom paper size
    {
        const double vw = mCustomWidthSpinbox->value();
        const double vh = mCustomHeightSpinbox->value();

        if (orient==Qt::Vertical)			// portrait orientation
        {
            mCustomWidthSpinbox->setValue(qMin(vw, vh));
            mCustomHeightSpinbox->setValue(qMax(vw, vh));
        }
        else						// landscape orientation
        {
            mCustomWidthSpinbox->setValue(qMax(vw, vh));
            mCustomHeightSpinbox->setValue(qMin(vw, vh));
        }

        mCustomWidthSpinbox->setEnabled(true);
        mCustomHeightSpinbox->setEnabled(true);
    }
}


void MultipageOptionsDialog::slotValueChanged()
{
    if (!mCustomWidthSpinbox->isEnabled())		// not setting a custom size
    {							// and not needing initialsation
        if (mPortraitRadio->isChecked() || mLandscapeRadio->isChecked()) return;
    }

    const double vw = mCustomWidthSpinbox->value();
    const double vh = mCustomHeightSpinbox->value();
    if (vw>vh) mLandscapeRadio->setChecked(true);	// set to landscape orientation
    else mPortraitRadio->setChecked(true);		// set to portrait orientation
}


QSizeF MultipageOptionsDialog::pageSize() const
{
    const QSizeF pageSize = mPageSizeCombo->currentData().value<QSizeF>();
    const Qt::Orientation orient = (mLandscapeRadio->isChecked() ? Qt::Horizontal : Qt::Vertical);
    qDebug() << "selected size" << pageSize << "orient" << orient;

    if (pageSize.isValid())				// preset paper size
    {
        return (orient==Qt::Vertical ? pageSize : pageSize.transposed());
    }
    else						// custom paper size,
    {							// directly from spin boxes
        return (QSizeF(mCustomWidthSpinbox->value(), mCustomHeightSpinbox->value()));
    }
}

//////////////////////////////////////////////////////////////////////////
//									//
//  DestinationMultipage						//
//									//
//////////////////////////////////////////////////////////////////////////

DestinationMultipage::DestinationMultipage(QObject *pnt, const QVariantList &args)
    : AbstractDestination(pnt, "DestinationMultipage")
{
    mSaveFile = nullptr;
    mPdfPrinter = nullptr;

    mPageSize = QSizeF(KookaSettings::destinationMultipageSizeWidth(), KookaSettings::destinationMultipageSizeHeight());
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

        // Because the list of paper sizes supported by QPageSize and those
        // provided by libpaper do not correspond in any way, the PDF page
        // size is always set as a custom size.
        const QPageSize pageSize(mPageSize, QPageSize::Millimeter, QString(), QPageSize::ExactMatch);
        mPdfPrinter->setPageSize(pageSize);
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
    // The mPdfPrinter will not have been created until the scan of the
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
    qDebug() << "new size" << mPageSize;
}


void DestinationMultipage::slotFormatChanged()
{
    const QString fmt = mFormatCombo->currentData().toString();
    mPageSetupButton->setEnabled(fmt=="application/pdf");
}
