/* This file is part of the KDE Project         -*- mode:c++; -*-
   Copyright (C) 2010 Jonathan Marten <jjm@keelhaul.me.uk>

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

#ifndef SCANPARAMSPAGE_H
#define SCANPARAMSPAGE_H

#include <qwidget.h>

#include "kookascan_export.h"

//  Limits for the scan option and label, to keep them to reasonable sizes
#define MAX_CONTROL_WIDTH       300
#define MAX_LABEL_WIDTH         150
#define MIN_LABEL_WIDTH         50

class QLabel;
class QGridLayout;


/**
 * This class represents a single options page (Basic, Other or Advanced)
 * within the scan parameter GUI.
 *
 * @author Jonathan Marten
 */

class KOOKASCAN_EXPORT ScanParamsPage : public QWidget
{
    Q_OBJECT

public:
    /**
     * Create a new options page.
     *
     * @param parent  The parent widget
     * @param name    Qt object name
     */
    explicit ScanParamsPage(QWidget *parent, const char *name = nullptr);

    /**
     * Destructor.
     */
    virtual ~ScanParamsPage() = default;

    /**
     * Add a standard parameter row to the page.
     *
     * @param lab    Label for the parameter, normally from @c KScanOption::getLabel(),
     *               or @c nullptr for no label.
     * @param wid    Scan control widget, normally from @c KScanOption::widget().
     * @param unit   SANE unit label, normally from @c KScanOption::getUnit(), or
     *               @c nullptr for no unit label.
     * @param align  Special alignment flags for layout row.

     * @note The layout coloumns are used as follows:  the @p lab label goes in
     * column 0, left aligned.  Column 1 is a spacer.  If the @p unit label is
     * present then it goes in column 3 and the @p wid widget in column 2.  If
     * there is no @p unit label then the @p wid widget spans columns 2 and 3.
     */
    void addRow(QLabel *lab, QWidget *wid, QLabel *unit = nullptr, Qt::Alignment align = Qt::Alignment());

    /**
     * Add a standard parameter row to the page.
     *
     * @param lab    Label text for the parameter, or a null string for no label.
     * @param wid    Scan control widget, normally from @c KScanOption::widget().
     * @param unit   SANE unit label, normally from @c KScanOption::getUnit(), or
     *               @c nullptr for no unit label.
     * @param align  Special alignment flags for layout row.

     * @see the previous overload for column and alignment information.
     */
    void addRow(const QString &lab, QWidget *wid, QLabel *unit = nullptr, Qt::Alignment align = Qt::Alignment());

    /**
     * Add a full width widget to the page.
     *
     * @param wid The widget to add.
     */
    void addRow(QWidget *wid);

    /**
     * Finish off the layout by adding a stretch widget (so that all the created
     * parameters are aligned to the top), and check whether any widgets have
     * been added.
     *
     * @return @c true if any parameter widgets are present
     */
    bool lastRow();

    /**
     * Add a group separator to the layout.  Because of the widget sorting there
     * may be nothing following, or another group, so the group is retained as
     * "pending" and added just before the next row.
     *
     * @param wid The widget to add.
     */
    void addGroup(QWidget *wid);

    /**
     * Retrieve the current layout row, which is the next row that
     * will be filled by either of the @c addRow() functions.
     *
     * @return the current row
     **/
    int currentRow() const				{ return (mNextRow); }

    /**
     * Set the current layout row, which is the next row that will be filled
     * by either of the @c addRow() functions.
     *
     * @param row The layout row to set.
     **/
    void setCurrentRow(int row)				{ mNextRow = row; }

    /**
     * Clear the current layout row and delete any widgets which it contains.
     **/
    void clearRow();

private:
    void checkPendingGroup();

    QGridLayout *mLayout;
    int mNextRow;
    QWidget *mPendingGroup;
};

#endif // SCANPARAMSPAGE_H
