/* This file is part of the KDE Project
   Copyright (C) 2000 Jonathan Marten <jjm@keelhaul.me.uk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "scanparamsdialog.h"

#include <qlabel.h>
#include <qframe.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qlistwidget.h>

#include <klineedit.h>
#include <klocale.h>
#include <kconfig.h>
#include <QDebug>
#include <kmessagebox.h>
#include <kstandardguiitem.h>

extern "C" {
#include <sane/saneopts.h>
}

#include "kscandevice.h"
#include "kscanoption.h"
#include "kscanoptset.h"

#include "newscanparams.h"

ScanParamsDialog::ScanParamsDialog(QWidget *parent, KScanDevice *scandev)
    : KDialog(parent)
{
    setObjectName("ScanParamsDialog");

    setModal(true);
    setButtons(KDialog::Close);
    setCaption(i18n("Scan Parameters"));
    showButtonSeparator(true);

    // TODO: can this be just a QWidget?
    QFrame *mf = new QFrame(this);
    setMainWidget(mf);
    QGridLayout *gl = new QGridLayout(mf);

    paramsList = new QListWidget(mf);
    paramsList->setSelectionMode(QAbstractItemView::SingleSelection);
    paramsList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    paramsList->setMinimumWidth(200);
    connect(paramsList, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
            SLOT(slotSelectionChanged(QListWidgetItem *)));
    connect(paramsList, SIGNAL(itemDoubleClicked(QListWidgetItem *)),
            SLOT(slotLoadAndClose(QListWidgetItem *)));
    gl->addWidget(paramsList, 1, 0, 5, 1);

    QLabel *l = new QLabel(i18n("Saved scan parameter sets:"), mf);
    gl->addWidget(l, 0, 0, Qt::AlignLeft);
    l->setBuddy(paramsList);

    buttonLoad = new QPushButton(i18n("Load"), mf);
    connect(buttonLoad, SIGNAL(clicked()), SLOT(slotLoad()));
    buttonLoad->setToolTip(i18n("Load the selected scan parameter set to use as scanner settings"));
    gl->addWidget(buttonLoad, 1, 2);

    buttonSave = new QPushButton(i18n("Save..."), mf);
    connect(buttonSave, SIGNAL(clicked()), SLOT(slotSave()));
    buttonSave->setToolTip(i18n("Save the current scanner settings as a new scan parameter set"));
    gl->addWidget(buttonSave, 2, 2);

    buttonDelete = new QPushButton(i18n("Delete"), mf);
    connect(buttonDelete, SIGNAL(clicked()), SLOT(slotDelete()));
    buttonDelete->setToolTip(i18n("Delete the selected scan parameter set"));
    gl->addWidget(buttonDelete, 3, 2);

    buttonEdit = new QPushButton(i18n("Edit..."), mf);
    connect(buttonEdit, SIGNAL(clicked()), SLOT(slotEdit()));
    buttonEdit->setToolTip(i18n("Change the name or description of the selected scan parameter set"));
    gl->addWidget(buttonEdit, 4, 2);

    gl->setRowStretch(5, 9);
    gl->setRowMinimumHeight(6, KDialog::marginHint());

    gl->setColumnStretch(0, 9);
    gl->setColumnMinimumWidth(1, KDialog::marginHint());

    descLabel = new QLabel(i18n("-"), mf);
    gl->addWidget(descLabel, 7, 0, 1, 3);

    sane = scandev;

    populateList();
    slotSelectionChanged(NULL);
}

void ScanParamsDialog::populateList()
{
    paramsList->clear();
    sets = KScanOptSet::readList();

    for (KScanOptSet::StringMap::const_iterator it = sets.constBegin(); it != sets.constEnd(); ++it) {
        //qDebug() << "saveset" << it.key();
        paramsList->addItem(it.key());
    }
}

void ScanParamsDialog::slotSelectionChanged(QListWidgetItem *item)
{
    QString desc;
    bool enable = false;

    if (item == NULL) {
        desc = i18n("No save set selected.");
    } else {                    // something getting selected
        desc = sets[item->text()];
        enable = true;
    }

    descLabel->setText(desc);
    buttonLoad->setEnabled(enable);
    buttonDelete->setEnabled(enable);
    buttonEdit->setEnabled(enable);

    if (enable) {
        buttonLoad->setDefault(true);
    } else {
        setDefaultButton(KDialog::Close);
    }
}

void ScanParamsDialog::slotLoad()
{
    QListWidgetItem *item = paramsList->currentItem();
    if (item == NULL) {
        return;
    }
    QString name = item->text();
    //qDebug() << "set" << name;

    KScanOptSet optSet(name);
    if (!optSet.loadConfig()) {
        //qDebug() << "Failed to load set" << name;
        return;
    }

    sane->loadOptionSet(&optSet);
    sane->reloadAllOptions();
}

void ScanParamsDialog::slotLoadAndClose(QListWidgetItem *item)
{
    if (item == NULL) {
        return;
    }

    //qDebug() << "set" << item->text();

    paramsList->setCurrentItem(item);
    slotLoad();
    accept();
}

void ScanParamsDialog::slotSave()
{
    QString name = QString::null;
    QListWidgetItem *item = paramsList->currentItem();
    if (item != NULL) {
        name = item->text();
    }
    //qDebug() << "selected set" << name;

    QString newdesc = QString::null;
    if (sets.contains(name)) {
        newdesc = sets[name];
    } else {
        const KScanOption *sm = sane->getExistingGuiElement(SANE_NAME_SCAN_MODE);
        const KScanOption *sr = sane->getExistingGuiElement(SANE_NAME_SCAN_RESOLUTION);
        if (sm != NULL && sr != NULL) newdesc = i18n("%1, %2 dpi",
                                                    sm->get().constData(),
                                                    sr->get().constData());
    }

    NewScanParams d(this, name, newdesc, false);
    if (d.exec()) {
        QString newName = d.getName();
        QString newDesc = d.getDescription();

        //qDebug() << "name" << newName << "desc" << newDesc;

        KScanOptSet optSet(newName);
        sane->getCurrentOptions(&optSet);

        optSet.saveConfig(sane->scannerBackendName(), newDesc);
        sets[newName] = newDesc;

        // TODO: why?
        paramsList->setCurrentItem(NULL);
        QList<QListWidgetItem *> found = paramsList->findItems(newName, Qt::MatchFixedString | Qt::MatchCaseSensitive);
        if (found.count() == 0) {
            paramsList->addItem(newName);
            item = paramsList->item(paramsList->count() - 1);
        } else {
            item = found.first();
        }

        paramsList->setCurrentItem(item);
        slotSelectionChanged(item);
    }
}

void ScanParamsDialog::slotEdit()
{
    QListWidgetItem *item = paramsList->currentItem();
    if (item == NULL) {
        return;
    }
    QString oldName = item->text();
    //qDebug() << "selected set" << oldName;

    NewScanParams d(this, oldName, sets[oldName], true);
    if (d.exec()) {
        QString newName = d.getName();
        QString newDesc = d.getDescription();
        if (newName == oldName && newDesc == sets[oldName]) {
            return;
        }

        //qDebug() << "new name" << newName << "desc" << newDesc;

        KScanOptSet optSet(oldName.toLocal8Bit());
        if (!optSet.loadConfig()) {
            //qDebug() << "Failed to load set" << oldName;
            return;
        }

        KScanOptSet::deleteSet(oldName);        // do first, in case name not changed
        optSet.setSetName(newName);
        optSet.saveConfig(sane->scannerBackendName(), newDesc);

        sets.remove(oldName);               // do first, ditto
        sets[newName] = newDesc;

        int ix = paramsList->row(item);
        item->setText(newName);
        slotSelectionChanged(paramsList->item(ix)); // recalculate 'item', it may change
    }
}

void ScanParamsDialog::slotDelete()
{
    QListWidgetItem *item = paramsList->currentItem();
    if (item == NULL) {
        return;
    }
    QString name = item->text();
    //qDebug() << "set" << name;

    if (KMessageBox::warningContinueCancel(this, i18n("<qt>Do you really want to delete the set '<b>%1</b>'?", name),
                                           i18n("Delete Scan Parameter Set"),
                                           KStandardGuiItem::del(),
                                           KStandardGuiItem::cancel(),
                                           "deleteSaveSet") != KMessageBox::Continue) {
        return;
    }

    KScanOptSet::deleteSet(name);
    delete paramsList->takeItem(paramsList->row(item));
    paramsList->setCurrentItem(NULL);           // clear selection
}
