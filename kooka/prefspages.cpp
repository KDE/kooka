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

#include "prefspages.h"

#include <qlayout.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qfileinfo.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qgroupbox.h>
#include <qdebug.h>

#include <klocalizedstring.h>
#include <kcombobox.h>
#include <kmessagebox.h>
#include <kurlrequester.h>
#include <kseparator.h>
#include <kconfigskeleton.h>
#include <kimageio.h>
#include <kfiledialog.h>

#include "scansettings.h"

#include "kookapref.h"
#include "kookagallery.h"
#include "formatdialog.h"
#include "ocrgocrengine.h"
#include "ocrocradengine.h"
#include "ocrkadmosengine.h"
#include "kookasettings.h"


//  Abstract base

KookaPrefsPage::KookaPrefsPage(KPageDialog *parent)
    : QWidget(parent)
{
    mLayout = new QVBoxLayout(this);
}

KookaPrefsPage::~KookaPrefsPage()
{
}

//  "General" page

KookaGeneralPage::KookaGeneralPage(KPageDialog *parent)
    : KookaPrefsPage(parent)
{
    mLayout->addStretch(9);             // push down to bottom

    QGroupBox *gb = new QGroupBox(i18n("Hidden Messages"), this);
    QGridLayout *gl = new QGridLayout(gb);

    /* Enable messages and questions */
    QLabel *lab = new QLabel(i18n("Use this button to reenable all messages and questions which\nhave been hidden by using \"Don't ask me again\"."), gb);
    gl->addWidget(lab, 0, 0);

    mEnableMessagesButton = new QPushButton(i18n("Enable Messages/Questions"), gb);
    connect(mEnableMessagesButton, &QPushButton::clicked, this, &KookaGeneralPage::slotEnableWarnings);
    gl->addWidget(mEnableMessagesButton, 1, 0, Qt::AlignRight);

    mLayout->addWidget(gb);
}

void KookaGeneralPage::saveSettings()
{
}

void KookaGeneralPage::defaultSettings()
{
}

void KookaGeneralPage::slotEnableWarnings()
{
    //qDebug();

    KMessageBox::enableAllMessages();
    FormatDialog::forgetRemembered();
    KSharedConfig::openConfig()->reparseConfiguration();

    mEnableMessagesButton->setEnabled(false);           // show this has been done
}

//  "Startup" page

KookaStartupPage::KookaStartupPage(KPageDialog *parent)
    : KookaPrefsPage(parent)
{
    /* Query for network scanner (Checkbox) */
    const KConfigSkeletonItem *item = ScanSettings::self()->startupOnlyLocalItem();
    Q_ASSERT(item!=NULL);
    mNetQueryCheck = new QCheckBox(item->label(), this);
    mNetQueryCheck->setToolTip(item->toolTip());
    mLayout->addWidget(mNetQueryCheck);

    /* Show scanner selection box on startup (Checkbox) */
    item = ScanSettings::self()->startupSkipAskItem();
    Q_ASSERT(item!=NULL);
    mSelectScannerCheck = new QCheckBox(item->label(), this);
    mSelectScannerCheck->setToolTip(item->toolTip());
    mLayout->addWidget(mSelectScannerCheck);

    /* Read startup image on startup (Checkbox) */
    item = KookaSettings::self()->startupReadImageItem();
    Q_ASSERT(item!=NULL);
    mRestoreImageCheck = new QCheckBox(item->label(), this);
    mRestoreImageCheck->setToolTip(item->toolTip());
    mLayout->addWidget(mRestoreImageCheck);

    applySettings();
}

void KookaStartupPage::saveSettings()
{
    ScanSettings::setStartupSkipAsk(!mSelectScannerCheck->isChecked());
    ScanSettings::setStartupOnlyLocal(!mNetQueryCheck->isChecked());
    ScanSettings::self()->save();

    KookaSettings::setStartupReadImage(mRestoreImageCheck->isChecked());
    KookaSettings::self()->save();
}

void KookaStartupPage::defaultSettings()
{
    ScanSettings::self()->startupSkipAskItem()->setDefault();
    ScanSettings::self()->startupOnlyLocalItem()->setDefault();
    KookaSettings::self()->startupReadImageItem()->setDefault();
    applySettings();
}


void KookaStartupPage::applySettings()
{
    mSelectScannerCheck->setChecked(!ScanSettings::startupSkipAsk());
    mNetQueryCheck->setChecked(!ScanSettings::startupOnlyLocal());
    mRestoreImageCheck->setChecked(KookaSettings::startupReadImage());
}


//  "Saving" page

KookaSavingPage::KookaSavingPage(KPageDialog *parent)
    : KookaPrefsPage(parent)
{
    const KConfigSkeletonItem *item = KookaSettings::self()->saverAskForFormatItem();
    Q_ASSERT(item!=NULL);
    mAskSaveFormat = new QCheckBox(item->label(), this);
    mAskSaveFormat->setToolTip(item->toolTip());
    mLayout->addWidget(mAskSaveFormat);

    mLayout->addSpacing(2 * KDialog::spacingHint());

    item = KookaSettings::self()->saverAskForFilenameItem();
    mAskFileName = new QCheckBox(item->label(), this);
    mAskFileName->setToolTip(item->toolTip());
    mLayout->addWidget(mAskFileName);

    QButtonGroup *bg = new QButtonGroup(this);
    QGridLayout *gl = new QGridLayout;
    gl->setVerticalSpacing(0);
    gl->setColumnMinimumWidth(0, 2 * KDialog::marginHint());

    item = KookaSettings::self()->saverAskBeforeScanItem();
    Q_ASSERT(item!=NULL);
    mAskBeforeScan = new QRadioButton(item->label(), this);
    mAskBeforeScan->setEnabled(mAskFileName->isChecked());
    mAskBeforeScan->setToolTip(item->toolTip());
    connect(mAskFileName, &QCheckBox::toggled, mAskBeforeScan, &QRadioButton::setEnabled);
    bg->addButton(mAskBeforeScan);
    gl->addWidget(mAskBeforeScan, 0, 1);

    item = KookaSettings::self()->saverAskAfterScanItem();
    Q_ASSERT(item!=NULL);
    mAskAfterScan = new QRadioButton(item->label(), this);
    mAskAfterScan->setEnabled(mAskFileName->isChecked());
    mAskAfterScan->setToolTip(item->toolTip());
    connect(mAskFileName, &QCheckBox::toggled, mAskAfterScan, &QRadioButton::setEnabled);
    bg->addButton(mAskAfterScan);
    gl->addWidget(mAskAfterScan, 1, 1);

    mLayout->addLayout(gl);

    applySettings();
}

void KookaSavingPage::saveSettings()
{
    KookaSettings::setSaverAskForFormat(mAskSaveFormat->isChecked());
    KookaSettings::setSaverAskForFilename(mAskFileName->isChecked());
    KookaSettings::setSaverAskBeforeScan(mAskBeforeScan->isChecked());
    KookaSettings::self()->save();
}

void KookaSavingPage::defaultSettings()
{
    KookaSettings::self()->saverAskForFormatItem()->setDefault();
    KookaSettings::self()->saverAskForFilenameItem()->setDefault();
    KookaSettings::self()->saverAskBeforeScanItem()->setDefault();
    applySettings();
}


void KookaSavingPage::applySettings()
{
    mAskSaveFormat->setChecked(KookaSettings::saverAskForFormat());
    mAskFileName->setChecked(KookaSettings::saverAskForFilename());
    bool askBefore = KookaSettings::saverAskBeforeScan();
    mAskBeforeScan->setChecked(askBefore);
    mAskAfterScan->setChecked(!askBefore);
}

//  "Gallery/Thumbnail" page

KookaThumbnailPage::KookaThumbnailPage(KPageDialog *parent)
    : KookaPrefsPage(parent)
{
    // Gallery options
    QGroupBox *gb = new QGroupBox(i18n("Image Gallery"), this);
    QGridLayout *gl = new QGridLayout(gb);
    gl->setColumnStretch(1, 1);

    /* Layout */
    const KConfigSkeletonItem *item = KookaSettings::self()->galleryLayoutItem();
    Q_ASSERT(item!=NULL);
    QLabel *lab = new QLabel(item->label(), gb);
    lab->setToolTip(item->toolTip());
    gl->addWidget(lab, 0, 0, Qt::AlignRight);

    mGalleryLayoutCombo = new KComboBox(gb);
    mGalleryLayoutCombo->setToolTip(item->toolTip());
// TODO: eliminate former enums
    mGalleryLayoutCombo->addItem(i18n("Not shown"), KookaSettings::RecentNone);
    mGalleryLayoutCombo->addItem(i18n("At top"), KookaSettings::RecentAtTop);
    mGalleryLayoutCombo->addItem(i18n("At bottom"), KookaSettings::RecentAtBottom);

    lab->setBuddy(mGalleryLayoutCombo);
    gl->addWidget(mGalleryLayoutCombo, 0, 1);

    /* Allow renaming */
    item = KookaSettings::self()->galleryAllowRenameItem();
    Q_ASSERT(item!=NULL);
    mAllowRenameCheck = new QCheckBox(item->label(), gb);
    mAllowRenameCheck->setToolTip(item->toolTip());
    gl->addWidget(mAllowRenameCheck, 1, 0, 1, 2);

    mLayout->addWidget(gb);

    // Thumbnail View options
    gb = new QGroupBox(i18n("Thumbnail View"), this);
    gl = new QGridLayout(gb);
    gl->setColumnStretch(1, 1);

    // Do we want a background image?
    item = KookaSettings::self()->thumbnailCustomBackgroundItem();
    Q_ASSERT(item!=NULL);
    mCustomBackgroundCheck = new QCheckBox(item->label(), this);
    mCustomBackgroundCheck->setToolTip(item->toolTip());
    connect(mCustomBackgroundCheck, &QCheckBox::toggled, this, &KookaThumbnailPage::slotCustomThumbBgndToggled);
    gl->addWidget(mCustomBackgroundCheck, 2, 0, 1, 2);

    /* Background image */
    item = KookaSettings::self()->thumbnailBackgroundPathItem();
    Q_ASSERT(item!=NULL);
    lab = new QLabel(item->label(), this);
    lab->setToolTip(item->toolTip());
    gl->addWidget(lab, 3, 0, Qt::AlignRight);

    /* Image file selector */
    mTileSelector = new KUrlRequester(this);
    mTileSelector->setToolTip(item->toolTip());
    mTileSelector->setMode(KFile::File|KFile::ExistingOnly|KFile::LocalOnly);
    mTileSelector->fileDialog()->setMimeTypeFilters(KImageIO::mimeTypes(KImageIO::Reading));
    gl->addWidget(mTileSelector, 3, 1);
    lab->setBuddy(mTileSelector);

    gl->setRowMinimumHeight(4, 2 * KDialog::spacingHint());

    /* Preview size */
    item = KookaSettings::self()->thumbnailPreviewSizeItem();
    Q_ASSERT(item!=NULL);
    lab = new QLabel(item->label(), this);
    lab->setToolTip(item->toolTip());
    gl->addWidget(lab, 5, 0, Qt::AlignRight);

    mThumbSizeCombo = new KComboBox(this);
    mThumbSizeCombo->setToolTip(item->toolTip());
    mThumbSizeCombo->addItem(ThumbView::sizeName(KIconLoader::SizeEnormous), KIconLoader::SizeEnormous);
    mThumbSizeCombo->addItem(ThumbView::sizeName(KIconLoader::SizeHuge), KIconLoader::SizeHuge);
    mThumbSizeCombo->addItem(ThumbView::sizeName(KIconLoader::SizeLarge), KIconLoader::SizeLarge);
    mThumbSizeCombo->addItem(ThumbView::sizeName(KIconLoader::SizeMedium), KIconLoader::SizeMedium);
    mThumbSizeCombo->addItem(ThumbView::sizeName(KIconLoader::SizeSmallMedium), KIconLoader::SizeSmallMedium);
    gl->addWidget(mThumbSizeCombo, 5, 1);
    lab->setBuddy(mThumbSizeCombo);

    mLayout->addWidget(gb);

    applySettings();
    slotCustomThumbBgndToggled(mCustomBackgroundCheck->isChecked());
}

void KookaThumbnailPage::saveSettings()
{
    KookaSettings::setGalleryAllowRename(mAllowRenameCheck->isChecked());
    KookaSettings::setGalleryLayout(mGalleryLayoutCombo->itemData(mGalleryLayoutCombo->currentIndex()).toInt());

    KookaSettings::setThumbnailCustomBackground(mCustomBackgroundCheck->isChecked());
    KookaSettings::setThumbnailBackgroundPath(mTileSelector->url().url(QUrl::PreferLocalFile));

    int size = mThumbSizeCombo->itemData(mThumbSizeCombo->currentIndex()).toInt();
    if (size>0) KookaSettings::setThumbnailPreviewSize(size);

    KookaSettings::self()->save();
}

void KookaThumbnailPage::defaultSettings()
{
    KookaSettings::self()->galleryLayoutItem()->setDefault();
    KookaSettings::self()->galleryAllowRenameItem()->setDefault();
    KookaSettings::self()->thumbnailCustomBackgroundItem()->setDefault();
    KookaSettings::self()->thumbnailBackgroundPathItem()->setDefault();
    KookaSettings::self()->thumbnailPreviewSizeItem()->setDefault();

    applySettings();
    slotCustomThumbBgndToggled(false);
}


void KookaThumbnailPage::applySettings()
{
    mGalleryLayoutCombo->setCurrentIndex(KookaSettings::galleryLayout());
    mAllowRenameCheck->setChecked(KookaSettings::galleryAllowRename());
    mCustomBackgroundCheck->setChecked(KookaSettings::thumbnailCustomBackground());

    mTileSelector->setUrl(QUrl::fromLocalFile(KookaSettings::thumbnailBackgroundPath()));

    KIconLoader::StdSizes size = static_cast<KIconLoader::StdSizes>(KookaSettings::thumbnailPreviewSize());
    int sel = mThumbSizeCombo->findData(size, Qt::UserRole, Qt::MatchExactly);
    if (sel!=-1) mThumbSizeCombo->setCurrentIndex(sel);
    //else kWarning() << "Cannot find combo index for size" << size;
}

void KookaThumbnailPage::slotCustomThumbBgndToggled(bool state)
{
    mTileSelector->setEnabled(state);
}

//  "OCR" page

KookaOcrPage::KookaOcrPage(KPageDialog *parent)
    : KookaPrefsPage(parent)
{
    QGridLayout *lay = new QGridLayout;
    lay->setColumnStretch(1, 9);

    mEngineCombo = new KComboBox(this);
    mEngineCombo->addItem(OcrEngine::engineName(OcrEngine::EngineNone), OcrEngine::EngineNone);
    mEngineCombo->addItem(OcrEngine::engineName(OcrEngine::EngineGocr), OcrEngine::EngineGocr);
    mEngineCombo->addItem(OcrEngine::engineName(OcrEngine::EngineOcrad), OcrEngine::EngineOcrad);
    mEngineCombo->addItem(OcrEngine::engineName(OcrEngine::EngineKadmos), OcrEngine::EngineKadmos);

    connect(mEngineCombo, static_cast<void (KComboBox::*)(int)>(&KComboBox::activated), this, &KookaOcrPage::slotEngineSelected);
    lay->addWidget(mEngineCombo, 0, 1);

    QLabel *lab = new QLabel(i18n("OCR Engine:"), this);
    lab->setBuddy(mEngineCombo);
    lay->addWidget(lab, 0, 0, Qt::AlignRight);

    lay->setRowMinimumHeight(1, KDialog::marginHint());

    mOcrBinaryReq = new KUrlRequester(this);
    mOcrBinaryReq->setMode(KFile::File | KFile::ExistingOnly | KFile::LocalOnly);
    lay->addWidget(mOcrBinaryReq, 2, 1);

    lab = new QLabel(i18n("Engine executable:"), this);
    lab->setBuddy(mOcrBinaryReq);
    lay->addWidget(lab, 2, 0, Qt::AlignRight);

    lay->setRowMinimumHeight(3, KDialog::marginHint());

    KSeparator *sep = new KSeparator(Qt::Horizontal, this);
    lay->addWidget(sep, 4, 0, 1, 2);
    lay->setRowMinimumHeight(5, KDialog::marginHint());

    mDescLabel = new QLabel("?", this);
    mDescLabel->setOpenExternalLinks(true);
    mDescLabel->setWordWrap(true);
    lay->addWidget(mDescLabel, 6, 0, 1, 2, Qt::AlignTop);
    lay->setRowStretch(6, 1);

    mLayout->addLayout(lay);

    OcrEngine::EngineType originalEngine = static_cast<OcrEngine::EngineType>(KookaSettings::ocrEngine());
    mEngineCombo->setCurrentIndex(originalEngine);
    slotEngineSelected(originalEngine);
}

void KookaOcrPage::saveSettings()
{
    KookaSettings::setOcrEngine(static_cast<int>(mSelectedEngine));

    QString path = mOcrBinaryReq->url().path();
    if (!path.isEmpty()) {
        switch (mSelectedEngine) {
case OcrEngine::EngineGocr:
            if (checkOcrBinary(path, "gocr", true)) KookaSettings::setOcrGocrBinary(path);
            break;

case OcrEngine::EngineOcrad:
            if (checkOcrBinary(path, "ocrad", true)) KookaSettings::setOcrOcradBinary(path);
            break;

default:    break;
        }
    }
}

void KookaOcrPage::defaultSettings()
{
    mEngineCombo->setCurrentIndex(OcrEngine::EngineNone);
    slotEngineSelected(OcrEngine::EngineNone);
}

void KookaOcrPage::slotEngineSelected(int i)
{
    mSelectedEngine = static_cast<OcrEngine::EngineType>(i);
    //qDebug() << "engine is" << mSelectedEngine;

    QString msg;
    switch (mSelectedEngine) {
    case OcrEngine::EngineNone:
        mOcrBinaryReq->setEnabled(false);
        mOcrBinaryReq->clear();
        msg = i18n("No OCR engine is selected. Select and configure one to perform OCR.");
        break;

    case OcrEngine::EngineGocr:
        mOcrBinaryReq->setEnabled(true);
        mOcrBinaryReq->setUrl(QUrl::fromLocalFile(KookaPref::tryFindGocr()));
        msg = OcrGocrEngine::engineDesc();
        break;

    case OcrEngine::EngineOcrad:
        mOcrBinaryReq->setEnabled(true);
        mOcrBinaryReq->setUrl(QUrl::fromLocalFile(KookaPref::tryFindOcrad()));
        msg = OcrOcradEngine::engineDesc();
        break;

    case OcrEngine::EngineKadmos:
        mOcrBinaryReq->setEnabled(false);
        mOcrBinaryReq->clear();
        msg = OcrKadmosEngine::engineDesc();
        break;

    default:
        mOcrBinaryReq->setEnabled(false);
        mOcrBinaryReq->clear();
        msg = i18n("Unknown engine %1.", mSelectedEngine);
        break;
    }

    mDescLabel->setText(msg);
}

bool KookaOcrPage::checkOcrBinary(const QString &cmd, const QString &bin, bool show_msg)
{
    // Why do we do this test?  See KookaPref::tryFindBinary().
    if (!cmd.contains(bin)) {
        return (false);
    }

    QFileInfo fi(cmd);
    if (!fi.exists()) {
        if (show_msg) KMessageBox::sorry(this, i18n("<qt>"
                                             "The path <filename>%1</filename> is not a valid binary.\n"
                                             "Please check the path and install the program if necessary.", cmd),
                                             i18n("OCR Engine Not Found"));
        return (false);
    } else {
        /* File exists, check if not dir and executable */
        if (fi.isDir() || (!fi.isExecutable())) {
            if (show_msg) KMessageBox::sorry(this, i18n("<qt>"
                                                 "The program <filename>%1</filename> exists, but is not executable.\n"
                                                 "Please check the path and permissions, and/or reinstall the program if necessary.", cmd),
                                                 i18n("OCR Engine Not Executable"));
            return (false);
        }
    }

    return (true);
}
