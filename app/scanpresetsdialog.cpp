/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2008-2016 Jonathan Marten <jjm@keelhaul.me.uk>	*
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

#include "scanpresetsdialog.h"

#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qlistwidget.h>

#include <klocalizedstring.h>
#include <kmessagebox.h>
#include <kstandardguiitem.h>

extern "C" {
#include <sane/saneopts.h>
}

#include "kscandevice.h"
#include "kscanoption.h"
#include "kscanoptset.h"
#include "kscancontrols.h"
#include "newscanpresetdialog.h"
#include "kooka_logging.h"


// TODO: also associate an icon, default the "color"/"grey" etc

ScanPresetsDialog::ScanPresetsDialog(KScanDevice *scandev, QWidget *pnt)
    : DialogBase(pnt)
{
    setObjectName("ScanPresetsDialog");

    setButtons(QDialogButtonBox::Close);
    setWindowTitle(i18n("Scan Presets"));

    QWidget *w = new QWidget(this);
    QGridLayout *gl = new QGridLayout(w);

    mParamsList = new QListWidget(w);
    mParamsList->setSelectionMode(QAbstractItemView::SingleSelection);
    mParamsList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    mParamsList->setMinimumWidth(200);
    connect(mParamsList, &QListWidget::itemSelectionChanged, this, &ScanPresetsDialog::slotSelectionChanged);
    connect(mParamsList, &QListWidget::itemDoubleClicked, this, &ScanPresetsDialog::slotLoadAndClose);
    gl->addWidget(mParamsList, 1, 0, 5, 1);

    QLabel *l = new QLabel(i18n("Saved scan preset sets:"), w);
    gl->addWidget(l, 0, 0, Qt::AlignLeft);
    l->setBuddy(mParamsList);

    mLoadButton = new QPushButton(i18n("Select"), w);
    mLoadButton->setIcon(QIcon::fromTheme("dialog-ok-apply"));
    mLoadButton->setToolTip(i18n("Load the selected scan preset set to use as scanner settings"));
    connect(mLoadButton, &QPushButton::clicked, this, &ScanPresetsDialog::slotLoad);
    gl->addWidget(mLoadButton, 1, 2);

    mSaveButton = new QPushButton(i18n("Save..."), w);
    mSaveButton->setIcon(QIcon::fromTheme("bookmark-new"));
    mSaveButton->setToolTip(i18n("Save the current scanner settings as a new scan preset set"));
    connect(mSaveButton, &QPushButton::clicked, this, &ScanPresetsDialog::slotSave);
    gl->addWidget(mSaveButton, 2, 2);

    mDeleteButton = new QPushButton(w);
    KGuiItem::assign(mDeleteButton, KStandardGuiItem::del());
    mDeleteButton->setToolTip(i18n("Delete the selected scan preset set"));
    connect(mDeleteButton, &QPushButton::clicked, this, &ScanPresetsDialog::slotDelete);
    gl->addWidget(mDeleteButton, 3, 2);

    mEditButton = new QPushButton(i18n("Edit..."), w);
    mEditButton->setIcon(QIcon::fromTheme("document-edit"));
    mEditButton->setToolTip(i18n("Change the name or description of the selected scan preset set"));
    connect(mEditButton, &QPushButton::clicked, this, &ScanPresetsDialog::slotEdit);
    gl->addWidget(mEditButton, 4, 2);

    gl->setRowStretch(5, 9);
    gl->setRowMinimumHeight(6, 2*DialogBase::verticalSpacing());

    gl->setColumnStretch(0, 9);
    gl->setColumnMinimumWidth(1, 2*DialogBase::horizontalSpacing());

    mDescLabel = new QLabel(w);
    gl->addWidget(mDescLabel, 7, 0, 1, 3);

    mScanDevice = scandev;

    setMainWidget(w);
    populateList();
    slotSelectionChanged();
}


void ScanPresetsDialog::populateList()
{
    mParamsList->clear();
    mSets = KScanOptSet::readList();

    for (KScanOptSet::StringMap::const_iterator it = mSets.constBegin(); it != mSets.constEnd(); ++it)
    {
        qCDebug(KOOKA_LOG) << "saveset" << it.key();
        mParamsList->addItem(it.key());
    }
}


void ScanPresetsDialog::slotSelectionChanged()
{
    QString desc;
    bool enable = false;

    QListWidgetItem *item = mParamsList->currentItem();
    if (item == nullptr) desc = i18n("No preset set selected.");
    else						// something getting selected
    {
        desc = mSets[item->text()];
        enable = true;
    }

    mDescLabel->setText(desc);
    mLoadButton->setEnabled(enable);
    mDeleteButton->setEnabled(enable);
    mEditButton->setEnabled(enable);

    mLoadButton->setDefault(enable);
    buttonBox()->button(QDialogButtonBox::Close)->setDefault(!enable);
}


void ScanPresetsDialog::slotLoad()
{
    QListWidgetItem *item = mParamsList->currentItem();
    if (item == nullptr) return;

    QString name = item->text();
    qCDebug(KOOKA_LOG) << "set" << name;

    KScanOptSet optSet(name);
    if (!optSet.loadConfig())
    {
        qCWarning(KOOKA_LOG) << "Failed to load set" << name;
        return;
    }

    mScanDevice->loadOptionSet(&optSet);
    mScanDevice->reloadAllOptions();
}


void ScanPresetsDialog::slotLoadAndClose(QListWidgetItem *item)
{
    if (item == nullptr) return;

    qCDebug(KOOKA_LOG) << "set" << item->text();
    mParamsList->setCurrentItem(item);
    slotLoad();
    accept();
}


void ScanPresetsDialog::slotSave()
{
    QString name;
    QListWidgetItem *item = mParamsList->currentItem();
    if (item != nullptr) name = item->text();
    qCDebug(KOOKA_LOG) << "selected set" << name;

    QString newdesc;
    if (mSets.contains(name)) newdesc = mSets[name];
    else
    {
        const KScanOption *sm = mScanDevice->getExistingGuiElement(SANE_NAME_SCAN_MODE);
        const KScanOption *sr = mScanDevice->getExistingGuiElement(SANE_NAME_SCAN_RESOLUTION);
        if (sm != nullptr && sr != nullptr)
        {
            // See KScanCombo::setList() for explanation
            QString scanMode = ki18n(sm->widget()->text().toLocal8Bit().constData()).toString("mScanDevice-backends");
            if (scanMode.isEmpty()) scanMode = sm->get();
            newdesc = i18nc("New set name, %1=scan mode, %2=resolution", "%1, %2 dpi",
                            scanMode, sr->get().constData());
        }
    }

    NewScanPresetDialog d(name, newdesc, false, this);
    if (!d.exec()) return;

    const QString newName = d.getName();
    const QString newDesc = d.getDescription();
    qCDebug(KOOKA_LOG) << "name" << newName << "desc" << newDesc;

    KScanOptSet optSet(newName);
    mScanDevice->getCurrentOptions(&optSet);

    optSet.saveConfig(mScanDevice->scannerBackendName(), newDesc);
    mSets[newName] = newDesc;

    // TODO: why?
    mParamsList->setCurrentItem(nullptr);
    QList<QListWidgetItem *> found = mParamsList->findItems(newName, Qt::MatchFixedString | Qt::MatchCaseSensitive);
    if (found.count() == 0) {
        mParamsList->addItem(newName);
        item = mParamsList->item(mParamsList->count() - 1);
    } else {
        item = found.first();
    }

    mParamsList->setCurrentItem(item);
    slotSelectionChanged();
}


void ScanPresetsDialog::slotEdit()
{
    QListWidgetItem *item = mParamsList->currentItem();
    if (item == nullptr) {
        return;
    }
    QString oldName = item->text();
    qCDebug(KOOKA_LOG) << "selected set" << oldName;

    NewScanPresetDialog d(oldName, mSets[oldName], true, this);
    if (!d.exec()) return;

    const QString newName = d.getName();
    const QString newDesc = d.getDescription();
    if (newName==oldName && newDesc==mSets[oldName]) return;
    qCDebug(KOOKA_LOG) << "new name" << newName << "desc" << newDesc;

    KScanOptSet optSet(oldName.toLocal8Bit());
    if (!optSet.loadConfig()) {
        qCWarning(KOOKA_LOG) << "Failed to load set" << oldName;
        return;
    }

    KScanOptSet::deleteSet(oldName);			// do first, in case name not changed
    optSet.setSetName(newName);
    optSet.saveConfig(mScanDevice->scannerBackendName(), newDesc);

    mSets.remove(oldName);				// do first, ditto
    mSets[newName] = newDesc;

    item->setText(newName);
    slotSelectionChanged();				// recalculate 'item', it may change
}


void ScanPresetsDialog::slotDelete()
{
    QListWidgetItem *item = mParamsList->currentItem();
    if (item == nullptr) return;

    QString name = item->text();
    qCDebug(KOOKA_LOG) << "set" << name;
    if (KMessageBox::warningContinueCancel(this,
                                           xi18nc("@info", "Do you really want to delete the set <resource>%1</resource>?", name),
                                           i18n("Delete Scan Preset Set"),
                                           KStandardGuiItem::del(),
                                           KStandardGuiItem::cancel(),
                                           "deleteSaveSet") != KMessageBox::Continue) return;

    KScanOptSet::deleteSet(name);
    delete mParamsList->takeItem(mParamsList->row(item));
    mParamsList->setCurrentItem(nullptr);		// clear selection
}
