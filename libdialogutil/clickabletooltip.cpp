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

#include "clickabletooltip.h"

#include <qmenu.h>
#include <qpainter.h>
#include <qtimer.h>
#include <qguiapplication.h>
#include <qapplication.h>
#include <qscreen.h>
#include <qstyleoption.h>


ClickableToolTip::ClickableToolTip(QWidget *pnt)
    : QLabel(pnt, Qt::Popup|Qt::BypassGraphicsProxyWidget)
{
    // Do not set the window flags to include Qt::Sheet.  Although the
    // documentation gives the impression that this only applies to
    // macOS, it seems to have the effect of inhibiting all mouse events
    // on the tool tip.  Qt::ToolTip means Qt::Popup|Qt::Sheet so this
    // cannot be used, just using Qt::Popup instead.

    setForegroundRole(QPalette::ToolTipText);
    setWordWrap(true);

    mMouseTimer = new QTimer(this);
    mMouseTimer->setInterval(250);
    connect(mMouseTimer, &QTimer::timeout, this, &ClickableToolTip::slotCheckCursor);

    mHideTimer = new QTimer(this);
    mHideTimer->setSingleShot(true);
    connect(mHideTimer, &QTimer::timeout, this, &QWidget::hide);

    mRefWidget = nullptr;
    mRefPos = QPoint();
    mMenuShowing = false;

    setContentsMargins(4, 4, 4, 4);
}


void ClickableToolTip::slotCheckCursor()
{
    // Ignore if the link context menu is visible
    QMenu *menu = findChild<QMenu *>(QString(), Qt::FindDirectChildrenOnly);
    if (menu!=nullptr && menu->isVisible()) return;

    // An arbitrary check for mouse position.  Since we have to be able to move
    // inside the tooltip margins (standard QToolTip hides itself on hover),
    // let's add some margins just for safety.
    QRegion region(geometry().adjusted(-10, -10, 10, 10));

    if (mRefWidget!=nullptr)
    {
        QRect r = mRefWidget->rect();
        r.moveTopLeft(mRefWidget->mapToGlobal(QPoint(0, 0)));
        region |= QRegion(r);
    }
    else
    {
        QRect r(0, 0, 16, 16);
        r.moveCenter(mRefPos);
        region |= QRegion(r, QRegion::Ellipse);
    }

    if (!region.contains(QCursor::pos())) hide();
}


void ClickableToolTip::show()
{
    QLabel::show();
    installEventFilter(this);
}


bool ClickableToolTip::event(QEvent *ev)
{
    // Just for safety...
    if (ev->type()==QEvent::WindowDeactivate) hide();
    return (QLabel::event(ev));
}


bool ClickableToolTip::eventFilter(QObject *obj, QEvent *ev)
{
    // If we detect a mouse button or key press that has not originated
    // from the label, assume that the tooltip should be closed.  Note that
    // widgets that have been just mapped ("shown") might return events for
    // their QWindow instead of the actual QWidget.
    if (obj!=this)
    {
        QEvent::Type type = ev->type();
        if (type==QEvent::MouseButtonPress || type==QEvent::KeyPress) hide();
    }

    return (QLabel::eventFilter(obj, ev));
}


void ClickableToolTip::move(const QPoint &pos)
{
    // Ensure that the style has "polished" the widget (font, palette, etc.)
    ensurePolished();
    // Ensure that the tooltip is shown within the available screen area
    QRect geo = QRect(pos, sizeHint());

    QScreen *screen = QGuiApplication::screenAt(pos);
    if (screen==nullptr) screen = QGuiApplication::primaryScreen();

    const QRect screenGeo = screen->availableGeometry();
    // The screen geometry correction should always consider the top-left corners
    // *last* so that at least their beginning text is always visible (that's
    // why I used pairs of "if" instead of "if/else"); also note that this
    // doesn't take into account right-to-left languages, but that can be
    // accounted for by checking QGuiApplication.layoutDirection()
    if (geo.bottom()>screenGeo.bottom()) geo.moveBottom(screenGeo.bottom());
    if (geo.top()<screenGeo.top()) geo.moveTop(screenGeo.top());
    if (geo.right()>screenGeo.right()) geo.moveRight(screenGeo.right());
    if (geo.left()<screenGeo.left()) geo.moveLeft(screenGeo.left());

    QLabel::move(geo.topLeft());
}


void ClickableToolTip::contextMenuEvent(QContextMenuEvent *ev)
{
    // Check the child QMenu objects before showing the menu, which could
    // potentially hide the label.
    QList<QMenu *> knownChildMenus = findChildren<QMenu *>(QString(), Qt::FindDirectChildrenOnly);
    mMenuShowing = true;
    QLabel::contextMenuEvent(ev);

    QList<QMenu *> newMenus = findChildren<QMenu *>(QString(), Qt::FindDirectChildrenOnly);

    if (knownChildMenus==newMenus) hide();
    else
    {
        for (QMenu *m : newMenus)
        {
            if (!knownChildMenus.contains(m))
            {
                // Hide ourselves as soon as the (new) menus close
                connect(m, &QMenu::aboutToHide, this, &QWidget::hide);
                mMenuShowing = false;
            }
        }
    }
}


void ClickableToolTip::mouseReleaseEvent(QMouseEvent *ev)
{
    // Click events on the link are delivered on button release!
    QLabel::mouseReleaseEvent(ev);
    hide();
}


void ClickableToolTip::hide()
{
    if (!mMenuShowing) QLabel::hide();
}


void ClickableToolTip::hideEvent(QHideEvent *ev)
{
    QLabel::hideEvent(ev);

    removeEventFilter(this);
    if (mRefWidget!=nullptr) mRefWidget->window()->removeEventFilter(this);
    mMouseTimer->stop();
    mHideTimer->stop();
    mRefWidget = nullptr;
    mRefPos = QPoint();
}


void ClickableToolTip::resizeEvent(QResizeEvent *ev)
{
    QLabel::resizeEvent(ev);

    // On some systems the tool tip is not a rectangle, so "mask" the label
    // according to the system defaults.
    QStyleOption opt;
    opt.initFrom(this);
    QStyleHintReturn ret;
    if (style()->styleHint(QStyle::SH_ToolTip_Mask, &opt, this, &ret))
    {
        const QStyleHintReturnMask *ret2 = qstyleoption_cast<const QStyleHintReturnMask *>(&ret);
        setMask(ret2->region);
    }
}


void ClickableToolTip::paintEvent(QPaintEvent *ev)
{
    // We cannot directly draw the label, since a tooltip could have an inner
    // border, so let's draw the "background" before that.
    QPainter p(this);
    QStyleOption opt;
    opt.initFrom(this);
    style()->drawPrimitive(QStyle::PE_PanelTipLabel, &opt, &p, this);

    // Now we paint the label contents
    QLabel::paintEvent(ev);
}


static ClickableToolTip *sInstance = nullptr;

/* static */ ClickableToolTip *ClickableToolTip::showText(const QPoint &pos,
                                        const QString &text,
                                        QWidget *pnt,
                                        const QRect &rect,
                                        int delay)
{
    // This is a method similar to QToolTip.showText().
    // It reuses an existing instance, but also returns the tooltip so that
    // its linkActivated signal can be connected.

    if (sInstance==nullptr)
    {
        if (text.isEmpty()) return (nullptr);
        sInstance = new ClickableToolTip;
    }

    ClickableToolTip *tip = sInstance;
    tip->mMouseTimer->stop();
    tip->mHideTimer->stop();

    // Disconnect all previously connected signals, if any
    disconnect(tip, &QLabel::linkActivated, nullptr, nullptr);

    if (text.isEmpty())
    {
        tip->hide();
        return (nullptr);
    }

    tip->setText(text);
    if (pnt==nullptr) delay = 0;

    const QPoint adjustedPos = pos+QPoint(16, 16);

    // Adjust the tooltip position if necessary (based on arbitrary margins)
    if (!tip->isVisible() || pnt!=tip->mRefWidget ||
        (pnt==nullptr && !tip->mRefPos.isNull() && (tip->mRefPos-adjustedPos).manhattanLength()>10))
    {
        tip->move(adjustedPos);
    }

    // We assume that, if no parent argument is given, the current activeWindow
    // is what we should use as a reference for mouse detection
    tip->mRefWidget = (pnt!=nullptr ? pnt : QApplication::activeWindow());
    connect(tip->mRefWidget, &QObject::destroyed, tip, &QWidget::hide);

    tip->mRefPos = adjustedPos;
    tip->show();
    tip->mMouseTimer->start();
    if (delay!=0) tip->mHideTimer->start(delay);

    return (tip);
}
