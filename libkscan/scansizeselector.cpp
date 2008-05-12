/* This file is part of the KDE Project
   Copyright (C) 2008 Jonathan Marten <jjm@keelhaul.me.uk>

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

#include <qcombobox.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qvbox.h>

#include <klocale.h>
#include <kdebug.h>

#include "scansizeselector.h"
#include "scansizeselector.moc"


struct PaperSize
{
    const char *name;					// no I18N needed here?
    int width,height;					// in portrait orientation
};

static const PaperSize sizes[] =
{
    { "ISO A3",		297,420 },
    { "ISO A4",		210,297 },
    { "ISO A5",		148,210 },
    { "ISO A6",		105,148 },
    { "US Letter",	215,279 },
    { "US Legal",	216,356 },
    { "9x13",		 90,130 },
    { "10x15",		100,150 },
    { NULL,		  0,  0 }
};


ScanSizeSelector::ScanSizeSelector(QWidget *parent,const QSize &bedSize)
    : QVBox(parent)
{
    kdDebug(29000) << k_funcinfo << "bed size = " << bedSize << endl;

    bedWidth = bedSize.width();
    bedHeight = bedSize.height();

    m_sizeCb = new QComboBox(this);
    connect(m_sizeCb,SIGNAL(activated(int)),SLOT(slotSizeSelected(int)));
    setFocusProxy(m_sizeCb);

    m_sizeCb->insertItem(i18n("Full size"));		// index 0
    m_sizeCb->insertItem(i18n("(No selection)"));	// index 1
    for (int i = 0; sizes[i].name!=NULL; ++i)		// index 2 and upwards
    {							// only those that will fit
        if (sizes[i].width>bedWidth || sizes[i].height>bedHeight) continue;
        m_sizeCb->insertItem(sizes[i].name);
    }
    m_sizeCb->setCurrentItem(0);

    QButtonGroup *bg = new QButtonGroup(1,Qt::Vertical,this);
    bg->setFrameStyle(QFrame::NoFrame);
    connect(bg,SIGNAL(clicked(int)),SLOT(slotPortraitLandscape(int)));

    m_portraitRb = new QRadioButton(i18n("Portrait"),bg);
    m_portraitRb->setEnabled(false);
    m_landscapeRb = new QRadioButton(i18n("Landscape"),bg);
    m_landscapeRb->setEnabled(false);

    m_customSize = QRect();
    m_prevSelected = m_sizeCb->currentItem();
}


ScanSizeSelector::~ScanSizeSelector()
{
}


const PaperSize *findPaperSize(QString sizename)
{
    for (int i = 0; sizes[i].name!=NULL; ++i)		// search for that size
    {
        if (sizename==sizes[i].name) return (&sizes[i]);
    }

    return (NULL);
}


void ScanSizeSelector::implementPortraitLandscape(const PaperSize *sp)
{
    m_portraitRb->setEnabled(sp->width<=bedWidth && sp->height<=bedHeight);
    m_landscapeRb->setEnabled(sp->width<=bedHeight && sp->height<=bedWidth);

    if (!m_portraitRb->isChecked() && !m_landscapeRb->isChecked())
    {
        m_portraitRb->setChecked(true);
    }

    if (m_portraitRb->isChecked() && !m_portraitRb->isEnabled())
    {
        m_landscapeRb->setChecked(m_landscapeRb->isEnabled());
    }
    if (m_landscapeRb->isChecked() && !m_landscapeRb->isEnabled())
    {
        m_portraitRb->setChecked(m_portraitRb->isEnabled());
    }
}


void ScanSizeSelector::implementSizeSetting(const PaperSize *sp)
{
    for (int i = 2; i<m_sizeCb->count(); ++i)		// search combo box by name
    {
        if (m_sizeCb->text(i)==sp->name)
        {
            m_sizeCb->setCurrentItem(i);		// set combo box selection
            break;
        }
    }

    implementPortraitLandscape(sp);			// set up radio buttons
}


void ScanSizeSelector::newScanSize(int width,int height)
{
    if (m_portraitRb->isChecked())
    {
        emit sizeSelected(QRect(0,0,width,height));
    }
    else if (m_landscapeRb->isChecked())
    {
        emit sizeSelected(QRect(0,0,height,width));
    }
    else
    {
        kdDebug(29000) << k_funcinfo << "no orientation appropriate!" << endl;
    }
}


void ScanSizeSelector::slotPortraitLandscape(int id)
{
    int idx = m_sizeCb->currentItem();
    if (idx<2) return;					// "Full size" or "Custom"

    const PaperSize *sp = findPaperSize(m_sizeCb->text(idx));
    if (sp==NULL) return;
    newScanSize(sp->width,sp->height);
}


void ScanSizeSelector::slotSizeSelected(int idx)
{
    if (idx==0)						// Full size
    {
        m_portraitRb->setEnabled(false);
        m_landscapeRb->setEnabled(false);
        emit sizeSelected(QRect());
        m_prevSelected = idx;
    }
    else if (idx==1)					// Custom
    {
        if (!m_customSize.isValid())			// is there a saved custom size?
        {
            m_sizeCb->setCurrentItem(m_prevSelected);	// no, snap back to previous
        }
        else
        {
            selectCustomSize(m_customSize);
            emit sizeSelected(m_customSize);
        }
    }
    else						// Named size
    {
        const PaperSize *sp = findPaperSize(m_sizeCb->text(idx));
        if (sp==NULL) return;

        implementPortraitLandscape(sp);
        newScanSize(sp->width,sp->height);
        m_prevSelected = idx;
    }
}


void ScanSizeSelector::selectCustomSize(const QRect &rect)
{
    m_portraitRb->setEnabled(false);
    m_landscapeRb->setEnabled(false);

    if (!rect.isValid()) m_sizeCb->setCurrentItem(0);	// "Full Size"
    else
    {
        m_customSize = rect;				// remember the custom size
        m_sizeCb->changeItem(i18n("Selection"),1);	// now can use this option
        m_sizeCb->setCurrentItem(1);			// "Custom"
    }
}


void ScanSizeSelector::selectSize(const QRect &rect)
{
    kdDebug(29000) << k_funcinfo << "rect=" << rect << " valid=" << rect.isValid() << endl;

    if (rect.isValid() && rect.left()==0 && rect.top()==0)
    {							// can this be a preset size?
        for (int i = 0; sizes[i].name!=NULL; ++i)	// search for preset that size
        {
            if (sizes[i].width==rect.width() && sizes[i].height==rect.height())
            {
                m_portraitRb->setChecked(true);
                m_landscapeRb->setChecked(false);
                implementSizeSetting(&sizes[i]);	// set up controls
                break;
            }
            else if (sizes[i].width==rect.height() && sizes[i].height==rect.width())
            {
                m_portraitRb->setChecked(false);
                m_landscapeRb->setChecked(true);
                implementSizeSetting(&sizes[i]);
                break;
            }
        }

    }
    else selectCustomSize(rect);
}
