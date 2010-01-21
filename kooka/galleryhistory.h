/***************************************************** -*- mode:c++; -*- ***
                          GalleryHistory.h - combobox for image names
                             -------------------
    begin                : Tue Nov 13 2001
    copyright            : (C) 2001 by Klaas Freitag
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

#ifndef GALLERYHISTORY_H
#define GALLERYHISTORY_H

#include <kcombobox.h>

/**
  *@author Klaas Freitag
*/

class FileTreeBranch;


class GalleryHistory : public KComboBox
{
    Q_OBJECT

public:
    GalleryHistory(QWidget *parent);

public slots:
    void slotPathChanged(const FileTreeBranch *branch, const QString &relPath);
    void slotPathRemoved(const FileTreeBranch *branch, const QString &relPath);

signals:
    void pathSelected(const QString &branchName, const QString &relPath);

protected slots:
    void slotActivated(int idx);
};


#endif							// GALLERYHISTORY_H
