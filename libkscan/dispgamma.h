/* This file is part of the KDE Project
   Copyright (C) 2000 Klaas Freitag <freitag@suse.de>  

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

#ifndef DISPGAMMA_H
#define DISPGAMMA_H

#include "libkscanexport.h"

#include <qwidget.h>
#include <qsizepolicy.h>
#include <qsize.h>

extern "C"
{
#include <sane/sane.h>
#include <sane/saneopts.h>
}

class QPaintEvent;
class QVector<class T>;


/**
  *@author Klaas Freitag
  */

class KSCAN_EXPORT DispGamma : public QWidget
{
    Q_OBJECT

public: 
    DispGamma( QWidget *parent );
    ~DispGamma();

    QSize sizeHint() const;
    QSizePolicy sizePolicy();

    void setValueRef(QVector<SANE_Word> *newVals) { vals = newVals; }

protected:
    void paintEvent(QPaintEvent *ev);
    void resizeEvent(QResizeEvent *ev);

private:
    QVector<SANE_Word> *vals;
    int margin;
};

#endif							// DISPGAMMA_H
