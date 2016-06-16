/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2003-2016 Klaas Freitag <freitag@suse.de>		*
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

#ifndef IMGPRINTDIALOG_H
#define IMGPRINTDIALOG_H

#include <qmap.h>
#include <qcheckbox.h>
#ifndef KDE3
#include <qwidget.h>
#endif

#ifdef KDE3
#include <kdeprint/kprintdialogpage.h>
#endif

class KookaImage;
class KookaPrint;

class QLabel;
class QButtonGroup;
class QCheckBox;
class QSpinBox;
class QTimer;

class KookaImage;
class KookaPrint;


#ifdef KDE3
class ImgPrintDialog : public KPrintDialogPage
#else
class ImgPrintDialog : public QWidget
#endif
{
    Q_OBJECT

public:
    ImgPrintDialog(const KookaImage *img, KookaPrint *prt, QWidget *pnt = NULL);
    virtual ~ImgPrintDialog() = default;

    QString checkValid() const;

public slots:
    void updatePrintParameters();

protected:
    KookaPrint *printer() const				{ return (mPrinter); }

protected slots:
    void slotScaleChanged(int id);
    void slotCustomWidthChanged(int);
    void slotCustomHeightChanged(int);

private:
    void initOptions();

private:
    QButtonGroup  *m_scaleRadios;

    QSpinBox *m_sizeW;
    QSpinBox *m_sizeH;
    QSpinBox *m_dpi;

    QCheckBox    *m_psDraft;
    QCheckBox    *m_ratio;

    QLabel     *m_screenRes;

    const KookaImage *m_image;
    int mScreenDpi;
    QTimer *mUpdateTimer;
    KookaPrint *mPrinter;
};

#endif							// IMGPRINTDIALOG_H
