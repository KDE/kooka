/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2000-2016 Klaas Freitag <freitag@suse.de>		*
 *                          Jonathan Marten <jjm@keelhaul.me.uk>	*
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

#include "kookapref.h"

#include <qlayout.h>
#include <qicon.h>
#include <qpushbutton.h>

#include <klocalizedstring.h>

#include "prefspages.h"
#include "kookasettings.h"
#include "dialogstatesaver.h"
#include "kooka_logging.h"


KookaPref::KookaPref(QWidget *parent)
    : KPageDialog(parent)
{
    setObjectName("KookaPref");

    setModal(true);
    setStandardButtons(QDialogButtonBox::Help | QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults);
    buttonBox()->button(QDialogButtonBox::Ok)->setDefault(true);
    buttonBox()->button(QDialogButtonBox::RestoreDefaults)->setIcon(QIcon::fromTheme("edit-undo"));
    setWindowTitle(i18n("Preferences"));

    createPage(new KookaGeneralPage(this), i18n("General"), i18n("General Options"), "configure");
    createPage(new KookaStartupPage(this), i18n("Startup"), i18n("Startup Options"), "system-run");
    createPage(new KookaSavingPage(this), i18n("Image Saving"), i18n("Image Saving Options"), "document-save");
    createPage(new KookaThumbnailPage(this), i18n("Gallery & Thumbnails"), i18n("Image Gallery and Thumbnail View"), "view-list-icons");
    createPage(new KookaOcrPage(this), i18n("OCR"), i18n("Optical Character Recognition"), "ocr");

    connect(buttonBox()->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &KookaPref::slotSaveSettings);
    connect(buttonBox()->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &KookaPref::slotSaveSettings);
    connect(buttonBox()->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this, &KookaPref::slotSetDefaults);

    setMinimumSize(670, 380);
    new DialogStateSaver(this);
}

int KookaPref::createPage(KookaPrefsPage *page,
                          const QString &name,
                          const QString &header,
                          const char *icon)
{
    QVBoxLayout *top = static_cast<QVBoxLayout *>(page->layout());
    if (top != nullptr) {
        top->addStretch(1);
    }

    KPageWidgetItem *item = addPage(page, name);
    item->setHeader(header);
    item->setIcon(QIcon::fromTheme(icon));

    int idx = mPages.count();				// index of new item
    mPages.append(item);
    return (idx);					// index of item added
}

void KookaPref::slotSaveSettings()
{
    for (int i = 0; i < mPages.size(); ++i) {
        KookaPrefsPage *page = static_cast<KookaPrefsPage *>(mPages[i]->widget());
        page->saveSettings();
    }

    KSharedConfig::openConfig()->sync();
    emit dataSaved();
}

void KookaPref::slotSetDefaults()
{
    for (int i = 0; i < mPages.size(); ++i) {
        KookaPrefsPage *page = static_cast<KookaPrefsPage *>(mPages[i]->widget());
        page->defaultSettings();
    }
}

void KookaPref::showPageIndex(int page)
{
    if (page >= 0 && page < mPages.size()) {
        setCurrentPage(mPages[page]);
    }
}

int KookaPref::currentPageIndex()
{
    return (mPages.indexOf(currentPage()));
}
