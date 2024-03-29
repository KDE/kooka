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
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qgroupbox.h>
#include <qcombobox.h>
#include <qguiapplication.h>

#include <klocalizedstring.h>
#include <kmessagebox.h>
#include <kurlrequester.h>
#include <kseparator.h>
#include <kconfigskeleton.h>
#include <kmessagewidget.h>

#include "scansettings.h"
#include "imagefilter.h"

#include "kookapref.h"
#include "kookagallery.h"
#include "formatdialog.h"
#include "thumbview.h"
#include "pluginmanager.h"
#include "abstractocrengine.h"
#include "kookasettings.h"
#include "kooka_logging.h"


//  Abstract base

KookaPrefsPage::KookaPrefsPage(KPageDialog *parent)
    : QWidget(parent)
{
    mLayout = new QVBoxLayout(this);
}

//  "General" page

KookaGeneralPage::KookaGeneralPage(KPageDialog *parent)
    : KookaPrefsPage(parent)
{
    mLayout->addStretch(9);				// push down to bottom

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
    qCDebug(KOOKA_LOG);

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
    Q_ASSERT(item!=nullptr);
    mNetQueryCheck = new QCheckBox(item->label(), this);
    mNetQueryCheck->setToolTip(item->toolTip());
    mLayout->addWidget(mNetQueryCheck);

    /* Use network proxy settings (Checkbox) */
    item = ScanSettings::self()->startupUseProxyItem();
    Q_ASSERT(item!=nullptr);
    mNetProxyCheck = new QCheckBox(item->label(), this);
    mNetProxyCheck->setToolTip(item->toolTip());
    mLayout->addWidget(mNetProxyCheck);

    mLayout->addSpacing(2*DialogBase::verticalSpacing());

    /* Show scanner selection box on startup (Checkbox) */
    item = ScanSettings::self()->startupSkipAskItem();
    Q_ASSERT(item!=nullptr);
    mSelectScannerCheck = new QCheckBox(item->label(), this);
    mSelectScannerCheck->setToolTip(item->toolTip());
    mLayout->addWidget(mSelectScannerCheck);

    /* Read startup image on startup (Checkbox) */
    item = KookaSettings::self()->startupReadImageItem();
    Q_ASSERT(item!=nullptr);
    mRestoreImageCheck = new QCheckBox(item->label(), this);
    mRestoreImageCheck->setToolTip(item->toolTip());
    mLayout->addWidget(mRestoreImageCheck);

    mLayout->addStretch(9);				// push down to bottom

    KMessageWidget *mw = new KMessageWidget(this);
    mw->setCloseButtonVisible(false);
    mw->setMessageType(KMessageWidget::Information);
    mw->setWordWrap(true);
    mw->setText(i18n("%1 must be restarted for these settings to take effect.", QGuiApplication::applicationDisplayName()));
    mw->setIcon(QIcon::fromTheme("dialog-information"));
    mLayout->addWidget(mw);

    applySettings();
}

void KookaStartupPage::saveSettings()
{
    ScanSettings::setStartupSkipAsk(!mSelectScannerCheck->isChecked());
    ScanSettings::setStartupOnlyLocal(!mNetQueryCheck->isChecked());
    ScanSettings::setStartupUseProxy(mNetProxyCheck->isChecked());
    ScanSettings::self()->save();

    KookaSettings::setStartupReadImage(mRestoreImageCheck->isChecked());
    KookaSettings::self()->save();
}

void KookaStartupPage::defaultSettings()
{
    ScanSettings::self()->startupSkipAskItem()->setDefault();
    ScanSettings::self()->startupOnlyLocalItem()->setDefault();
    ScanSettings::self()->startupUseProxyItem()->setDefault();
    KookaSettings::self()->startupReadImageItem()->setDefault();
    applySettings();
}


void KookaStartupPage::applySettings()
{
    mSelectScannerCheck->setChecked(!ScanSettings::startupSkipAsk());
    mNetQueryCheck->setChecked(!ScanSettings::startupOnlyLocal());
    mNetProxyCheck->setChecked(ScanSettings::startupUseProxy());
    mRestoreImageCheck->setChecked(KookaSettings::startupReadImage());
}


//  "Saving" page

KookaSavingPage::KookaSavingPage(KPageDialog *parent)
    : KookaPrefsPage(parent)
{
    const KConfigSkeletonItem *item = KookaSettings::self()->saverAskForFormatItem();
    Q_ASSERT(item!=nullptr);
    mAskSaveFormat = new QCheckBox(item->label(), this);
    mAskSaveFormat->setToolTip(item->toolTip());
    mLayout->addWidget(mAskSaveFormat);

    mLayout->addSpacing(2 * DialogBase::verticalSpacing());

    item = KookaSettings::self()->saverAskForFilenameItem();
    mAskFileName = new QCheckBox(item->label(), this);
    mAskFileName->setToolTip(item->toolTip());
    mLayout->addWidget(mAskFileName);

    QButtonGroup *bg = new QButtonGroup(this);
    QGridLayout *gl = new QGridLayout;
    gl->setVerticalSpacing(0);
    gl->setColumnMinimumWidth(0, 3*DialogBase::verticalSpacing());

    item = KookaSettings::self()->saverAskBeforeScanItem();
    Q_ASSERT(item!=nullptr);
    mAskBeforeScan = new QRadioButton(item->label(), this);
    mAskBeforeScan->setEnabled(mAskFileName->isChecked());
    mAskBeforeScan->setToolTip(item->toolTip());
    connect(mAskFileName, &QCheckBox::toggled, mAskBeforeScan, &QRadioButton::setEnabled);
    bg->addButton(mAskBeforeScan);
    gl->addWidget(mAskBeforeScan, 0, 1);

    item = KookaSettings::self()->saverAskAfterScanItem();
    Q_ASSERT(item!=nullptr);
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
    Q_ASSERT(item!=nullptr);
    QLabel *lab = new QLabel(item->label(), gb);
    lab->setToolTip(item->toolTip());
    gl->addWidget(lab, 0, 0, Qt::AlignRight);

    mGalleryLayoutCombo = new QComboBox(gb);
    mGalleryLayoutCombo->setToolTip(item->toolTip());
// TODO: eliminate former enums
    mGalleryLayoutCombo->addItem(i18n("Not shown"), KookaSettings::RecentNone);
    mGalleryLayoutCombo->addItem(i18n("At top"), KookaSettings::RecentAtTop);
    mGalleryLayoutCombo->addItem(i18n("At bottom"), KookaSettings::RecentAtBottom);

    lab->setBuddy(mGalleryLayoutCombo);
    gl->addWidget(mGalleryLayoutCombo, 0, 1);

    /* Allow renaming */
    item = KookaSettings::self()->galleryAllowRenameItem();
    Q_ASSERT(item!=nullptr);
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
    Q_ASSERT(item!=nullptr);
    mCustomBackgroundCheck = new QCheckBox(item->label(), this);
    mCustomBackgroundCheck->setToolTip(item->toolTip());
    connect(mCustomBackgroundCheck, &QCheckBox::toggled, this, &KookaThumbnailPage::slotCustomThumbBgndToggled);
    gl->addWidget(mCustomBackgroundCheck, 2, 0, 1, 2);

    /* Background image */
    item = KookaSettings::self()->thumbnailBackgroundPathItem();
    Q_ASSERT(item!=nullptr);
    lab = new QLabel(item->label(), this);
    lab->setToolTip(item->toolTip());
    gl->addWidget(lab, 3, 0, Qt::AlignRight);

    /* Image file selector */
    mTileSelector = new KUrlRequester(this);
    mTileSelector->setToolTip(item->toolTip());
    mTileSelector->setMode(KFile::File|KFile::ExistingOnly|KFile::LocalOnly);
    mTileSelector->setFilter(ImageFilter::kdeFilter(ImageFilter::Reading));
    gl->addWidget(mTileSelector, 3, 1);
    lab->setBuddy(mTileSelector);

    gl->setRowMinimumHeight(4, 2*DialogBase::verticalSpacing());

    /* Preview size */
    item = KookaSettings::self()->thumbnailPreviewSizeItem();
    Q_ASSERT(item!=nullptr);
    lab = new QLabel(item->label(), this);
    lab->setToolTip(item->toolTip());
    gl->addWidget(lab, 5, 0, Qt::AlignRight);

    mThumbSizeCombo = new QComboBox(this);
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

    const QString configuredEngine = KookaSettings::ocrEngineName();
    qCDebug(KOOKA_LOG) << "configured engine" << configuredEngine;
    int engineIndex = 0;

    mEngineCombo = new QComboBox(this);
    mOcrPlugins = PluginManager::self()->allPlugins(PluginManager::OcrPlugin);
    qCDebug(KOOKA_LOG) << "have" << mOcrPlugins.count() << "OCR plugins";

    mEngineCombo->addItem(i18n("None"), QString());
    for (QMap<QString,AbstractPluginInfo>::const_iterator it = mOcrPlugins.constBegin();
         it!=mOcrPlugins.constEnd(); ++it)
    {
        const AbstractPluginInfo &info = it.value();
        if (info.key==configuredEngine) engineIndex = mEngineCombo->count();
        mEngineCombo->addItem(QIcon::fromTheme(info.icon), info.name, info.key);
    }

    connect(mEngineCombo, QOverload<int>::of(&QComboBox::activated), this, &KookaOcrPage::slotEngineSelected);
    lay->addWidget(mEngineCombo, 0, 1);

    QLabel *lab = new QLabel(i18n("OCR Engine:"), this);
    lab->setBuddy(mEngineCombo);
    lay->addWidget(lab, 0, 0, Qt::AlignRight);

    lay->setRowMinimumHeight(1, 2*DialogBase::verticalSpacing());

    mOcrAdvancedButton = new QPushButton(i18n("OCR Engine Settings..."), this);
    connect(mOcrAdvancedButton, &QPushButton::clicked, this, &KookaOcrPage::slotOcrAdvanced);
    lay->addWidget(mOcrAdvancedButton, 2, 1, Qt::AlignRight);

    lay->setRowMinimumHeight(3, 2*DialogBase::verticalSpacing());

    KSeparator *sep = new KSeparator(Qt::Horizontal, this);
    lay->addWidget(sep, 4, 0, 1, 2);
    lay->setRowMinimumHeight(5, 2*DialogBase::verticalSpacing());

    mDescLabel = new QLabel("?", this);
    mDescLabel->setOpenExternalLinks(true);
    mDescLabel->setWordWrap(true);
    lay->addWidget(mDescLabel, 6, 0, 1, 2, Qt::AlignTop);
    lay->setRowStretch(6, 1);

    mLayout->addLayout(lay);

    mEngineCombo->setCurrentIndex(engineIndex);
    slotEngineSelected(mEngineCombo->currentIndex());
}


void KookaOcrPage::saveSettings()
{
    KookaSettings::setOcrEngineName(mEngineCombo->currentData().toString());
}


void KookaOcrPage::defaultSettings()
{
    mEngineCombo->setCurrentIndex(0);
    slotEngineSelected(mEngineCombo->currentIndex());
}


void KookaOcrPage::slotEngineSelected(int i)
{
    const QString pluginKey = mEngineCombo->currentData().toString();
    qCDebug(KOOKA_LOG) << "selected" << pluginKey;

    QString msg;					// description message
    bool enableAdvanced = false;			// for the moment, anyway

    if (pluginKey.isEmpty())				// no engine selected
    {
        msg = i18n("No OCR engine is selected. Select and configure one to be able to perform OCR.");
    }
    else						// an engine is selected,
    {							// try to load its plugin
        const AbstractPlugin *plugin = PluginManager::self()->loadPlugin(PluginManager::OcrPlugin, pluginKey);
        const AbstractOcrEngine *engine = qobject_cast<const AbstractOcrEngine *>(plugin);
        if (engine==nullptr)				// plugin not found
        {
            msg = i18n("Unknown engine '%1'.", pluginKey);
        }
        else
        {
            msg = engine->pluginInfo()->description;
            enableAdvanced = engine->hasAdvancedSettings();
        }
    }

    mDescLabel->setText(msg);				// show description text
    mOcrAdvancedButton->setEnabled(enableAdvanced);	// enable button if applicable
}


void KookaOcrPage::slotOcrAdvanced()
{
    AbstractPlugin *plugin = PluginManager::self()->currentPlugin(PluginManager::OcrPlugin);
    AbstractOcrEngine *engine = qobject_cast<AbstractOcrEngine *>(plugin);
    Q_ASSERT(engine!=nullptr);
    Q_ASSERT(engine->hasAdvancedSettings());

    engine->openAdvancedSettings();
}
