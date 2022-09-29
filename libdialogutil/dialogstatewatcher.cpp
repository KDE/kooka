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

#include "dialogstatewatcher.h"

#include <qdialog.h>
#include <qevent.h>
#include <qapplication.h>
#include <qabstractbutton.h>

#include "dialogstatesaver.h"
#include "libdialogutil_logging.h"


DialogStateWatcher::DialogStateWatcher(QDialog *pnt)
    : QObject(pnt)
{
    Q_ASSERT(pnt!=nullptr);
    mParent = pnt;
    mParent->installEventFilter(this);
    connect(mParent, &QDialog::accepted, this, &DialogStateWatcher::saveConfigInternal);

    mStateSaver = new DialogStateSaver(mParent);	// use our own as default
    mHaveOwnSaver = true;				// note that we created it
}


void DialogStateWatcher::setSaveOnButton(QAbstractButton *but)
{
    qCDebug(LIBDIALOGUTIL_LOG) << "button" << but->text();
    connect(but, &QAbstractButton::clicked, this, &DialogStateWatcher::saveConfigInternal);
}


bool DialogStateWatcher::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj==mParent && ev->type()==QEvent::Show)	// only interested in show event
    {
        restoreConfigInternal();			// restore size and config
    }
    return (false);					// always pass the event on
}


void DialogStateWatcher::restoreConfigInternal()
{
    if (mStateSaver==nullptr) return;			// no saver set or provided
    mStateSaver->restoreConfig();
}


void DialogStateWatcher::saveConfigInternal() const
{
    if (mStateSaver==nullptr) return;			// no saver set or provided
    mStateSaver->saveConfig();
}


void DialogStateWatcher::setStateSaver(DialogStateSaver *saver)
{
    // We only delete the existing saver if we created it.
    if (mStateSaver!=nullptr && mHaveOwnSaver) delete mStateSaver;

    mStateSaver = saver;				// set the new one
    mHaveOwnSaver = false;				// note that it's not ours
}
