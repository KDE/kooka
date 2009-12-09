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

#include <qmap.h>

#include <kdiroperator.h>
#include <kiconloader.h>
#include <kurl.h>

//  KConfig group definitions
#define THUMB_GROUP		"thumbnailView"
#define THUMB_PREVIEW_SIZE	"previewSize"
#define THUMB_BG_WALLPAPER	"BackGroundTile"
#define THUMB_STD_TILE_IMG	"kooka/pics/thumbviewtile.png"

class KAction;
class KFileItem;
class KMenu;
class KActionMenu;
class KToggleAction;


class ThumbView : public KDirOperator
{
   Q_OBJECT

public:
    ThumbView(QWidget *parent);
    ~ThumbView();

    KMenu *contextMenu() const { return (mContextMenu); }
    bool readSettings();

    static QString standardBackground();
    static QString sizeName(KIconLoader::StdSizes size);

public slots:
    void slotImageDeleted(const KFileItem *kfi);
    void slotImageChanged(const KFileItem *kfi);
    void slotImageRenamed(const KFileItem *kfi, const QString &newName);
    void slotHighlightItem(const KUrl &url, bool isDir);

protected:
    void saveConfig();

protected slots:
    void slotContextMenu(const QPoint &pos);
    void slotFileSelected(const KFileItem &kfi);
    void slotFileHighlighted(const KFileItem &kfi);
    void slotFinishedLoading();
    void slotEnsureVisible();
    void slotSetSize(int size);

signals:
    void itemHighlighted(const KUrl &url);
    void itemActivated(const KUrl &url);

private:
    void setBackground();

    KMenu *mContextMenu;
    bool m_firstMenu;
    KActionMenu *m_sizeMenu;
    QMap<KIconLoader::StdSizes, KToggleAction *> m_sizeMap;

    KIconLoader::StdSizes m_thumbSize;
    QString m_bgImg;

    KUrl m_lastSelected;
    KUrl m_toSelect;
    KUrl m_toChangeTo;
};


#endif							// THUMBVIEW_H
