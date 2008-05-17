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

#include <kdebug.h>
#include <kdialog.h>
#include <klocale.h>
#include <kglobal.h>

#include <qlayout.h>
#include <qhbox.h>
#include <qlabel.h>

#include "scanpackager.h"
#include "imagenamecombo.h"
#include "kookapref.h"

#include "kookagallery.h"
#include "kookagallery.moc"


KookaGallery::KookaGallery(QWidget *parent)
    : QFrame(parent)
{
    kdDebug(28000) << k_funcinfo << endl;
    m_layout = new QGridLayout(this,2,1);

    m_recentBox = new QHBox(this);
    m_recentBox->setMargin(KDialog::spacingHint());
    QLabel *lab = new QLabel(i18n("Folder: "),m_recentBox);
    lab->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));

    m_galleryRecent = new ImageNameCombo(m_recentBox);
    lab->setBuddy(m_galleryRecent);

    m_galleryTree = new ScanPackager(this);

    readSettings();
}


void KookaGallery::readSettings()
{
    KConfig *konf = KGlobal::config();
    konf->setGroup(GROUP_GALLERY);

    m_galleryTree->setAllowRename(konf->readBoolEntry(GALLERY_ALLOW_RENAME,false));
    setLayout(static_cast<KookaGallery::Layout>(konf->readNumEntry(GALLERY_LAYOUT,KookaGallery::RecentAtTop)));
}


void KookaGallery::setLayout(KookaGallery::Layout option)
{
    m_layout->remove(m_galleryTree);
    m_layout->remove(m_recentBox);

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