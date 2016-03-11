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

#include "kookagallery.h"

#include <qlayout.h>
#include <qlabel.h>
#include <qdebug.h>

#include <klocalizedstring.h>
#include <khbox.h>

#include "scangallery.h"
#include "galleryhistory.h"
#include "kookasettings.h"


KookaGallery::KookaGallery(QWidget *parent)
    : QWidget(parent)
{
    //qDebug();
    m_layout = new QGridLayout(this);
    m_layout->setMargin(0);

    m_recentBox = new KHBox(this);
    //m_recentBox->setMargin(KDialog::spacingHint());
    QLabel *lab = new QLabel(i18n("Folder:"), m_recentBox);
    lab->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

    m_galleryRecent = new GalleryHistory(m_recentBox);
    lab->setBuddy(m_galleryRecent);

    m_galleryTree = new ScanGallery(this);

    readSettings();
}

void KookaGallery::readSettings()
{
    m_galleryTree->setAllowRename(KookaSettings::galleryAllowRename());
    setLayout(static_cast<KookaGallery::Layout>(KookaSettings::galleryLayout()));
}

ScanGallery *KookaGallery::galleryTree() const
{
    return (m_galleryTree);
}

GalleryHistory *KookaGallery::galleryRecent() const
{
    return (m_galleryRecent);
}

void KookaGallery::setLayout(KookaGallery::Layout option)
{
    m_layout->removeWidget(m_galleryTree);
    m_layout->removeWidget(m_recentBox);

    switch (option) {
    case KookaGallery::RecentAtTop:
        m_layout->addWidget(m_recentBox, 0, 0);
        m_layout->addWidget(m_galleryTree, 1, 0);
        break;

    case KookaGallery::RecentAtBottom:
        m_layout->addWidget(m_galleryTree, 0, 0);
        m_layout->addWidget(m_recentBox, 1, 0);
        break;

    case KookaGallery::NoRecent:
        m_layout->addWidget(m_galleryTree, 0, 0);
        break;
    }
}
