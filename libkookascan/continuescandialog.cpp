/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2024  Jonathan Marten <jjm@keelhaul.me.uk>		*
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

// Inspired by https://gist.github.com/RobertHabrich/d7c4d81f47098b4e524d
// See also https://stackoverflow.com/questions/18687084/

#include "continuescandialog.h"

#include <qpushbutton.h>
#include <qapplication.h>
#include <qstyle.h>
#include <qtimer.h>
#include <qlabel.h>

#include <kmessagebox.h>

#include "libkookascan_logging.h"


ContinueScanDialog::ContinueScanDialog(int timeout, QWidget *pnt)
    : QDialog(pnt)
{
    qCDebug(LIBKOOKASCAN_LOG) << "timeout" << timeout;

    mTimeout = timeout;
    mTextLabel = nullptr;
    mPauseButton = nullptr;

    setModal(true);
    setWindowTitle(i18n("Next Page"));

    QString buttonText;
    mButtonBox = new QDialogButtonBox(this);
    if (timeout==0)					// not timed so static
    {
        buttonText = i18n("Scan Next");
        mMessageText = kxi18nc("@info",
                               "<emphasis strong=\"1\">Ready to scan the next page</emphasis>%1..."
                               "<nl/><nl/>"
                               "Prepare the next page and then click <interface>Scan&nbsp;Next</interface> to scan it,"
                               "<nl/>"
                               "or click <interface>Finish</interface> to end the scan batch "
                               "or <interface>Cancel</interface> to cancel the entire scan.");
        mTimer = nullptr;
    }
    else						// timed with countdown
    {
        buttonText = i18n("Scan Now");
        mMessageText = kxi18nc("@info",
                               "<emphasis strong=\"1\">Waiting to scan the next page</emphasis>%1..."
                               "<nl/><nl/>"
                               "Prepare the next page and then wait, or click <interface>Scan&nbsp;Now</interface> to scan it immediately"
                               "<nl/>"
                               "or click <interface>Finish</interface> to end the scan batch "
                               "or <interface>Cancel</interface> to cancel the entire scan.");

        mTimer = new QTimer(this);
        mTimer->setInterval(1000);
        mTimer->setSingleShot(false);
        mTimer->callOnTimeout(this, &ContinueScanDialog::slotTimer);

        mPauseButton = new QPushButton(i18n("Pause"), mButtonBox);
        mPauseButton->setIcon(QIcon::fromTheme("media-playback-pause"));
        connect(mPauseButton, &QAbstractButton::clicked, this, &ContinueScanDialog::slotPause);
    }

    // The button roles set here are not used by this dialogue, but are
    // chosen so that the button display order (under Plasma anyway), and
    // the button that becomes the default if Pause is clicked, are as
    // intended.
    QPushButton *but = new QPushButton(buttonText, mButtonBox);
    but->setIcon(KStandardGuiItem::cont().icon());
    but->setAutoDefault(true);
    connect(but, &QAbstractButton::clicked, this, [this]() { mResult = QDialogButtonBox::Ok; });
    mButtonBox->addButton(but, QDialogButtonBox::AcceptRole);
    mScanButton = but;

    if (mPauseButton!=nullptr) mButtonBox->addButton(mPauseButton, QDialogButtonBox::ActionRole);

    but = new QPushButton(i18n("Finish"), mButtonBox);
    but->setIcon(KStandardGuiItem::ok().icon());
    connect(but, &QAbstractButton::clicked, this, [this]() { mResult = QDialogButtonBox::Close; });
    mButtonBox->addButton(but, QDialogButtonBox::ApplyRole);

    but = new QPushButton("", mButtonBox);
    KStandardGuiItem::assign(but, KStandardGuiItem::Cancel);
    connect(but, &QAbstractButton::clicked, this, [this]() { mResult = QDialogButtonBox::Cancel; });
    mButtonBox->addButton(but, QDialogButtonBox::RejectRole);

    KMessageBox::createKMessageBox(this,				// dialog
                                   mButtonBox,				// buttons
                                   QApplication::style()->standardIcon(QStyle::SP_MessageBoxQuestion, nullptr, this),
                                   mMessageText.subs("").toString(),	// text
                                   QStringList(),			// strlist
                                   QString(),				// ask
                                   nullptr,				// checkboxReturn
                                   KMessageBox::NoExec);		// options

    // The message box created with the above options contains two QLabel's,
    // one for the icon pixmap and one for the message text.  We want the
    // latter, so detect it by checking that its current text() is not null.
    const QList<QLabel *> labels = findChildren<QLabel *>();
    for (QLabel *label : std::as_const(labels))
    {
        if (!label->text().isEmpty())
        {
            mTextLabel = label;
            break;
        }
    }
    if (mTextLabel==nullptr) qCWarning(LIBKOOKASCAN_LOG) << "Cannot find message text label";
}


int ContinueScanDialog::exec()
{
    mResult = QDialogButtonBox::NoButton;

    if (!isVisible()) show();
    if (mTimer!=nullptr)				// timed with countdown
    {
        slotTimer();					// show countdown message immediately
        mTimer->start();
    }

    while (mResult==QDialogButtonBox::NoButton) QCoreApplication::processEvents();

    hide();
    deleteLater();					// safer than setting Qt::WA_DeleteOnClose

    qCDebug(LIBKOOKASCAN_LOG) << "returning" << mResult;
    return (mResult);
}


void ContinueScanDialog::closeEvent(QCloseEvent *ev)
{
    if (mTimer!=nullptr) mTimer->stop();
    mResult = QDialogButtonBox::Cancel;
}


void ContinueScanDialog::slotTimer()
{
    if (mTimeout>=1)					// still more time remaining
    {
        if (mTextLabel!=nullptr)			// update countdown text
        {
            mTextLabel->setText(mMessageText.subs(i18np(" (%1 second)", " (%1 seconds)", mTimeout)).toString());
        }
        --mTimeout;
    }
    else						// timeout now expired
    {
        mTimer->stop();
        mScanButton->animateClick();			// activate the "Scan" button
    }
}


void ContinueScanDialog::slotPause()
{
    mTimer->stop();
    mPauseButton->setEnabled(false);

    if (mTextLabel!=nullptr)				// update countdown text
    {
        mTextLabel->setText(mMessageText.subs(i18n(" (Paused)")).toString());
    }
}
