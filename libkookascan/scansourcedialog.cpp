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

#include "scansourcedialog.h"

#include <qlabel.h>
#include <qradiobutton.h>
#include <qgroupbox.h>
#include <qlayout.h>

#include <KLocalizedString>
#include <QDebug>
#include <kvbox.h>

#include "kscancontrols.h"

extern "C" {
#include <sane/saneopts.h>
}

#ifndef SANE_NAME_DOCUMENT_FEEDER
#define SANE_NAME_DOCUMENT_FEEDER "Automatic Document Feeder"
#endif

/**
  * Internal private class for ScanSourceDialog
  */
class ScanSourceDialogPrivate
{
public:
    KScanCombo       *sources;
    QGroupBox        *bgroup;
    QVBoxLayout      *bgroupLayout;
    QRadioButton     *rbADFTillEnd, *rbADFOnce;
    AdfBehaviour adf;
    bool adf_enabled;

public:
    ScanSourceDialogPrivate()
        : sources(0), bgroup(0), bgroupLayout(0), rbADFTillEnd(0), rbADFOnce(0),
          adf(ADF_OFF), adf_enabled(false)
    {
    }
};

ScanSourceDialog::ScanSourceDialog(QWidget *parent, const QList<QByteArray> list, AdfBehaviour adfBehave)
    : KDialog(parent)
{
    d = new ScanSourceDialogPrivate();
    setObjectName("ScanSourceDialog");

    setButtons(KDialog::Ok | KDialog::Cancel);
    setCaption(i18n("Custom Gamma Tables"));
    setModal(true);
    showButtonSeparator(true);

    KVBox *vbox = new KVBox(this);
    setMainWidget(vbox);

    (void) new QLabel(i18n("<b>Source selection</b><br />"
                           "Note that you may see more sources than actually exist"), vbox);

    /* Combo Box for sources */
    d->sources = new KScanCombo(vbox, QString());
    d->sources->setList(list);
    connect(d->sources, SIGNAL(activated(int)), SLOT(slotChangeSource(int)));

    if (sourceAdfEntry() > -1) {
        d->bgroup = new QGroupBox(i18n("Advanced ADF Options"));

        /* Two buttons inside */
        d->bgroupLayout = new QVBoxLayout();

        d->rbADFTillEnd = new QRadioButton(i18n("Scan until ADF reports out of paper"));
        connect(d->rbADFTillEnd, SIGNAL(toggled(bool)), this, SLOT(slotNotifyADF(bool)));
        d->bgroupLayout->addWidget(d->rbADFTillEnd);

        d->rbADFOnce = new QRadioButton(i18n("Scan only one sheet of ADF per click"));
        connect(d->rbADFOnce, SIGNAL(toggled(bool)), this, SLOT(slotNotifyADF(bool)));
        d->bgroupLayout->addWidget(d->rbADFOnce);

        d->bgroup->setLayout(d->bgroupLayout);

        switch (adfBehave) {
        case ADF_OFF:
            d->rbADFOnce->setChecked(true);
            enableBGroup(false);
            break;
        case ADF_SCAN_ONCE:
            d->rbADFOnce->setChecked(true);
            break;
        case ADF_SCAN_ALONG:
            d->rbADFTillEnd->setChecked(true);
            break;
        default:
            //qDebug() << "Undefined Source!";
            // Hmmm.
            break;
        }

        d->adf = adfBehave;
    }
}

ScanSourceDialog::~ScanSourceDialog()
{
    if (d) {
        delete d;
        d = nullptr;
    }
}

QString  ScanSourceDialog::getText(void) const
{
    return (d->sources->text());
}

AdfBehaviour ScanSourceDialog::getAdfBehave(void) const
{
    return (d->adf);
}

void ScanSourceDialog::slotNotifyADF(bool checked)
{
    if (checked) {
        // check which one sent the event:
        d->adf = ADF_OFF;
        if (sender() == d->rbADFOnce) {
            d->adf = ADF_SCAN_ONCE;
        } else if (sender() == d->rbADFTillEnd) {
            d->adf = ADF_SCAN_ALONG;
        }
    }

#if 0
    // debug( "reported adf-select %d", adf_group );
    /* this seems to be broken, adf_text is a visible string?
    *  needs rework if SANE 2 comes up which supports i18n */
    QString adf_text = getText();

    adf = ADF_OFF;

    if (adf_text == "Automatic Document Feeder" ||
            adf_text == "ADF") {
        if (adf_group == 0) {
            adf = ADF_SCAN_ALONG;
        } else {
            adf = ADF_SCAN_ONCE;
        }
    }
#endif
}

void ScanSourceDialog::slotChangeSource(int i)
{
    if (d->bgroup == nullptr) {
        return;
    }

    if (i == sourceAdfEntry()) {
        /* Adf was switched on */
        enableBGroup(true);
        d->rbADFOnce->setChecked(true);
        d->adf = ADF_SCAN_ALONG;
        d->adf_enabled = true;
    } else {
        enableBGroup(false);
        // d->adf = ADF_OFF;
        d->adf_enabled = false;
    }
}

int ScanSourceDialog::sourceAdfEntry() const
{
    if (d->sources == nullptr) {
        return (-1);
    }

    int cou = d->sources->count();

    for (int i = 0; i < cou; i++) {
        QString q = d->sources->textAt(i);

// TODO: this enables advanced ADF options, not implemented yet
#if 0
        if (q == "ADF" || q == SANE_NAME_DOCUMENT_FEEDER) {
            return (i);
        }
#endif

    }
    return (-1);
}

void ScanSourceDialog::slotSetSource(const QString &source)
{
    if (d->sources == nullptr) {
        return;
    }

    //qDebug() << "Setting source to" << source;

    enableBGroup(false);
    d->adf_enabled = false ;

    for (int i = 0; i < d->sources->count(); i++) {
        if (d->sources->textAt(i) == source) {
            d->sources->setValue(i);
            if (source == QString::number(sourceAdfEntry())) {
                if (d->bgroup) {
                    enableBGroup(true);
                }

                d->adf_enabled = true;
            }
            break;
        }
    }

}

/**
  QGroupBox doesn't have a way to enable / disable all contained radio buttons, so we need to implement a
  dedicated function for it.
  */

void ScanSourceDialog::enableBGroup(bool enable)
{
    if (d->bgroup) {
        d->rbADFOnce->setEnabled(enable);
        d->rbADFTillEnd->setEnabled(enable);
    }
}
