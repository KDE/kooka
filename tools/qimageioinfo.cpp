/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2009-2018 Jonathan Marten <jjm@keelhaul.me.uk>	*
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
#include <qtextstream.h>
#include <qcoreapplication.h>
#include <qcommandlineparser.h>
#include <qimagereader.h>
#include <qimagewriter.h>
#include <qmimetype.h>
#include <qmimedatabase.h>


#define READ		0x01
#define WRITE		0x02

#define FMT		"%1  %2 %3    %4   %5    %6"
#define FIELD1		(-8)
#define FIELD23		(-5)
#define FIELD4		(-30)
#define FIELD5		(-9)
#define FIELD6		(-5)

#define YESNO(b)	((b) ? " Y" : " -")

#if (QT_VERSION>=QT_VERSION_CHECK(5, 12, 0))
#define HAVE_REVERSE_LOOKUP
#endif


int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("qimageioinfo");
    app.setApplicationVersion("1.0.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("Show QImageIO supported formats and information");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(app);

    QHash<QByteArray,int> combinedMap;

    QList<QByteArray> readTypes = QImageReader::supportedImageFormats();
    QList<QByteArray> writeTypes = QImageWriter::supportedImageFormats();

    for (const QByteArray &it : qAsConst(readTypes))
    {
        combinedMap[it.toUpper().trimmed()] |= READ;
    }

    for (const QByteArray &it : qAsConst(writeTypes))
    {
        combinedMap[it.toUpper().trimmed()] |= WRITE;
    }

    QList<QByteArray> formats(combinedMap.keys());
    std::sort(formats.begin(), formats.end());

    QTextStream ts(stdout);

    ts << endl << QString("Found %1 supported image formats").arg(formats.count()) << endl;

    ts << endl;
    ts << QString(FMT)
        .arg("Format",FIELD1)
        .arg("Read",FIELD23)
        .arg("Write",FIELD23)
        .arg("MIME type",FIELD4)
#ifdef HAVE_REVERSE_LOOKUP
        .arg("Reverse",FIELD5)
#else
        .arg("",FIELD5)
#endif
        .arg("Extension",FIELD6)
       << endl;
    ts << QString(FMT)
        .arg("------",FIELD1)
        .arg("----",FIELD23)
        .arg("-----",FIELD23)
        .arg("---------",FIELD4)
#ifdef HAVE_REVERSE_LOOKUP
        .arg("-------",FIELD5)
#else
        .arg("",FIELD5)
#endif
        .arg("---------",FIELD6)
       << endl;

    QMimeDatabase db;

    for (const QByteArray &format : qAsConst(formats))
    {
        int sup = combinedMap[format];

        const QMimeType mime = db.mimeTypeForFile(QString("/tmp/x.")+format.toLower(), QMimeDatabase::MatchExtension);
        bool isDefault = (mime.name()=="application/octet-stream");
        QString type = (isDefault ? "(none)" : mime.name());
        QString ext = (isDefault ? "" : mime.suffixes().value(0));
        QString icon = mime.iconName();

#ifdef HAVE_REVERSE_LOOKUP
        QString backfmt = "(none)";
        QList<QByteArray> formats = QImageReader::imageFormatsForMimeType(mime.name().toLocal8Bit());
        int fcount = formats.count();			// get format from file name
        if (fcount>0)
        {
            backfmt = formats.first().trimmed().toUpper();
            if (fcount>1) backfmt += QString(" (+%1)").arg(fcount-1);
        }
#else
        QString backfmt = "";
#endif

        ts << QString(FMT)
            .arg(format.constData(),FIELD1)
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
