/* This file is part of the KDE Project
   Copyright (C) 2000 Klaas Freitag <freitag@suse.de>  

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

#include "devselector.h"
#include "devselector.moc"

#include <qgroupbox.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qfile.h>
#include <qradiobutton.h>

#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kconfig.h>
#include <kconfiggroup.h>

/**
  * Internal private class for DeviceSelector dialog.
  */

class DeviceSelector::DeviceSelectorPrivate
{
public:
   QGroupBox      * selectBox;
   QVBoxLayout    * selectBoxLayout;
   QButtonGroup   * selectBoxGroup;
   QStringList    deviceList;
   QCheckBox      * cbSkipDialog;

public:
   DeviceSelectorPrivate()
        : selectBox(0), selectBoxLayout(0), selectBoxGroup(0), cbSkipDialog(0)
   { }
};



/**
  * Constructs the DeviceSelector object
  */

DeviceSelector::DeviceSelector(QWidget *parent,
                               const QList<QByteArray> &sources,
                               const QStringList &descs )
    : KDialog(parent)
{
    d = new DeviceSelectorPrivate();

    setObjectName("DeviceSelector");

    kDebug();

    setModal(true);
    setButtons(KDialog::Ok|KDialog::Cancel);
    setCaption(i18n("Select Scan Device"));

    // Layout-Boxes
    QWidget *page = new QWidget( this );
    setMainWidget( page );

    QVBoxLayout *topLayout = new QVBoxLayout(page);
    topLayout->setSpacing(KDialog::spacingHint());

    QLabel *label = new QLabel(page);
    label->setPixmap( QPixmap("kookalogo.png" ));
    label->resize( 100, 350 );
    topLayout->addWidget( label );

    d->selectBox = new QGroupBox( i18n( "Available Scanners"), page);
    d->selectBoxGroup = new QButtonGroup(d->selectBox);
    d->selectBoxLayout = new QVBoxLayout();
    d->selectBox->setLayout(d->selectBoxLayout);
        setScanSources(sources, descs);
    d->selectBoxLayout->addStretch();
    topLayout->addWidget( d->selectBox );

    d->cbSkipDialog = new QCheckBox( i18n("Always use this &device at startup"), this);
    const KSharedConfig *gcfg = KGlobal::config().data();
    const KConfigGroup grp = gcfg->group(GROUP_STARTUP);
    bool skipDialog = grp.readEntry(STARTUP_SKIP_ASK, false);
    d->cbSkipDialog->setChecked( skipDialog );
    topLayout->addWidget(d->cbSkipDialog);
}


DeviceSelector::~DeviceSelector()
{
    if (d)
    {
        delete d;
        d = NULL;
    }
}


QByteArray DeviceSelector::getDeviceFromConfig() const
{
    const KSharedConfig *gcfg = KGlobal::config().data();
    const KConfigGroup grp = gcfg->group(GROUP_STARTUP);
    bool skipDialog = grp.readEntry(STARTUP_SKIP_ASK, false);

    QByteArray result;

    /* in this case, the user has checked 'Do not ask me again' and does not
     * want to be bothered any more.
     */
    result = QFile::encodeName(grp.readEntry(STARTUP_SCANDEV, ""));
    kDebug() << "Got scanner from config file to use:" << result;
   
    /* Now check if the scanner read from the config file is available !
     * if not, ask the user !
     */
	if (skipDialog && d->deviceList.contains(result))
    {
        kDebug() << "Using scanner from config" << result;
    }
    else
    {
        kDebug() << "Cannot use scanner from config";
        result = "";
    }
   
    return (result);
}

bool DeviceSelector::getShouldSkip() const
{
   return( d->cbSkipDialog->isChecked());
}

QByteArray DeviceSelector::getSelectedDevice() const
{
    int selID = d->selectBoxGroup->checkedId();

    int dcount = d->deviceList.count();
    kDebug() << "selected ID:" << selID << "of" << dcount;

    QByteArray dev = d->deviceList.at(selID).toLocal8Bit();
    kDebug() << "selected device:" << dev;

    /* Store scanner selection settings */
    KSharedConfig *gcfg = KGlobal::config().data();
    KConfigGroup grp = gcfg->group(GROUP_STARTUP);

    /* Write both the scan device and the skip-start-dialog flag as KDE global. */
    grp.writeEntry(STARTUP_SCANDEV, dev, KConfigBase::Global);
    grp.writeEntry(STARTUP_SKIP_ASK, getShouldSkip(), KConfigBase::Global);
    grp.sync();
   
    return (dev);
}


void DeviceSelector::setScanSources(const QList<QByteArray> &sources,
                                    const QStringList &descs)
{
    const KSharedConfig *gcfg = KGlobal::config().data();
    const KConfigGroup grp = gcfg->group(GROUP_STARTUP);
    QByteArray defstr = grp.readEntry(STARTUP_SCANDEV).toLocal8Bit();

    /* Selector-Stuff*/
    QRadioButton * rbDef = NULL;
    int nr = 0;

    QStringList::const_iterator it2 = descs.constBegin();
    for (QList<QByteArray>::const_iterator it = sources.constBegin();
        it!=sources.constEnd(); ++it, ++it2 )
    {
        d->deviceList.append(*it);

        QString text = QString("&%1. %2\n%3").arg(nr + 1)
                                           .arg(QString::fromLocal8Bit(*it))
                                           .arg(*it2);
        QRadioButton *rb = new QRadioButton( text);
        d->selectBoxGroup->addButton(rb, nr);
        d->selectBoxLayout->addWidget(rb);

        // check this one either if it matches the default one, or else is the first one:
        if (rbDef == NULL || *it == defstr )
            rbDef = rb;

        nr++;
    }

    if (rbDef)
        rbDef->setChecked(true);
}
