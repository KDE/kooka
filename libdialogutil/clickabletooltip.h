/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2024 Jonathan Marten <jjm@keelhaul.me.uk>		*
 *  Based on the original Python version by "musicamante" at		*
 *  https://stackoverflow.com/questions/59902049/			*
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

#ifndef CLICKABLETOOLTIP_H
#define CLICKABLETOOLTIP_H

#include "libdialogutil_export.h"

// class QComboBox;
// class KLineEdit;
// class QPushButton;

#include <qlabel.h>


/**
 * @short A tool tip that can contain clickable hyperlinks.
 *
 * @author Jonathan Marten
 */

class LIBDIALOGUTIL_EXPORT ClickableToolTip : public QLabel
{
    Q_OBJECT

public:
    ClickableToolTip(QWidget *pnt = nullptr);
    virtual ~ClickableToolTip() = default;

    bool eventFilter(QObject *obj, QEvent *ev) override;
    void move(const QPoint &pos);

    static ClickableToolTip *showText(const QPoint &pos,
                                      const QString &text,
                                      QWidget *pnt = nullptr,
                                      const QRect &rect = QRect(),
                                      int delay = 0);

public slots:
    void show();
    void hide();

private slots:
    void slotCheckCursor();

protected:
    bool event(QEvent *ev) override;
    void contextMenuEvent(QContextMenuEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
    void hideEvent(QHideEvent *ev) override;
    void resizeEvent(QResizeEvent *ev) override;
    void paintEvent(QPaintEvent *ev) override;


private:
    QWidget *mRefWidget;
    QPoint mRefPos;
    QTimer *mMouseTimer;
    QTimer *mHideTimer;
    bool mMenuShowing;
};

#endif							// CLICKABLETOOLTIP_H
