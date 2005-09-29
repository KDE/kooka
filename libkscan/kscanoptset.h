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

#ifndef KSCANOPTSET_H
#define KSCANOPTSET_H

#include <qobject.h>
#include <qstring.h>
#include <qptrlist.h>
#include <qasciidict.h>


#include "kscanoption.h"

/**
  * This is a container class for KScanOption-objects, which contain information
  * about single scanner dependant options. It allows you to store a bunch
  * of options and accessing them via a iterator.
  *
  * The class which is inherited from QAsciiDict does no deep copy of the options
  * to store by with the standard method insert.
  * @see backupOption to get a deep copy.
  *
  * Note that the destructor of the KScanOptSet only clears the options created
  * by backupOption.
  *
  * @author  Klaas Freitag@SuSE.de
  * @version 0.1
  */



class KScanOptSet: public QAsciiDict<KScanOption>
{

public:
   /**
    *  Constructor to create  a new Container. Takes a string as a name, which
    *  has no special meaning yet ;)
    */
   KScanOptSet( const QCString& );
   ~KScanOptSet();

   /**
    *  function to store a deep copy of an option. Note that this class is inherited
    *  from QAsciiDict and thus does no deep copies.  This method does.
    *  @see insert
    */
   bool backupOption( const KScanOption& );

   /**
    *  returns a pointer to a stored option given by name.
    */
   KScanOption *get( const QCString name ) const;
   QCString      getValue( const QCString name ) const;

   void backupOptionDict( const QAsciiDict<KScanOption>& ); 

   /**
    * saves a configuration set to the configuration file 'ScanSettings'
    * in the default dir config (@see KDir). It uses the group given
    * in configName and stores the entire option set in that group.
    * additionally, a description  is also saved.
    *
    * @param scannerName : the name of the scanner
    * @param configName: The name of the config, e.g. Black and White
    * @param descr : A description for the config.
    */
   void saveConfig( const QString&, const QString&, const QString&);

   /**
    * allows to load a configuration. Simple create a optionSet with the
    * approbiate name the config was called ( @see saveConfig ) and call
    * load for the scanner you want.
    * @param scannerName: A scanner's name
    */
   bool load( const QString& scannerName );

   QString  getDescription() const;
   
public slots:
 
    void slSetDescription( const QString& );
   
private:
   QCString name;

   /* List to collect objects for which memory was allocated and must be freed */
   QPtrList<KScanOption> strayCatsList;

   class KScanOptSetPrivate;
   KScanOptSetPrivate *d;

   QString description;
};

#endif // KScanOptSet
