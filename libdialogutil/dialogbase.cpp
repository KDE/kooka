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

#include "dialogbase.h"

#include <qlayout.h>
#include <qdesktopwidget.h>
#include <qdebug.h>
#include <qframe.h>
#include <qstyle.h>
#include <qpushbutton.h>
#include <qapplication.h>

#include <kguiitem.h>

#include "dialogstatesaver.h"


static bool sButtonSeparatorShown = true;


DialogBase::DialogBase(QWidget *pnt)
    : QDialog(pnt)
{
    qDebug();

    setModal(true);					// convenience, can reset if necessary

    mMainWidget = NULL;					// caller not provided yet
    mStateSaver = new DialogStateSaver(this);		// use our own as default

    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    connect(mButtonBox, &QDialogButtonBox::accepted, this, &DialogBase::accept);
    connect(mButtonBox, &QDialogButtonBox::rejected, this, &DialogBase::reject);

    mButtonSeparatorShown = sButtonSeparatorShown;
}


DialogBase::~DialogBase()
{
    qDebug() << "done";
}


void DialogBase::showEvent(QShowEvent *ev)
{
    if (layout()==NULL)					// layout not yet set up
    {
        qDebug() << "setup layout";
        QVBoxLayout *mainLayout = new QVBoxLayout;
        setLayout(mainLayout);

        if (mMainWidget==NULL)
        {
            qWarning() << "No main widget set for" << objectName();
            mMainWidget = new QWidget(this);
        }

        mainLayout->addWidget(mMainWidget);
        mainLayout->setStretchFactor(mMainWidget, 1);

        if (mButtonSeparatorShown)
        {
            QFrame *frame = new QFrame(this);
            frame->setFrameStyle(QFrame::Sunken|QFrame::HLine);
            mainLayout->addWidget(frame);
        }

        mainLayout->addWidget(mButtonBox);
    }

    QDialog::showEvent(ev);				// show the dialogue
}


void DialogBase::setButtons(QDialogButtonBox::StandardButtons buttons)
{
    qDebug() << buttons;
    mButtonBox->setStandardButtons(buttons);

    if (buttons & QDialogButtonBox::Ok)
    {
        qDebug() << "setup OK button";
        QPushButton *okButton = mButtonBox->button(QDialogButtonBox::Ok);
        okButton->setDefault(true);
        okButton->setShortcut(Qt::CTRL|Qt::Key_Return);
    }

// set F1 shortcut for Help?

}


void DialogBase::setButtonEnabled(QDialogButtonBox::StandardButton button, bool state)
{
    QPushButton *but = mButtonBox->button(button);
    if (but!=NULL) but->setEnabled(state);
}


void DialogBase::setButtonText(QDialogButtonBox::StandardButton button, const QString &text)
{
    QPushButton *but = mButtonBox->button(button);
    if (but!=NULL) but->setText(text);
}

void DialogBase::setButtonIcon(QDialogButtonBox::StandardButton button, const QIcon &icon)
{
    QPushButton *but = mButtonBox->button(button);
    if (but!=NULL) but->setIcon(icon);
}

void DialogBase::setButtonGuiItem(QDialogButtonBox::StandardButton button, const KGuiItem &guiItem)
{
    QPushButton *but = mButtonBox->button(button);
    if (but!=NULL) KGuiItem::assign(but, guiItem);
}


void DialogBase::setButtonSeparatorShownDefault(bool on)
{
    sButtonSeparatorShown = on;
}


int DialogBase::spacingHint()
{
    // from KDE4 KDialog::spacingHint()
    return (QApplication::style()->pixelMetric(QStyle::PM_DefaultLayoutSpacing));
}


void DialogBase::setStateSaver(DialogStateSaver *saver)
{
    if (mStateSaver!=NULL) delete mStateSaver;
    mStateSaver = saver;
}
