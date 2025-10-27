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

#include <qwidget.h>

#include "scanimage.h"

#include "kookacore_export.h"

class QLabel;
class QButtonGroup;
class QCheckBox;
class QComboBox;
class QLineEdit;
class QSpinBox;
class QTimer;
class QPrintDialog;

class KookaPrint;

//////////////////////////////////////////////////////////////////////////
//  ImgPrintTab								//
//////////////////////////////////////////////////////////////////////////

class KOOKACORE_EXPORT ImgPrintTab : public QWidget
{
    Q_OBJECT

public:
    explicit ImgPrintTab(ScanImage::Ptr img, KookaPrint *prt, QWidget *pnt = nullptr);
    virtual ~ImgPrintTab() = default;

    QString checkValid() const;

public slots:
    void updatePrintParameters();

protected slots:
    void slotScaleChanged(int id);
    void slotCustomWidthChanged(int);
    void slotCustomHeightChanged(int);
    void slotAdjustCustomSize();

private:
    void initOptions();

private:
    QButtonGroup  *m_scaleRadios;

    QSpinBox *m_sizeW;
    QSpinBox *m_sizeH;
    QSpinBox *m_dpi;

    QCheckBox *m_psDraft;
    QCheckBox *m_ratio;
    QComboBox *m_cutsCombo;

    QLineEdit *m_screenRes;

    QLabel *mImageSize;
    QLabel *mPrintArea;
    QLabel *mImageArea;
    QLabel *mPrintPages;

    ScanImage::Ptr m_image;
    QTimer *mUpdateTimer;
    KookaPrint *mPrinter;
};

//////////////////////////////////////////////////////////////////////////
//  ImgPrintDialog							//
//////////////////////////////////////////////////////////////////////////

class KOOKACORE_EXPORT ImgPrintDialog
{
public:
    explicit ImgPrintDialog(ScanImage::Ptr img, KookaPrint *prt, QWidget *pnt = nullptr);
    ~ImgPrintDialog() = default;

    bool exec();

private:
    ImgPrintTab *mImgTab;
    QPrintDialog *mDialog;
};

#endif							// IMGPRINTDIALOG_H
