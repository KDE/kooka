/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2016 Jonathan Marten <jjm@keelhaul.me.uk>		*
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

#include "dialogstatesaver.h"

#include <qdialog.h>
#include <qdesktopwidget.h>
#include <qevent.h>
#include <qapplication.h>
#include <qdebug.h>

#include <kconfiggroup.h>
#include <ksharedconfig.h>


static bool sSaveSettings = true;


DialogStateSaver::DialogStateSaver(QDialog *pnt)
    : QObject(pnt)
{
    mParent = pnt;
    Q_ASSERT(mParent!=NULL);
    mParent->installEventFilter(this);
    connect(mParent, &QDialog::accepted, this, &DialogStateSaver::saveConfigInternal);
}


DialogStateSaver::~DialogStateSaver()
{
    qDebug() << "done";
}


bool DialogStateSaver::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj==mParent && ev->type()==QEvent::Show)	// only interested in show event
    {
        restoreConfigInternal();			// restore size and config
    }
    return (false);					// always pass the event on
}


void DialogStateSaver::restoreConfigInternal()
{
    if (!sSaveSettings) return;

    QString objName = mParent->objectName();
    qDebug() << "for" << objName << "which is a" << mParent->metaObject()->className();
    if (objName.isEmpty())
    {
        objName = mParent->metaObject()->className();
        qWarning() << "object name not set, using class name" << objName;
    }

    const KConfigGroup grp = KSharedConfig::openConfig(QString(), KConfig::NoCascade)->group(objName);
    qDebug() << "from" << grp.name() << "in" << grp.config()->name();
    this->restoreConfig(mParent, grp);
}


void DialogStateSaver::restoreConfig(QDialog *dialog, const KConfigGroup &grp)
{
    // from KDE4 KDialog::restoreDialogSize()
    int scnum = QApplication::desktop()->screenNumber(dialog->parentWidget());
    QRect desk = QApplication::desktop()->screenGeometry(scnum);
    int width = dialog->sizeHint().width();
    int height = dialog->sizeHint().height();

    width = grp.readEntry(QString::fromLatin1("Width %1").arg(desk.width()), width);
    height = grp.readEntry(QString::fromLatin1("Height %1").arg(desk.height()), height);
    dialog->resize(width, height);
}


void DialogStateSaver::saveConfigInternal() const
{
    if (!sSaveSettings) return;

    QString objName = mParent->objectName();
    if (objName.isEmpty()) objName = mParent->metaObject()->className();

    KConfigGroup grp = KSharedConfig::openConfig(QString(), KConfig::NoCascade)->group(objName);
    qDebug() << "to" << grp.name() << "in" << grp.config()->name();
    this->saveConfig(mParent, grp);
    grp.sync();
}


void DialogStateSaver::saveConfig(QDialog *dialog, KConfigGroup &grp) const
{
    // from KDE4 KDialog::saveDialogSize()
    const int scnum = QApplication::desktop()->screenNumber(dialog->parentWidget());
    QRect desk = QApplication::desktop()->screenGeometry(scnum);
    const QSize sizeToSave = dialog->size();

    grp.writeEntry(QString::fromLatin1("Width %1").arg(desk.width()), sizeToSave.width());
    grp.writeEntry( QString::fromLatin1("Height %1").arg(desk.height()), sizeToSave.height());
}


void DialogStateSaver::setSaveSettingsDefault(bool on)
{
    sSaveSettings = on;
}
