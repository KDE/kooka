/* This file is part of the KDE Project

   Copyright (C) 2000 Klaas Freitag <freitag@suse.de>
   Copyright (C) 2010 Jonathan Marten <jjm@keelhaul.me.uk>  

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   As a special exception, permission is given to link this program
   with any version of the KADMOS ocr/icr engine of reRecognition GmbH,
   Kreuzlingen and distribute the resulting executable without
   including the source code for KADMOS in the source distribution.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.

*/

#include "prefspages.h"
#include "prefspages.moc"

#include <qlayout.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qfileinfo.h>

#include <klocale.h>
#include <kdebug.h>
#include <kcombobox.h>
#include <kmessagebox.h>
#include <kurlrequester.h>
#include <kseparator.h>

#include "libkscan/scanglobal.h"

#include "kookapref.h"
#include "kookagallery.h"
#include "imgsaver.h"
#include "thumbview.h"
#include "formatdialog.h"
#include "ocrgocrengine.h"
#include "ocrocradengine.h"
#include "ocrkadmosengine.h"


//  Abstract base

KookaPrefsPage::KookaPrefsPage(KPageDialog *parent, const char *configGroup)
    : QWidget(parent)
{
    mLayout = new QVBoxLayout(this);

    if (configGroup!=NULL) mConfig = KGlobal::config()->group(configGroup);
}


KookaPrefsPage::~KookaPrefsPage()
{
}


//  "General" page

KookaGeneralPage::KookaGeneralPage(KPageDialog *parent)
    : KookaPrefsPage(parent, GROUP_GALLERY)
{
    mLayout->addStretch(9);				// push down to bottom

    QGroupBox *gb = new QGroupBox(i18n("Hidden Messages"), this);
    QGridLayout *gl = new QGridLayout(gb);

    /* Enable messages and questions */
    QLabel *lab = new QLabel(i18n("Use this button to reenable all messages and questions which\nhave been hidden by using \"Don't ask me again\"."), gb);
    gl->addWidget(lab, 0, 0);

    mEnableMessagesButton = new QPushButton(i18n("Enable Messages/Questions"), gb);
    connect(mEnableMessagesButton, SIGNAL(clicked()), SLOT(slotEnableWarnings()));
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
    kDebug();

    KMessageBox::enableAllMessages();
    FormatDialog::forgetRemembered();
    KGlobal::config()->reparseConfiguration();

    mEnableMessagesButton->setEnabled(false);			// show this has been done
}


//  "Startup" page

KookaStartupPage::KookaStartupPage(KPageDialog *parent)
    : KookaPrefsPage(parent, GROUP_STARTUP)
{
    const KConfigGroup grp2 = ScanGlobal::self()->configGroup();

    /* Query for network scanner (Checkbox) */
    mNetQueryCheck = new QCheckBox(i18n("Query network for available scanners"), this);
    mNetQueryCheck->setToolTip(i18n("Check this if you want a network query for available scanners.\nNote that this does not mean a query over the entire network but only the stations configured for SANE."));
    mNetQueryCheck->setChecked(!grp2.readEntry(STARTUP_ONLY_LOCAL, false));
    mLayout->addWidget(mNetQueryCheck);

    /* Show scanner selection box on startup (Checkbox) */
    mSelectScannerCheck = new QCheckBox(i18n("Show the scanner selection dialog"), this);
    mSelectScannerCheck->setToolTip(i18n("Check this to show the scanner selection dialog on startup."));
    mSelectScannerCheck->setChecked(!grp2.readEntry(STARTUP_SKIP_ASK, false));
    mLayout->addWidget(mSelectScannerCheck);

    /* Read startup image on startup (Checkbox) */
    mRestoreImageCheck = new QCheckBox(i18n("Load the last selected image into the viewer"), this);
    mRestoreImageCheck->setToolTip(i18n("Check this if you want Kooka to load the last selected image into the viewer on startup.\nIf your images are large, that might slow down Kooka's startup."));
    mRestoreImageCheck->setChecked(mConfig.readEntry(STARTUP_READ_IMAGE, true));
    mLayout->addWidget(mRestoreImageCheck);
}


void KookaStartupPage::saveSettings()
{
    KConfigGroup grp2 = ScanGlobal::self()->configGroup();  // global, for libkscan also

    bool cbVal = !mSelectScannerCheck->isChecked();
    kDebug() << "Writing" << STARTUP_SKIP_ASK << "as" << cbVal;
    grp2.writeEntry(STARTUP_SKIP_ASK, cbVal);
    grp2.writeEntry(STARTUP_ONLY_LOCAL, !mNetQueryCheck->isChecked());
							// Kooka startup options
    mConfig.writeEntry(STARTUP_READ_IMAGE, mRestoreImageCheck->isChecked());
}


void KookaStartupPage::defaultSettings()
{
    mNetQueryCheck->setChecked(true);
    mSelectScannerCheck->setChecked(true);
    mRestoreImageCheck->setChecked(false);
}


//  "Saving" page

KookaSavingPage::KookaSavingPage(KPageDialog *parent)
    : KookaPrefsPage(parent, OP_SAVER_GROUP)
{
    mAskSaveFormat = new QCheckBox(i18n("Always use the Save Assistant"), this);
    mAskSaveFormat->setChecked(mConfig.readEntry(OP_SAVER_ASK_FORMAT, false));
    mAskSaveFormat->setToolTip(i18n("Check this if you want to always use the image save assistant, even if there is a default format for the image type."));
    mLayout->addWidget(mAskSaveFormat);

    mAskFileName = new QCheckBox(i18n("Ask for filename when saving"), this);
    mAskFileName->setChecked(mConfig.readEntry(OP_SAVER_ASK_FILENAME, false));
    mAskFileName->setToolTip(i18n("Check this if you want to enter a filename when an image has been scanned."));
    mLayout->addWidget(mAskFileName);
}


void KookaSavingPage::saveSettings()
{
    mConfig.writeEntry(OP_SAVER_ASK_FORMAT, mAskSaveFormat->isChecked());
    mConfig.writeEntry(OP_SAVER_ASK_FILENAME, mAskFileName->isChecked());
}


void KookaSavingPage::defaultSettings()
{
    mAskSaveFormat->setChecked(true);
    mAskFileName->setChecked(true);
}


//  "Gallery/Thumbnail" page

KookaThumbnailPage::KookaThumbnailPage(KPageDialog *parent)
    : KookaPrefsPage(parent, THUMB_GROUP)
{
    // Gallery options
    QGroupBox *gb = new QGroupBox(i18n("Image Gallery"), this);
    QGridLayout *gl = new QGridLayout(gb);
    gl->setColumnStretch(1, 1);

    /* Layout */
    QLabel *lab = new QLabel(i18n("Show recent folders:"), gb);
    gl->addWidget(lab, 0, 0, Qt::AlignRight);

    mGalleryLayoutCombo = new KComboBox(gb);
    mGalleryLayoutCombo->addItem(i18n("Not shown"), KookaGallery::NoRecent);
    mGalleryLayoutCombo->addItem(i18n("At top"), KookaGallery::RecentAtTop);
    mGalleryLayoutCombo->addItem(i18n("At bottom"), KookaGallery::RecentAtBottom);
    mGalleryLayoutCombo->setCurrentIndex(mConfig.readEntry(GALLERY_LAYOUT, static_cast<int>(KookaGallery::RecentAtTop)));
    lab->setBuddy(mGalleryLayoutCombo);
    gl->addWidget(mGalleryLayoutCombo, 0, 1);

    /* Allow renaming */
    mAllowRenameCheck = new QCheckBox(i18n("Allow click-to-rename"), gb);
    mAllowRenameCheck->setToolTip(i18n("Check this if you want to be able to rename gallery items by clicking on them (otherwise, use the \"Rename\" menu option)"));
    mAllowRenameCheck->setChecked(mConfig.readEntry(GALLERY_ALLOW_RENAME, false));
    gl->addWidget(mAllowRenameCheck, 1, 0, 1, 2);

    mLayout->addWidget(gb);

    // Thumbnail View options
    gb = new QGroupBox(i18n("Thumbnail View"), this);
    gl = new QGridLayout(gb);
    gl->setColumnStretch(1, 1);

    // Do we want a background image?
    mCustomBackgroundCheck = new QCheckBox(i18n("Use a custom background image"), this);
    mCustomBackgroundCheck->setChecked(mConfig.readEntry(THUMB_CUSTOM_BGND, false));
    connect(mCustomBackgroundCheck, SIGNAL(toggled(bool)), SLOT(slotCustomThumbBgndToggled(bool)));
    gl->addWidget(mCustomBackgroundCheck, 2, 0, 1, 2);

    /* Background image */
    QString bgImg = mConfig.readPathEntry(THUMB_BG_WALLPAPER, ThumbView::standardBackground());

    QLabel *l = new QLabel(i18n("Image:"), this);
    gl->addWidget(l, 3, 0, Qt::AlignRight);

    /* Image file selector */
    mTileSelector = new KUrlRequester(this);
    mTileSelector->setMode(KFile::File|KFile::ExistingOnly|KFile::LocalOnly);
    kDebug() << "Setting tile URL" << bgImg;
    mTileSelector->setUrl(bgImg);

    gl->addWidget(mTileSelector, 3, 1);
    l->setBuddy(mTileSelector);

    gl->setRowMinimumHeight(4, 2*KDialog::spacingHint());

    /* Preview size */
    l = new QLabel(i18n("Preview size:"), this);
    gl->addWidget(l, 5, 0, Qt::AlignRight);

    mThumbSizeCombo = new KComboBox(this);
    mThumbSizeCombo->addItem(ThumbView::sizeName(KIconLoader::SizeEnormous));		// 0
    mThumbSizeCombo->addItem(ThumbView::sizeName(KIconLoader::SizeHuge));		// 1
    mThumbSizeCombo->addItem(ThumbView::sizeName(KIconLoader::SizeLarge));		// 2
    mThumbSizeCombo->addItem(ThumbView::sizeName(KIconLoader::SizeMedium));		// 3
    mThumbSizeCombo->addItem(ThumbView::sizeName(KIconLoader::SizeSmallMedium));	// 4

    KIconLoader::StdSizes size = static_cast<KIconLoader::StdSizes>(mConfig.readEntry(THUMB_PREVIEW_SIZE,
                                                                                      static_cast<int>(KIconLoader::SizeHuge)));
    int sel;
    switch (size)
    {
case KIconLoader::SizeEnormous:		sel = 0;	break;
default:
case KIconLoader::SizeHuge:		sel = 1;	break;
case KIconLoader::SizeLarge:		sel = 2;	break;
case KIconLoader::SizeMedium:		sel = 3;	break;
case KIconLoader::SizeSmallMedium:	sel = 4;	break;
    }
    mThumbSizeCombo->setCurrentIndex(sel);

    gl->addWidget(mThumbSizeCombo, 5, 1);
    l->setBuddy(mThumbSizeCombo);

    mLayout->addWidget(gb);

    slotCustomThumbBgndToggled(mCustomBackgroundCheck->isChecked());
}


void KookaThumbnailPage::saveSettings()
{
    mConfig.writeEntry(GALLERY_ALLOW_RENAME, mAllowRenameCheck->isChecked());
    mConfig.writeEntry(GALLERY_LAYOUT, mGalleryLayoutCombo->currentIndex());

    mConfig.writePathEntry(THUMB_BG_WALLPAPER, mTileSelector->url().pathOrUrl());
    mConfig.writeEntry(THUMB_CUSTOM_BGND, mCustomBackgroundCheck->isChecked());

    KIconLoader::StdSizes size;
    switch (mThumbSizeCombo->currentIndex())
    {
case 0:		size = KIconLoader::SizeEnormous;	break;
default:
case 1:		size = KIconLoader::SizeHuge;		break;
case 2:		size = KIconLoader::SizeLarge;		break;
case 3:		size = KIconLoader::SizeMedium;		break;
case 4:		size = KIconLoader::SizeSmallMedium;	break;
    }
    mConfig.writeEntry(THUMB_PREVIEW_SIZE, static_cast<int>(size));
}


void KookaThumbnailPage::defaultSettings()
{
    mAllowRenameCheck->setChecked(false);
    mGalleryLayoutCombo->setCurrentIndex(KookaGallery::RecentAtTop);

    mCustomBackgroundCheck->setChecked(false);
    slotCustomThumbBgndToggled(false);
    mTileSelector->setUrl(ThumbView::standardBackground());
    mThumbSizeCombo->setCurrentIndex(1);			// "Very Large"
}


void KookaThumbnailPage::slotCustomThumbBgndToggled(bool state)
{
    mTileSelector->setEnabled(state);
}


//  "OCR" page

KookaOcrPage::KookaOcrPage(KPageDialog *parent)
    : KookaPrefsPage(parent, CFG_GROUP_OCR_DIA)
{
    QGridLayout *lay = new QGridLayout;
    lay->setColumnStretch(1, 9);

    mEngineCombo = new KComboBox(this);
    mEngineCombo->addItem(OcrEngine::engineName(OcrEngine::EngineNone), OcrEngine::EngineNone);
    mEngineCombo->addItem(OcrEngine::engineName(OcrEngine::EngineGocr), OcrEngine::EngineGocr);
    mEngineCombo->addItem(OcrEngine::engineName(OcrEngine::EngineOcrad), OcrEngine::EngineOcrad);
    mEngineCombo->addItem(OcrEngine::engineName(OcrEngine::EngineKadmos), OcrEngine::EngineKadmos);

    connect(mEngineCombo, SIGNAL(activated(int)), SLOT(slotEngineSelected(int)));
    lay->addWidget(mEngineCombo, 0, 1);

    QLabel *lab = new QLabel(i18n("OCR Engine:"), this);
    lab->setBuddy(mEngineCombo);
    lay->addWidget(lab, 0, 0, Qt::AlignRight);

    lay->setRowMinimumHeight(1, KDialog::marginHint());

    mOcrBinaryReq = new KUrlRequester(this);
    mOcrBinaryReq->setMode(KFile::File|KFile::ExistingOnly|KFile::LocalOnly);
    lay->addWidget(mOcrBinaryReq,2,1);

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

    OcrEngine::EngineType originalEngine = static_cast<OcrEngine::EngineType>(mConfig.readEntry(CFG_OCR_ENGINE2, static_cast<int>(OcrEngine::EngineNone)));
    mEngineCombo->setCurrentIndex(originalEngine);
    slotEngineSelected(originalEngine);
}


void KookaOcrPage::saveSettings()
{
    mConfig.writeEntry(CFG_OCR_ENGINE2, static_cast<int>(mSelectedEngine));

    QString path = mOcrBinaryReq->url().path();
    if (!path.isEmpty())
    {
        switch (mSelectedEngine)
        {
case OcrEngine::EngineGocr:
            if (checkOcrBinary(path, "gocr", true)) mConfig.writePathEntry(CFG_GOCR_BINARY, path);
            break;

case OcrEngine::EngineOcrad:
            if (checkOcrBinary(path, "ocrad", true)) mConfig.writePathEntry(CFG_OCRAD_BINARY, path);
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
    kDebug() << "engine is" << mSelectedEngine;

    QString msg;
    switch (mSelectedEngine)
    {
case OcrEngine::EngineNone:
        mOcrBinaryReq->setEnabled(false);
        mOcrBinaryReq->clear();
        msg = i18n("No OCR engine is selected. Select and configure one to perform OCR.");
        break;

case OcrEngine::EngineGocr:
        mOcrBinaryReq->setEnabled(true);
        mOcrBinaryReq->setUrl(KookaPref::tryFindGocr());
        msg = OcrGocrEngine::engineDesc();
        break;

case OcrEngine::EngineOcrad:
        mOcrBinaryReq->setEnabled(true);
        mOcrBinaryReq->setUrl(KookaPref::tryFindOcrad());
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
    if (!cmd.contains(bin)) return (false);

    QFileInfo fi(cmd);
    if (!fi.exists())
    {
        if (show_msg) KMessageBox::sorry(this,i18n("<qt>"
                                                   "The path <filename>%1</filename> is not a valid binary.\n"
                                                   "Please check the path and install the program if necessary.", cmd),
                                         i18n("OCR Engine Not Found"));
        return (false);
    }
    else
    {
        /* File exists, check if not dir and executable */
        if (fi.isDir() || (!fi.isExecutable()))
        {
            if (show_msg) KMessageBox::sorry(this,i18n("<qt>"
                                                       "The program <filename>%1</filename> exists, but is not executable.\n"
                                                       "Please check the path and permissions, and/or reinstall the program if necessary.", cmd),
                                             i18n("OCR Engine Not Executable"));
            return (false);
        }
    }

    return (true);
}
