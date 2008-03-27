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

#include <klineedit.h>
#include <klocale.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kstdguiitem.h>

#include <qlabel.h>
#include <qframe.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qtooltip.h>

#include "kscandevice.h"
#include "kscanoptset.h"
#include "newscanparams.h"

#include "scanparamsdialog.h"
#include "scanparamsdialog.moc"


ScanParamsDialog::ScanParamsDialog(QWidget *parent,KScanDevice *scandev)
    : KDialogBase(parent,NULL,true,i18n("Scan Parameters"),KDialogBase::Close)
{
    enableButtonSeparator(true);

    QFrame *mf = makeMainWidget();
    QGridLayout *gl = new QGridLayout(mf,8,3,KDialog::marginHint(),KDialog::spacingHint());

    paramsList = new QListBox(mf);
    paramsList->setSelectionMode(QListBox::Single);
    paramsList->setVScrollBarMode(QScrollView::AlwaysOn);
    paramsList->setMinimumWidth(200);
    connect(paramsList,SIGNAL(selectionChanged(QListBoxItem *)),
            SLOT(slotSelectionChanged(QListBoxItem *)));
    connect(paramsList,SIGNAL(doubleClicked(QListBoxItem *)),
            SLOT(slotLoadAndClose(QListBoxItem *)));
    gl->addMultiCellWidget(paramsList,1,4,0,0);

    QLabel *l = new QLabel(i18n("Saved scan parameter sets:"),mf);
    gl->addWidget(l,0,0,Qt::AlignLeft);
    l->setBuddy(paramsList);

    buttonLoad = new QPushButton(i18n("Load"),mf);
    connect(buttonLoad,SIGNAL(clicked()),SLOT(slotLoad()));
    QToolTip::add(buttonLoad,i18n("Load the selected scan parameter set to use as scanner settings"));
    gl->addWidget(buttonLoad,1,2);

    buttonSave = new QPushButton(i18n("Save..."),mf);
    connect(buttonSave,SIGNAL(clicked()),SLOT(slotSave()));
    QToolTip::add(buttonSave,i18n("Save the current scanner settings as a new scan parameter set"));
    gl->addWidget(buttonSave,2,2);

    buttonDelete = new QPushButton(i18n("Delete"),mf);
    connect(buttonDelete,SIGNAL(clicked()),SLOT(slotDelete()));
    QToolTip::add(buttonDelete,i18n("Delete the selected scan parameter set"));
    gl->addWidget(buttonDelete,3,2);

    buttonEdit = new QPushButton(i18n("Edit..."),mf);
    connect(buttonEdit,SIGNAL(clicked()),SLOT(slotEdit()));
    QToolTip::add(buttonEdit,i18n("Change the name or description of the selected scan parameter set"));
    gl->addWidget(buttonEdit,4,2);

    gl->setRowStretch(5,9);
    gl->setRowSpacing(6,KDialog::marginHint());

    gl->setColStretch(0,9);
    gl->setColSpacing(1,KDialog::marginHint());

    descLabel = new QLabel(i18n("-"),mf);
    gl->addMultiCellWidget(descLabel,7,7,0,2);

    sane = scandev;

    populateList();
    slotSelectionChanged(NULL);
}


void ScanParamsDialog::populateList()
{
    paramsList->clear();
    sets = KScanOptSet::readList();

    for (KScanOptSet::StringMap::const_iterator it = sets.constBegin(); it!=sets.constEnd(); ++it)
    {
        kdDebug(28000) << k_funcinfo << "saveset [" << it.key() << "]" << endl;
        paramsList->insertItem(it.key());
    }
}


void ScanParamsDialog::slotSelectionChanged(QListBoxItem *item)
{
    QString desc;
    bool enable = false;

    if (item==NULL) desc = i18n("No save set selected.");
    else						// something getting selected
    {
        desc = sets[item->text()];
        enable = true;
    }

    descLabel->setText(desc);
    buttonLoad->setEnabled(enable);
    buttonDelete->setEnabled(enable);
    buttonEdit->setEnabled(enable);

    if (enable) buttonLoad->setDefault(true);
    else actionButton(KDialogBase::Close)->setDefault(true);
}



void ScanParamsDialog::slotLoad()
{

    QListBoxItem *item = paramsList->selectedItem();
    if (item==NULL) return;
    QString name = item->text();
    kdDebug(28000) << k_funcinfo << "set [" << name << "]" << endl;

    KScanOptSet optSet(name.local8Bit());
    if (!optSet.load())
    {
        kdDebug(28000) << k_funcinfo << "Failed to load set [" << name << "]!" << endl;
        return;
    }

    sane->loadOptionSet(&optSet);
    sane->slReloadAll();
}


void ScanParamsDialog::slotLoadAndClose(QListBoxItem *item)
{
    if (item==NULL) return;

    kdDebug(28000) << k_funcinfo << "set [" << item->text() << "]" << endl;

    paramsList->setSelected(item,true);
    slotLoad();
    accept();
}


void ScanParamsDialog::slotSave()
{
    QString name = QString::null;
    QListBoxItem *item = paramsList->selectedItem();
    if (item!=NULL) name = item->text();
    kdDebug(28000) << k_funcinfo << "selected set [" << name << "]" << endl;

    NewScanParams d(this,name,(sets.contains(name) ? sets[name] : QString::null),false);
    if (d.exec())
    {
        QString newName = d.getName();
        QString newDesc = d.getDescription();

        kdDebug(28000) << k_funcinfo << "name=[" << newName << "] desc=[" << newDesc << "]" << endl;

        KScanOptSet optSet(newName.local8Bit());
        sane->getCurrentOptions(&optSet);

        optSet.saveConfig(sane->shortScannerName(),newName,newDesc);
        sets[newName] = newDesc;

        paramsList->setCurrentItem(0);
        QListBoxItem *item = paramsList->findItem(newName,Qt::ExactMatch|Qt::CaseSensitive);
        if (item==NULL)
        {
            paramsList->insertItem(newName);
            item = paramsList->item(paramsList->numRows()-1);
        }
        paramsList->setSelected(item,true);
        slotSelectionChanged(item);
    }
}


void ScanParamsDialog::slotEdit()
{
    QListBoxItem *item = paramsList->selectedItem();
    if (item==NULL) return;
    QString oldName = item->text();
    kdDebug(28000) << k_funcinfo << "selected set [" << oldName << "]" << endl;

    NewScanParams d(this,oldName,sets[oldName],true);
    if (d.exec())
    {
        QString newName = d.getName();
        QString newDesc = d.getDescription();
        if (newName==oldName && newDesc==sets[oldName]) return;

        kdDebug(28000) << k_funcinfo << "new name=[" << newName << "] desc=[" << newDesc << "]" << endl;

        KScanOptSet optSet(oldName.local8Bit());
        if (!optSet.load())
        {
            kdDebug(28000) << k_funcinfo << "Failed to load set [" << oldName << "]!" << endl;
            return;
        }

        KScanOptSet::deleteSet(oldName);		// do first, in case name not changed
        optSet.saveConfig(sane->shortScannerName(),newName,newDesc);

        sets.erase(oldName);				// do first, ditto
        sets[newName] = newDesc;

        int ix = paramsList->index(item);
        paramsList->changeItem(newName,ix);
        slotSelectionChanged(paramsList->item(ix));	// recalculate 'item', it may change
    }
}


void ScanParamsDialog::slotDelete()
{
    QListBoxItem *item = paramsList->selectedItem();
    if (item==NULL) return;
    QString name = item->text();
    kdDebug(28000) << k_funcinfo << "set [" << name << "]" << endl;

    if (KMessageBox::warningContinueCancel(this,i18n("<qt>Do you really want to delete the set '<b>%1</b>'?").arg(name),
                                           i18n("Delete Scan Parameter Set"),
                                           KStdGuiItem::del(),
                                           "deleteSaveSet")!=KMessageBox::Continue) return;

    KScanOptSet::deleteSet(name);
    paramsList->removeItem(paramsList->index(item));
    slotSelectionChanged(NULL);
}
