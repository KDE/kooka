/***************************************************************************
                          kookapref.h  - Preferences
                             -------------------                                         
    begin                : Sun Jan 16 2000                                           
    copyright            : (C) 2000 by Klaas Freitag                         
    email                : kooka@suse.de            
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   * 
 *                                                                         *
 ***************************************************************************/
#ifndef KOOKAPREF_H
#define KOOKAPREF_H

#include <kdialogbase.h>
#include <qframe.h>

class KookaPrefPageOne;
class KookaPrefPageTwo;

class KookaPreferences : public KDialogBase
{
    Q_OBJECT
public:
    KookaPreferences();

private:
    KookaPrefPageOne *m_pageOne;
    KookaPrefPageTwo *m_pageTwo;
};

class KookaPrefPageOne : public QFrame
{
    Q_OBJECT
public:
    KookaPrefPageOne(QWidget *parent = 0);
};

class KookaPrefPageTwo : public QFrame
{
    Q_OBJECT
public:
    KookaPrefPageTwo(QWidget *parent = 0);
};

#endif // KOOKAPREF_H
