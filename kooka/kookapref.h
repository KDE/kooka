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

#ifndef KOOKAPREF_H
#define KOOKAPREF_H

#include <kpagedialog.h>

class KookaPrefsPage;


class KookaPref : public KPageDialog
{
    Q_OBJECT

public:
    KookaPref(QWidget *parent = Q_NULLPTR);

    int createPage(KookaPrefsPage *page, const QString &name,
                   const QString &header, const char *icon);

    static QString tryFindGocr();
    static QString tryFindOcrad();

    void showPageIndex(int page);
    int currentPageIndex();

    /**
     * Static function that returns the image gallery base dir.
     */
    // TODO: this should return a KUrl
    static QString galleryRoot();

protected slots:
    void slotSaveSettings();
    void slotSetDefaults();

signals:
    void dataSaved();

private:
    QVector<KPageWidgetItem *> mPages;

    static QString findGalleryRoot();

    static QString sGalleryRoot;
};

#endif							// KOOKAPREF_H
