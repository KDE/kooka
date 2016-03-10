/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2002-2016 Klaas Freitag <freitag@suse.de>		*
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

#ifndef THUMBVIEW_H
#define THUMBVIEW_H

#include <qmap.h>
#include <qurl.h>

#include <kdiroperator.h>
#include <kiconloader.h>

// TODO: use KConfigXT
//  KConfig group definitions
#define THUMB_GROUP     "thumbnailView"
#define THUMB_PREVIEW_SIZE  "previewSize"
#define THUMB_CUSTOM_BGND   "customBackground"
#define THUMB_BG_WALLPAPER  "BackGroundTile"
#define THUMB_STD_TILE_IMG  "kooka/pics/thumbviewtile.png"

class QMenu;
class KFileItem;
class KActionMenu;
class KToggleAction;

class ThumbView : public KDirOperator
{
    Q_OBJECT

public:
    ThumbView(QWidget *parent);
    ~ThumbView();

    QMenu *contextMenu() const
    {
        return (mContextMenu);
    }
    bool readSettings();

    static QString standardBackground();
    static QString sizeName(KIconLoader::StdSizes size);

public slots:
    void slotImageDeleted(const KFileItem *kfi);
    void slotImageChanged(const KFileItem *kfi);
    void slotImageRenamed(const KFileItem *kfi, const QString &newName);
    void slotHighlightItem(const QUrl &url, bool isDir);

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
    void itemHighlighted(const QUrl &url);
    void itemActivated(const QUrl &url);

private:
    void setBackground();

    QMenu *mContextMenu;
    bool m_firstMenu;
    KActionMenu *m_sizeMenu;
    QMap<KIconLoader::StdSizes, KToggleAction *> m_sizeMap;

    KIconLoader::StdSizes m_thumbSize;
    QString m_bgImg;
    bool m_customBg;

    QUrl m_lastSelected;
    QUrl m_toSelect;
    QUrl m_toChangeTo;
};

#endif                          // THUMBVIEW_H
