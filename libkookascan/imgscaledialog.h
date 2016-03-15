/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 1999-2016 Klaas Freitag <freitag@suse.de>		*
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

#ifndef IMGSCALEDIALOG_H
#define IMGSCALEDIALOG_H

#include "kookascan_export.h"

#include "dialogbase.h"

class QLineEdit;

/* ----------------------------------------------------------------------
 * The ImgScaleDialg is a small dialog to be used by the image canvas. It
 * allows the user to select a zoom factor in percent, either in steps
 * or as a custom value.
 */
class KOOKASCAN_EXPORT ImgScaleDialog : public DialogBase
{
    Q_OBJECT

public:
    explicit ImgScaleDialog(QWidget *parent = NULL, int curr_sel = 100);
    virtual ~ImgScaleDialog()				{}

    int getSelected() const;

signals:
    void customScaleChange(int scale);

protected slots:
    void slotSetSelValue(int val);
    void slotEnableAndFocus(bool b);
    void slotCustomChanged(const QString &text);

private:
    QLineEdit *leCust;
    int selected;
};

#endif							// IMGSCALEDIALOG_H
