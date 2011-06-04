/**************************************************************************
              kookagallery.cpp  -  scan gallery and history
                             -------------------
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This file may be distributed and/or modified under the terms of the    *
 *  GNU General Public License version 2 as published by the Free Software *
 *  Foundation and appearing in the file COPYING included in the           *
 *  packaging of this file.                                                *
 *
 *  As a special exception, permission is given to link this program       *
 *  with any version of the KADMOS ocr/icr engine of reRecognition GmbH,   *
 *  Kreuzlingen and distribute the resulting executable without            *
 *  including the source code for KADMOS in the source distribution.       *
 *
 *  As a special exception, permission is given to link this program       *
 *  with any edition of Qt, and distribute the resulting executable,       *
 *  without including the source code for Qt in the source distribution.   *
 *                                                                         *
 ***************************************************************************/

#include "kookagallery.h"
#include "kookagallery.moc"

#include <qlayout.h>
#include <qlabel.h>

#include <kdebug.h>
#include <klocale.h>
#include <kglobal.h>
#include <khbox.h>

#include "scangallery.h"
#include "galleryhistory.h"
#include "kookapref.h"



KookaGallery::KookaGallery(QWidget *parent)
    : QWidget(parent)
{
    kDebug();
    m_layout = new QGridLayout(this);
    m_layout->setMargin(0);

    m_recentBox = new KHBox(this);
    //m_recentBox->setMargin(KDialog::spacingHint());
    QLabel *lab = new QLabel(i18n("Folder:"),m_recentBox);
    lab->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));

    m_galleryRecent = new GalleryHistory(m_recentBox);
    lab->setBuddy(m_galleryRecent);

    m_galleryTree = new ScanGallery(this);

    readSettings();
}


void KookaGallery::readSettings()
{
    KConfigGroup grp = KGlobal::config()->group(GROUP_GALLERY);

    m_galleryTree->setAllowRename(grp.readEntry(GALLERY_ALLOW_RENAME, false));
    setLayout(static_cast<KookaGallery::Layout>(grp.readEntry(GALLERY_LAYOUT, static_cast<int>(KookaGallery::RecentAtTop))));
}


void KookaGallery::setLayout(KookaGallery::Layout option)
{
    m_layout->removeWidget(m_galleryTree);
    m_layout->removeWidget(m_recentBox);

    switch (option)
    {
case KookaGallery::RecentAtTop:
        m_layout->addWidget(m_recentBox,0,0);
        m_layout->addWidget(m_galleryTree,1,0);
        break;

case KookaGallery::RecentAtBottom:
        m_layout->addWidget(m_galleryTree,0,0);
        m_layout->addWidget(m_recentBox,1,0);
        break;

case KookaGallery::NoRecent:
        m_layout->addWidget(m_galleryTree,0,0);
        break;
    }
}
