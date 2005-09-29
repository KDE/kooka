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

#include <qstring.h>
#include <qasciidict.h>
#include <qmap.h>
#include <qdict.h>
#include <kdebug.h>
#include <kconfig.h>

#include "kscandevice.h"
#include "kscanoption.h"
#include "kscanoptset.h"

KScanOptSet::KScanOptSet( const QCString& setName )
{
  name = setName;

  setAutoDelete( false );

  description = "";

  strayCatsList.setAutoDelete( true );
}



KScanOptSet::~KScanOptSet()
{
   /* removes all deep copies from backupOption */
   strayCatsList.clear();
}



KScanOption *KScanOptSet::get( const QCString name ) const
{
  KScanOption *ret = 0;

  ret = (*this) [name];

  return( ret );
}

QCString KScanOptSet::getValue( const QCString name ) const
{
   KScanOption *re = get( name );
   QCString retStr = "";

   if( re )
   {
      retStr = re->get();
   }
   else
   {
      kdDebug(29000) << "option " << name << " from OptionSet is not available" << endl;
   }
   return( retStr );
}


bool KScanOptSet::backupOption( const KScanOption& opt )
{
  bool retval = true;

  /** Allocate a new option and store it **/
  const QCString& optName = opt.getName();
  if( !optName )
    retval = false;

  if( retval )
  {
     KScanOption *newopt = find( optName );

     if( newopt )
     {
	/** The option already exists **/
	/* Copy the new one into the old one. TODO: checken Zuweisungoperatoren OK ? */
	*newopt = opt;
     }
     else
     {
	const QCString& qq = opt.get();
	kdDebug(29000) << "Value is now: <" << qq << ">" << endl;
	const KScanOption *newopt = new KScanOption( opt );

	strayCatsList.append( newopt );

	if( newopt )
	{
	   insert( optName, newopt );
	} else {
	   retval = false;
	}
     }
  }

  return( retval );

}

QString KScanOptSet::getDescription() const
{
   return description;
}

void KScanOptSet::slSetDescription( const QString& str )
{
   description = str;
}

void KScanOptSet::backupOptionDict( const QAsciiDict<KScanOption>& optDict )
{
   QAsciiDictIterator<KScanOption> it( optDict );

   while ( it.current() )
   {
      kdDebug(29000) << "Dict-Backup of Option <" << it.currentKey() << ">" << endl;
      backupOption( *(it.current()));
      ++it;
   }


}

/* */
void KScanOptSet::saveConfig( const QString& scannerName, const QString& configName,
			      const QString& descr )
{
   QString confFile = SCANNER_DB_FILE;
   kdDebug( 29000) << "Creating scan configuration file <" << confFile << ">" << endl;

   KConfig *scanConfig = new KConfig( confFile );
   QString cfgName = configName;

   if( configName.isNull() || configName.isEmpty() )
      cfgName = "default";

   scanConfig->setGroup( cfgName );

   scanConfig->writeEntry( "description", descr );
   scanConfig->writeEntry( "scannerName", scannerName );
   QAsciiDictIterator<KScanOption> it( *this);

    while ( it.current() )
    {
       const QString line = it.current() -> configLine();
       const QString name = it.current()->getName();

       kdDebug(29000) << "writing " << name << " = <" << line << ">" << endl;

       scanConfig->writeEntry( name, line );

       ++it;
    }

    scanConfig->sync();
    delete( scanConfig );
}

bool KScanOptSet::load( const QString& /*scannerName*/ )
{
   QString confFile = SCANNER_DB_FILE;
   kdDebug( 29000) << "** Reading from scan configuration file <" << confFile << ">" << endl;
   bool ret = true;

   KConfig *scanConfig = new KConfig( confFile, true );
   QString cfgName = name; /* of the KScanOptSet, given in constructor */

   if( cfgName.isNull() || cfgName.isEmpty() )
      cfgName = "default";

   if( ! scanConfig->hasGroup( name ) )
   {
      kdDebug(29000) << "Group " << name << " does not exist in configuration !" << endl;
      ret = false;
   }
   else
   {
      scanConfig->setGroup( name );

      typedef QMap<QString, QString> StringMap;

      StringMap strMap = scanConfig->entryMap( name );

      StringMap::Iterator it;
      for( it = strMap.begin(); it != strMap.end(); ++it )
      {
	 QCString optName = it.key().latin1();
	 KScanOption optset( optName );

	 QCString val = it.data().latin1();
	 kdDebug(29000) << "Reading for " << optName << " value " << val << endl;

	 optset.set( val );

	 backupOption( optset );
      }
   }
   delete( scanConfig );

   return( ret );
}

/* END */
