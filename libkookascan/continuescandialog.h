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

#ifndef CONTINUESCANDIALOG_H
#define CONTINUESCANDIALOG_H

#include <qdialog.h>
#include <qdialogbuttonbox.h>

#include <klocalizedstring.h>

#include "multiscanoptions.h"

#include "kookascan_export.h"

class QPushButton;
class QTimer;
class QLabel;
class QCloseEvent;


class KOOKASCAN_EXPORT ContinueScanDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ContinueScanDialog(const MultiScanOptions &options, QWidget *pnt = nullptr);
    virtual ~ContinueScanDialog() = default;

    // The return type of this function must be 'int' so that it can
    // override QDialog::exec() without warnings, even though it is
    // really a QDialog::DialogCode.
    int exec() override;

protected:
    void closeEvent(QCloseEvent *ev) override;

private slots:
    void slotTimer();
    void slotPause();

private:
    QDialogButtonBox *mButtonBox;
    QPushButton *mScanButton;
    QPushButton *mPauseButton;

    QTimer *mTimer;
    int mTimeout;

    QDialogButtonBox::StandardButton mResult;

    QLabel *mTextLabel;
    KLocalizedString mMessageText;
};

#endif							// CONTINUESCANDIALOG_H
