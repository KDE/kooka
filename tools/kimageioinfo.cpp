/************************************************************************/
/*									*/
/*  This file is part of the KDE Project				*/
/*  Copyright (c) 2009 Jonathan Marten <jjm@keelhaul.me.uk>		*/
/*									*/
/*  This program is free software; you can redistribute it and/or	*/
/*  modify it under the terms of the GNU General Public License as	*/
/*  published by the Free Software Foundation; either version 2 of	*/
/*  the License, or (at your option) any later version.			*/
/*									*/
/*  It is distributed in the hope that it will be useful, but		*/
/*  WITHOUT ANY WARRANTY; without even the implied warranty of		*/
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*/
/*  GNU General Public License for more details.			*/
/*									*/
/*  You should have received a copy of the GNU General Public		*/
/*  License along with this program; see the file COPYING for further	*/
/*  details.  If not, write to the Free Software Foundation, Inc.,	*/
/*  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.	*/
/*									*/
/************************************************************************/

#include <stdlib.h>
#include <stdio.h>

#include <qhash.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtextstream.h>

#include <kcmdlineargs.h>
#include <kapplication.h>
#include <kimageio.h>
#include <kmimetype.h>


#define READ		0x01
#define WRITE		0x02

#define FMT		"%1    %2  %3      %4    %5"
#define FIELD1		(-8)
#define FIELD23		(-5)
#define FIELD4		(-30)
#define FIELD5		(-5)

#define YESNO(b)	((b) ? " Y" : " -")


int main(int argc, char **argv)
{
    KCmdLineArgs::init(argc,					// argc
                       argv,					// argv
                       "kimageioinfo",				// appname
                       "",					// catalog
                       ki18n("KImageIOinfo"),			// programName
                       "1.00",					// version
                       ki18n("Show KImageIO information"),	// description
                       KCmdLineArgs::CmdLineArgNone);		// stdargs

    KApplication app(false);					// no GUI

    QHash<QString,int> combinedMap;

    QStringList readTypes = KImageIO::types(KImageIO::Reading);
    QStringList writeTypes = KImageIO::types(KImageIO::Writing);

    for (QStringList::const_iterator it = readTypes.constBegin();
         it!=readTypes.constEnd(); ++it)
    {
        combinedMap[(*it).toUpper().trimmed()] |= READ;
    }

    for (QStringList::const_iterator it = writeTypes.constBegin();
         it!=writeTypes.constEnd(); ++it)
    {
        combinedMap[(*it).toUpper().trimmed()] |= WRITE;
    }

    QStringList formats(combinedMap.keys());
    formats.sort();

    QTextStream ts(stdout);

    ts << endl << QString("Found %1 supported image formats").arg(formats.count()) << endl;

    ts << endl;
    ts << QString(FMT)
        .arg("Format",FIELD1)
        .arg("Read",FIELD23)
        .arg("Write",FIELD23)
        .arg("MIME type",FIELD4)
        .arg("Extension",FIELD5)
       << endl;
    ts << QString(FMT)
        .arg("------",FIELD1)
        .arg("----",FIELD23)
        .arg("-----",FIELD23)
        .arg("---------",FIELD4)
        .arg("---------",FIELD5)
       << endl;

    for (QStringList::const_iterator it = formats.constBegin();
         it!=formats.constEnd(); ++it)
    {
        QString format = (*it);
        int sup = combinedMap[format];

        KMimeType::Ptr mime = KMimeType::findByPath("/tmp/x."+format.toLower(), 0, true);
        QString type = (mime->isDefault() ? "(none)" : mime->name());
        QString ext = (mime->isDefault() ? "" : mime->mainExtension());
        QString icon = mime->iconName();

        ts << QString(FMT)
            .arg(format,FIELD1)
            .arg(YESNO(sup & READ),FIELD23)
            .arg(YESNO(sup & WRITE),FIELD23)
            .arg(type,FIELD4)
            .arg(ext,FIELD5)
           << endl;
    }

    ts << endl;
    return (EXIT_SUCCESS);
}