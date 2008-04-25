/***************************************************** -*- mode:c++; -*- ***
                          kookagallery.h  -  Main view
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

#ifndef KOOKAGALLERY_H
#define KOOKAGALLERY_H

#include <qframe.h>

#include "scanpackager.h"

class QGridLayout;
class QHBox;

class KFileTreeViewItem;

class ImageNameCombo;


class KookaGallery : public QFrame
{
    Q_OBJECT

public:
    enum Layout
    {
        NoRecent,
        RecentAtTop,
        RecentAtBottom
    };

    KookaGallery(QWidget *parent);

    void readSettings();

    ScanPackager *galleryTree() const		{ return (m_galleryTree); }
    KFileTreeViewItem *currentItem() const	{ return (m_galleryTree->currentKFileTreeViewItem()); }
    ImageNameCombo *galleryRecent() const	{ return (m_galleryRecent); }

private:
    void setLayout(KookaGallery::Layout option);

    QGridLayout *m_layout;
    QHBox *m_recentBox;

    ImageNameCombo *m_galleryRecent;
    ScanPackager *m_galleryTree;
};

#endif							// KOOKAGALLERY_H
