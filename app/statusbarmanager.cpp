/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2013-2016 Jonathan Marten <jjm@keelhaul.me.uk>	*
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

#include "statusbarmanager.h"

#include <qstatusbar.h>
#include <qlabel.h>
#include <qstyle.h>

#include <klocalizedstring.h>
#include <kxmlguiwindow.h>

#include "imagecanvas.h"
#include "previewer.h"
#include "sizeindicator.h"


StatusBarManager::StatusBarManager(KXmlGuiWindow *mainWindow)
    : QObject(mainWindow)
{
    mStatusBar = mainWindow->statusBar();
    mMargin = 2*mStatusBar->style()->pixelMetric(QStyle::PM_LayoutLeftMargin);

    // Messages
    mMessageLabel = new QLabel(i18nc("@info:status", "Ready"));
    mStatusBar->addWidget(mMessageLabel, 1);

    // Image dimensions
    QString s = ImageCanvas::imageInfoString(2000, 2000, 48);
    mImageDimsLabel = new QLabel(s);
    mImageDimsLabel->setAlignment(Qt::AlignHCenter);
    mImageDimsLabel->setToolTip(i18nc("@info:tooltip", "The size of the image being viewed in the gallery"));
    mImageDimsLabel->setMinimumWidth(mImageDimsLabel->sizeHint().width()+mMargin);
    mStatusBar->addPermanentWidget(mImageDimsLabel);

    // Preview dimensions
    s = Previewer::previewInfoString(500.0, 500.0, 1200, 1200);
    mPreviewDimsLabel = new QLabel(s);
    mPreviewDimsLabel->setAlignment(Qt::AlignHCenter);
    mPreviewDimsLabel->setToolTip(i18nc("@info:tooltip", "The size of the selected area that will be scanned"));
    mPreviewDimsLabel->setMinimumWidth(mPreviewDimsLabel->sizeHint().width()+mMargin);
    mStatusBar->addPermanentWidget(mPreviewDimsLabel);

    // Preview file size
    mFileSize = new SizeIndicator(NULL);
    mFileSize->setMaximumWidth(100);
    mFileSize->setFrameStyle(QFrame::NoFrame);
    mFileSize->setToolTip(i18nc("@info:tooltip", "<qt>This is the uncompressed size of the scanned image. "
                                "It tries to warn you if you try to produce too big an image by "
                                "changing its background color."));
    mStatusBar->addPermanentWidget(mFileSize);

    mStatusBar->setSizeGripEnabled(false);
    mStatusBar->show();
}


// In this application, the length of some of the status bar strings
// (e.g. the image/preview dimensions) can vary greatly.  To avoid the
// status bar items annoyingly jumping around when this happens, once one
// of those items has reached a certain size it is not allowed to shrink
// again to less than that.
void StatusBarManager::setLabelText(QLabel *l, const QString &text)
{
    const int oldWidth = l->width();
    l->setText(text);
    const int newWidth = l->sizeHint().width();
    if (newWidth>oldWidth) l->setMinimumWidth(newWidth+mMargin);
}


void StatusBarManager::setStatus(const QString &text, StatusBarManager::Item item)
{
    switch (item)
    {
case StatusBarManager::Message:
        mMessageLabel->setText(text);
        break;

case StatusBarManager::ImageDims:
        setLabelText(mImageDimsLabel, i18nc("@info:status", "Image: %1", text));
        break;

case StatusBarManager::PreviewDims:
        setLabelText(mPreviewDimsLabel, i18nc("@info:status", "Scan: %1", text));
        break;

default:
        break;
    }
}


void StatusBarManager::clearStatus(StatusBarManager::Item item)
{
    setStatus(QString::null, item);
}


void StatusBarManager::setFileSize(long size)
{
    mFileSize->setSizeInByte(size);
}
