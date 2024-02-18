/***************************************************************************
 *                                                                         *
 *  This file is part of Kooka, a KDE scanning/OCR application.            *
 *                                                                         *
 *  This file may be distributed and/or modified under the terms of the    *
 *  GNU General Public License version 2 as published by the Free Software *
 *  Foundation and appearing in the file COPYING included in the           *
 *  packaging of this file.                                                *
 *                                                                         *
 *  As a special exception, permission is given to link this program       *
 *  with any version of the KADMOS ocr/icr engine of reRecognition GmbH,   *
 *  Kreuzlingen and distribute the resulting executable without            *
 *  including the source code for KADMOS in the source distribution.       *
 *                                                                         *
 *  As a special exception, permission is given to link this program       *
 *  with any edition of Qt, and distribute the resulting executable,       *
 *  without including the source code for Qt in the source distribution.   *
 *                                                                         *
 *  Author:  Jonathan Marten <jjm AT keelhaul DOT me DOT uk>               *
 *                                                                         *
 ***************************************************************************/

#include "kookascanparams.h"

#include <qicon.h>
#include <qcombobox.h>
#include <qlabel.h>

#include <kmessagewidget.h>
#include <klocalizedstring.h>

#include "abstractplugin.h"
#include "abstractdestination.h"
#include "pluginmanager.h"
#include "scanparamspage.h"
#include "kookasettings.h"
#include "kooka_logging.h"


#define ROWS_TO_RESERVE		5			// rows to reserve for plugin GUI


KookaScanParams::KookaScanParams(QWidget *parent)
    : ScanParams(parent)
{
    mNoScannerMessage = nullptr;
    mDestinationPlugin = nullptr;

    connect(this, &ScanParams::scanBatchStart, this, [this]()
    {
        if (mDestinationPlugin!=nullptr) mDestinationPlugin->batchStart();
    });

    connect(this, &ScanParams::scanBatchEnd, this, [this](bool ok)
    {
        if (mDestinationPlugin!=nullptr) mDestinationPlugin->batchEnd(ok);
    });
}


static KMessageWidget *createErrorWidget(const QString &text)
{
    KMessageWidget *msg = new KMessageWidget(text);
    msg->setMessageType(KMessageWidget::Error);
    msg->setIcon(QIcon::fromTheme("dialog-error"));
    msg->setWordWrap(true);
    msg->setCloseButtonVisible(false);
    return (msg);
}


QWidget *KookaScanParams::messageScannerNotSelected()
{
    if (mNoScannerMessage==nullptr)
    {
        mNoScannerMessage = createErrorWidget(
            xi18nc("@info",
                   "<title>Gallery Mode - No scanner selected</title>"
                   "<nl/><nl/>"
                   "In this mode you can browse, manipulate and OCR images already in the gallery."
                   "<nl/><nl/>"
                   "<link url=\"a:1\">Select a scanner device</link> "
                   "to perform scanning, or "
                   "<link url=\"a:2\">add a device</link> "
                   "if a scanner is not automatically detected."));

        mNoScannerMessage->setMessageType(KMessageWidget::Information);
        mNoScannerMessage->setIcon(QIcon::fromTheme("dialog-information"));
        connect(mNoScannerMessage, &KMessageWidget::linkActivated, this, &KookaScanParams::slotLinkActivated);
    }

    return (mNoScannerMessage);
}


void KookaScanParams::slotLinkActivated(const QString &link)
{
    if (link == QLatin1String("a:1")) {
        emit actionSelectScanner();
    } else if (link == QLatin1String("a:2")) {
        emit actionAddScanner();
    }
}


void KookaScanParams::createScanDestinationGUI(ScanParamsPage *frame)
{
    const QMap<QString,AbstractPluginInfo> plugins = PluginManager::self()->allPlugins(PluginManager::DestinationPlugin);
    qCDebug(KOOKA_LOG) << "have" << plugins.count() << "destination plugins";

    const QString configuredPlugin = KookaSettings::destinationPlugin();
    int configuredIndex = -1;

    mDestinationCombo = new QComboBox(frame);
    for (QMap<QString,AbstractPluginInfo>::const_iterator it = plugins.constBegin(); it!=plugins.constEnd(); ++it)
    {
        const AbstractPluginInfo &info = it.value();
        if (info.key==configuredPlugin) configuredIndex = mDestinationCombo->count();
        mDestinationCombo->addItem(QIcon::fromTheme(info.icon), info.name, info.key);
    }

    if (configuredIndex!=-1) mDestinationCombo->setCurrentIndex(configuredIndex);
    connect(mDestinationCombo, QOverload<int>::of(&QComboBox::activated), this, &KookaScanParams::slotDestinationSelected);
    frame->addRow(i18n("Scan &destination:"), mDestinationCombo);

    mParamsPage = frame;
    mParamsRow = frame->currentRow();

    slotDestinationSelected(mDestinationCombo->currentIndex());
    frame->setCurrentRow(mParamsRow+ROWS_TO_RESERVE);
}


void KookaScanParams::slotDestinationSelected(int idx)
{
    const QString destName = mDestinationCombo->itemData(idx).toString();
    qCDebug(KOOKA_LOG) << idx << destName;

    // Check the plugin that is currently loaded.  If it is the same as is
    // required then nothing needs to be done.
    AbstractDestination *currentPlugin = qobject_cast<AbstractDestination *>(PluginManager::self()->currentPlugin(PluginManager::DestinationPlugin));
    if (currentPlugin!=nullptr && currentPlugin->pluginInfo()->key==destName)
    {
        // But ensure that this is not the initialisation of scan parameters
        // for a new scanner, after the same plugin has previously been
        // loaded for the previous scanner.  In this case, the GUI setup
        // still needs to be done.
        if (mDestinationPlugin!=nullptr) return;
    }

    qCDebug(KOOKA_LOG) << "params at row" << mParamsRow;
    for (int i = ROWS_TO_RESERVE-1; i>=0; --i)		// clear any existing GUI rows,
    {							// backwards to leave first row as current
        mParamsPage->setCurrentRow(mParamsRow+i);
        mParamsPage->clearRow();
    }

    if (destName.isEmpty())
    {
        mParamsPage->addRow(createErrorWidget(xi18nc("@info", "No destination selected")));
        return;
    }

    // Save the settings for the current plugin before it gets unloaded.
    if (currentPlugin!=nullptr) currentPlugin->saveSettings();

    mDestinationPlugin = qobject_cast<AbstractDestination *>(PluginManager::self()->loadPlugin(PluginManager::DestinationPlugin, destName));
    if (mDestinationPlugin==nullptr)
    {
        mParamsPage->addRow(createErrorWidget(xi18nc("@info", "Unable to load plugin '%1'", destName)));
        return;
    }

    mDestinationPlugin->setParentWidget(this);		// give it a widget for reference
    mDestinationPlugin->createGUI(mParamsPage);		// create the plugin's GUI
}


void KookaScanParams::saveDestinationSettings()
{
    qCDebug(KOOKA_LOG);

    if (!hasScanDevice()) return;			// no scanner configured

    AbstractDestination *currentPlugin = qobject_cast<AbstractDestination *>(PluginManager::self()->currentPlugin(PluginManager::DestinationPlugin));
    if (currentPlugin==nullptr) return;			// nothing to save

    KookaSettings::setDestinationPlugin(currentPlugin->pluginInfo()->key);
    currentPlugin->saveSettings();

    KookaSettings::self()->save();
}
