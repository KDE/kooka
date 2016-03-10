/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2009 Jonathan Marten <jjm@keelhaul.me.uk>		*
 *									*
 *  Kooka is free software; you can redistribute it and/or modify it	*
 *  under the terms of the GNU Library General Public License as	*
 *  published by the Free Software Foundation and appearing in the	*
 *  file COPYING included in the packaging of this file;  either	*
 *  version 2 of the License, or (at your option) any later version.	*
 *									*
 *  As a special exception, permission is given to link this program	*
 *  with any version of the KADMOS OCR/ICR engine (a product of		*
 *  reRecognition GmbH, Kreuzlingen), and distribute the resulting	*
 *  executable without including the source code for KADMOS in the	*
 *  source distribution.						*
 *									*
 *  This program is distributed in the hope that it will be useful,	*
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of	*
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*
 *  GNU General Public License for more details.			*
 *									*
 *  You should have received a copy of the GNU General Public		*
 *  License along with this program;  see the file COPYING.  If		*
 *  not, see <http://www.gnu.org/licenses/>.				*
 *									*
 ************************************************************************/

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

#define FMT		"%1  %2 %3    %4   %5    %6"
#define FIELD1		(-8)
#define FIELD23		(-5)
#define FIELD4		(-30)
#define FIELD5		(-9)
#define FIELD6		(-5)

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
        .arg("Reverse",FIELD5)
        .arg("Extension",FIELD6)
       << endl;
    ts << QString(FMT)
        .arg("------",FIELD1)
        .arg("----",FIELD23)
        .arg("-----",FIELD23)
        .arg("---------",FIELD4)
        .arg("-------",FIELD5)
        .arg("---------",FIELD6)
       << endl;

    for (QStringList::const_iterator it = formats.constBegin();
         it!=formats.constEnd(); ++it)
    {
        QString format = (*it);
        int sup = combinedMap[format];

        KMimeType::Ptr mime = KMimeType::findByPath("/tmp/x."+format.toLower(), 0, true);
        QString type = (mime->isDefault() ? "(none)" : mime->name());
        mime->patterns();				// needed for below to work!
        QString ext = (mime->isDefault() ? "" : mime->mainExtension());
        QString icon = mime->iconName();

        QString backfmt = "(none)";
        QStringList formats = KImageIO::typeForMime(mime->name());
        int fcount = formats.count();			// get format from file name
        if (fcount>0)
        {
            backfmt = formats.first().trimmed().toUpper().toLocal8Bit();
            if (fcount>1) backfmt += QString(" (+%1)").arg(fcount-1);
        }

        ts << QString(FMT)
            .arg(format,FIELD1)
            .arg(YESNO(sup & READ),FIELD23)
            .arg(YESNO(sup & WRITE),FIELD23)
            .arg(type,FIELD4)
            .arg(backfmt,FIELD5)
            .arg(ext,FIELD6)
           << endl;
    }

    ts << endl;
    return (EXIT_SUCCESS);
}
