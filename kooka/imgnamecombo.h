/***************************************************************************
                          imgnamecombo.h - combobox for image names
                             -------------------                                         
    begin                : Tue Nov 13 2001
    copyright            : (C) 2001 by Klaas Freitag                         
    email                : freitag@suse.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef IMGNAMECOMBO_H
#define IMGNAMECOMBO_H


#include <kcombobox.h>

/**
  *@author Klaas Freitag
*/

class QListViewItem;
class KFileBranch;

class ImageNameCombo: public KComboBox
{
   Q_OBJECT
public: 
   ImageNameCombo( QWidget* );
   ~ImageNameCombo();

public slots:

   void slotGalleryPathChanged( KFileTreeBranch* branch, const QString& relativPath );
   void slotPathRemove( KFileTreeBranch *branch, const QString& relPath );
private:
   void rewriteList( KFileTreeBranch *, const QString& selText );
   QStringList items;
};

#endif
