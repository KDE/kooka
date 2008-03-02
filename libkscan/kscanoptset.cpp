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
//#include <qdict.h>
//#include <qstringlist.h>

#include <kdebug.h>
#include <kconfig.h>
#include <klocale.h>

#include "kscandevice.h"
#include "kscanoption.h"

#include "kscanoptset.h"


#define SAVESET_GROUP		"Save Set"
#define SAVESET_KEY_SETDESC	"SetDesc"
#define SAVESET_KEY_SCANNER	"ScannerName"


KScanOptSet::KScanOptSet( const QCString& setName )
{
  name = setName;

  setAutoDelete( false );

  description = "";

  strayCatsList.setAutoDelete( true );
}



KScanOptSet::~KScanOptSet()
{
    kdDebug(29000) << k_funcinfo << "have " << strayCatsList.count() << " strays" << endl;
    strayCatsList.clear();				/* deep copies from backupOption */
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


void KScanOptSet::saveConfig( const QString& scannerName, const QString& configName,
			      const QString& descr )
{
    QString confFile = SCANNER_DB_FILE;
    kdDebug(29000) << k_funcinfo << "scanner [" << scannerName << "]"
                   << " config=[" << configName << "]"
                   << " file <" << confFile << ">" << endl;

    KConfig scanConfig(confFile);
    QString cfgName = configName;
    if (cfgName.isEmpty()) cfgName = "default";

    scanConfig.setGroup(QString("%1 %2").arg(SAVESET_GROUP,cfgName));
    scanConfig.writeEntry(SAVESET_KEY_SETDESC,descr);
    scanConfig.writeEntry(SAVESET_KEY_SCANNER,scannerName);

    QAsciiDictIterator<KScanOption> it(*this);
    while (it.current()!=NULL)
    {
        const QString line = it.current()->configLine();
        if (line!=PARAM_ERROR)
        {
            const QString name = it.current()->getName();
            kdDebug(29000) << "  writing <" << name << "> = <" << line << ">" << endl;
            scanConfig.writeEntry(name,line);
        }
        ++it;
    }

    scanConfig.sync();
    kdDebug(29000) << k_funcinfo << "done" << endl;
}


bool KScanOptSet::load(const QString &scannerName)
{
    kdDebug(29000) << k_funcinfo << "Reading <" << name << "> from <" << SCANNER_DB_FILE << ">" << endl;

    KConfig conf(SCANNER_DB_FILE,true,false);

    QString grpName = QString("%1 %2").arg(SAVESET_GROUP,name); /* of the KScanOptSet, given in constructor */
    if (!conf.hasGroup(grpName))
    {
        kdDebug(29000) << "Group " << grpName << " does not exist in configuration!" << endl;
        return (false);
    }

    conf.setGroup(grpName);
    StringMap strMap = conf.entryMap(grpName);

    for (StringMap::Iterator it = strMap.begin(); it!=strMap.end(); ++it)
    {
        QString optName = it.key().latin1();
        if (optName==SAVESET_KEY_SETDESC) continue;
        if (optName==SAVESET_KEY_SCANNER) continue;

        KScanOption optset(optName.latin1());

        QCString val = it.data().latin1();
        kdDebug(29000) << "For <" << optName << "> read value <" << val << ">" << endl;

        optset.set(val);
        backupOption(optset);
    }

    return (true);
}



KScanOptSet::StringMap KScanOptSet::readList()
{
    KConfig conf(SCANNER_DB_FILE,true,false );
    const QString groupName = SAVESET_GROUP;
    StringMap ret;

    const QStringList groups = conf.groupList();
    for (QStringList::const_iterator it = groups.constBegin(); it!=groups.constEnd(); ++it)
    {
        QString grp = (*it);
        kdDebug(29000) << k_funcinfo << "group [" << grp << "]" << endl;
        if (grp.startsWith(groupName))
        {
            QString set = grp.mid(groupName.length()+1);
            if (set==DEFAULT_OPTIONSET) continue;	// don't show this one

            conf.setGroup(grp);
            ret[set] = conf.readEntry(SAVESET_KEY_SETDESC,i18n("No description"));
        }
    }

    return (ret);
}


void KScanOptSet::deleteSet(const QString &name)
{
    KConfig conf(SCANNER_DB_FILE,false,false );

    QString grpName = QString("%1 %2").arg(SAVESET_GROUP,name);
    conf.deleteGroup(grpName);
    conf.sync();
}
