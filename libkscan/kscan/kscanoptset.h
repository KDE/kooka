/***************************************************************************
                   kscanoptset.h - store a set of scan options
                             -------------------
    begin                : Wed Oct 13 2000
    copyright            : (C) 2000 by Klaas Freitag
    email                : Klaas.Freitag@SuSE.de

    $Id$
***************************************************************************/
 
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 

#ifndef KSCANOPTSET_H
#define KSCANOPTSET_H
#include <qobject.h>
#include <qstring.h>
#include <qlist.h>

/**
 *  KScanOptSet - Store a set of scan option.
 *
 *  KScanOptSet stores a set of scan options, which can be saved and restored
 *  later
 **/

#include "kscandevice.h"
#include "kscanoption.h"


class KScanOptSet: QObject
{
  Q_OBJECT

public:
   KScanOptSet( QString , const KScanDevice* );
  ~KScanOptSet() {};

private:
  QString name;

  QList <KScanOption> optionList;
};

#endif // KScanOptSet
