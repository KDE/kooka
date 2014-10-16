/* This file is part of the KDE Project
   Copyright (C) 1999 Klaas Freitag <freitag@suse.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef IMGSCALEDIALOG_H
#define IMGSCALEDIALOG_H

#include "libkscanexport_export.h"

#include <kdialog.h>

class KLineEdit;

/* ----------------------------------------------------------------------
 * The ImgScaleDialg is a small dialog to be used by the image canvas. It
 * allows the user to select a zoom factor in percent, either in steps
 * or as a custom value.
 */
class LIBKSCANEXPORT_EXPORT ImgScaleDialog : public KDialog
{
    Q_OBJECT

public:
    ImgScaleDialog(QWidget *parent, int curr_sel = 100);

    int getSelected() const;

signals:
    void customScaleChange(int scale);

protected slots:
    void slotSetSelValue(int val);
    void slotEnableAndFocus(bool b);
    void slotCustomChanged(const QString &text);

private:
    KLineEdit *leCust;
    int selected;

    class ImgScaleDialogPrivate;
    ImgScaleDialogPrivate *d;
};

#endif                          // IMGSCALEDIALOG_H
