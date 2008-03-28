/* This file is part of the KDE Project
   Copyright (C) 2008 Alex Kempshall <mcmurchy1917-kooka@yahoo.co.uk>  

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

#include <klocale.h>
#include <knuminput.h>
#include <kdialog.h>

#include <qstring.h>
#include <qmap.h>
#include <qlayout.h>
#include <qvbuttongroup.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include "kookaimage.h"
#include <qvgroupbox.h>
#include <qhgroupbox.h>
#include <qpaintdevicemetrics.h>
#include <qlabel.h>
#include <qtooltip.h>
#include <kdebug.h>

#include "photocopyprintdialogpage.h"
#include "photocopyprintdialogpage.moc"

#define ID_SCREEN 0
#define ID_ORIG   1
#define ID_CUSTOM 2
#define ID_FIT_PAGE 3
#define SANE_NAME_SCAN_TL_X             "tl-x"
#define SANE_NAME_SCAN_TL_Y             "tl-y"
#define SANE_NAME_SCAN_BR_X             "br-x"
#define SANE_NAME_SCAN_BR_Y             "br-y"

PhotoCopyPrintDialogPage::PhotoCopyPrintDialogPage( KScanDevice* newScanDevice )
    : KPrintDialogPage( )
{
    kdDebug(28000) << "Created a  PhotoCopyPrintDialogPage" << endl;
    QString str;

    setTitle(i18n("PhotoCopier"));

    sane_device = newScanDevice;

    QVBoxLayout *leftVBox = new QVBoxLayout( this );

    QHGroupBox *prtgroup1 = new QHGroupBox( i18n( "Print options" ), this );

    /* Allow for number of Copies */
    m_copies = new KIntNumInput( prtgroup1 );
    m_copies->setLabel( i18n("Copies: " ), AlignVCenter );
    m_copies->setValue( 1 );
    m_copies->setMinValue( 1 );
    m_copies->setMaxValue( 1 );

    QHBoxLayout *prtHBox = new QHBoxLayout( );

    prtHBox->addWidget( prtgroup1 );

//    QWidget *prtSpaceEater = new QWidget( prtgroup1 );
//    prtSpaceEater->setSizePolicy( QSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored ));
//    prtHBox->addWidget( prtSpaceEater );

    prtHBox->addStretch(1);

// Scanner options
    str = i18n("Scanner Settings: ");
    str += sane_device->getScannerName();
    QVGroupBox *scangroup1 = new QVGroupBox( str, this );

    str = i18n("Scan size:  ");
    QString strBR_X;
    QString strBR_Y;


    if( sane_device->optionExists( SANE_NAME_SCAN_MODE ) )
    {
        KScanOption res ( SANE_NAME_SCAN_BR_X );
        strBR_X = res.get();
    }

    if( sane_device->optionExists( SANE_NAME_SCAN_MODE ) )
    {
        KScanOption res ( SANE_NAME_SCAN_BR_Y );
        strBR_Y = res.get();
    }

    QString str1 = strBR_Y+"x"+strBR_X;
//if (strBR_X == "215" && strBR_Y == "295") str1 ="Letter";
//if (strBR_X == "215" && strBR_Y == "297") str1 ="A4";
//if (strBR_X == "151" && strBR_Y == "211") str1 ="A5";
//if (strBR_X == "107" && strBR_Y == "148") str1 ="A6";

    constructLabel( scangroup1, "Scan Size", &str1);
    constructLabel( scangroup1, "Scan Mode", SANE_NAME_SCAN_MODE );
    constructLabel( scangroup1, "Resolution", SANE_NAME_SCAN_RESOLUTION );
    constructLabel( scangroup1, "Brightness", SANE_NAME_BRIGHTNESS );
    constructLabel( scangroup1, "Contrast", SANE_NAME_CONTRAST );

    QHBoxLayout *scanHBox = new QHBoxLayout( );
    scanHBox->addWidget( scangroup1 );

    scanHBox->addStretch(1);

    leftVBox->addLayout( scanHBox );
    leftVBox->addLayout( prtHBox );

}

PhotoCopyPrintDialogPage::~PhotoCopyPrintDialogPage()
{
    kdDebug(28000) << "Collapsed a PhotoCopyPrintDialogPage!" << endl;
}

void PhotoCopyPrintDialogPage::setOptions(const QMap<QString,QString>& opts)
{
    kdDebug(28000) << "Entered PhotoCopyPrintDialogPage::setOptions" << endl;
}


void PhotoCopyPrintDialogPage::getOptions(QMap<QString,QString>& opts, bool )
{
    kdDebug(28000) << "Entered PhotoCopyPrintDialogPage::getOptions" << endl;
}

bool PhotoCopyPrintDialogPage::isValid(QString& msg)
{

    return true;
}

QLabel* PhotoCopyPrintDialogPage::constructLabel(QVGroupBox* group, char* strTitle,  const QCString&  strSaneOption)
{
    QString str = i18n(strTitle)+": ";
    str.append( "\t" );
    KScanOption res ( strSaneOption );
    str.append( res.get() );
    QLabel* lbl = new QLabel( group );
    lbl->setText( str );
    return ( lbl );
}

QLabel* PhotoCopyPrintDialogPage::constructLabel(QVGroupBox* group, char* strTitle,  QString* strContents)
{
    QString str = i18n(strTitle)+": ";

    str.append( "\t" );
    str.append( *strContents );
    QLabel* lbl = new QLabel( group );
    lbl->setText( str );
    return ( lbl );
}


