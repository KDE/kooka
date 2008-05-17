/***************************************************** -*- mode:c++; -*- ***
               thumbview.h  - Class to display thumbnailed images
                             -------------------
    begin                : Tue Apr 18 2002
    copyright            : (C) 2002 by Klaas Freitag
    email                : freitag@suse.de

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

#ifndef THUMBVIEW_H
#define THUMBVIEW_H

#include <qvbox.h>
#include <qmap.h>

#include <kicontheme.h>
#include <kurl.h>

//  KConfig group definitions
#define THUMB_GROUP		"thumbnailView"
#define THUMB_PREVIEW_SIZE	"previewSize"
#define THUMB_BG_WALLPAPER	"BackGroundTile"
#define THUMB_STD_TILE_IMG	"kooka/pics/thumbviewtile.png"

class KFileItem;
class KFileIconView;
class KFileTreeViewItem;
class KActionMenu;
class KToggleAction;

class QPopupMenu;

class ThumbViewDirOperator;


class ThumbView : public QVBox
{
   Q_OBJECT

public:
    ThumbView(QWidget *parent,const char *name = NULL);
    ~ThumbView();

    QPopupMenu *contextMenu() const;
    bool readSettings();

    static QString standardBackground();
    static QString sizeName(KIcon::StdSizes size);

public slots:
    void slotSetSize(int size);
    void slotImageDeleted(const KFileItem *kfi);
    void slotImageChanged(const KFileItem *kfi);
    void slotImageRenamed(const KFileTreeViewItem *item,const QString &newName);
    void slotSelectImage(const KFileTreeViewItem *item);

protected:
    void saveConfig();

protected slots:
    void slotAboutToShowMenu();
    void slotFileSelected(const KFileItem *kfi);
    void slotFinishedLoading();

signals:
    void selectFromThumbnail(const KURL &url);

private:
    void setBackground();

    KActionMenu *m_sizeMenu;
    bool m_firstMenu;
    QMap<KIcon::StdSizes,KToggleAction *> m_sizeMap;

    KIcon::StdSizes m_thumbSize;
    QString m_bgImg;

    ThumbViewDirOperator *m_dirop;
    KFileIconView *m_fileview;
    KURL m_lastSelected;
    QString m_toSelect;
};

#endif							// THUMBVIEW_H
