/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2000-2016 Klaas Freitag <freitag@suse.de>		*
 *                          Jonathan Marten <jjm@keelhaul.me.uk>	*
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

#include "galleryroot.h"

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_STRERROR
#include <string.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <qdir.h>
#include <qstandardpaths.h>

#include <kwidgetsaddons_version.h>
#include <klocalizedstring.h>
#include <kmessagebox.h>
#include <kstandardguiitem.h>

#include "kookasettings.h"
#include "kooka_logging.h"

// Support for the gallery location - moved here from KookaPref so that it
// can be used in the core library.  Before that is was part of the Previewer
// class in libkscan.


// The global resolved gallery location, static so that the user is only asked
// at most once in an application run.  No need to use Q_GLOBAL_STATIC, because
// its initialisation is guarded with the flag.
static QUrl sGalleryRoot;
static bool sGalleryLocated = false;


// Get the user's configured KDE documents path.  It may not exist yet, in
// which case QDir::canonicalPath() will fail - try QDir::absolutePath() in
// this case.  If all else fails then the last resort is the home directory.
static QString docsPath()
{
    QString docpath = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).canonicalPath();
    if (docpath.isEmpty()) {
        docpath = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).absolutePath();
    }
    if (docpath.isEmpty()) {
        docpath = QFile::decodeName(getenv("HOME"));
    }
    return (docpath);
}


// TODO: maybe save a .directory file there which shows a 'scanner' logo?
static QString createGallery(const QDir &d, bool *success = nullptr)
{
    if (!d.exists())					// does not already exist
    {
        if (mkdir(QFile::encodeName(d.path()).constData(), 0755) != 0)
        {						// using mkdir(2) so that we can
							// get the errno if it fails
#ifdef HAVE_STRERROR
            const char *reason = strerror(errno);
#else
            const char *reason = "";
#endif
            QString docs = docsPath();
            KMessageBox::error(nullptr,
                               xi18nc("@info", "Unable to create the directory <filename>%1</filename>"
                                      "<nl/>for the Kooka gallery"
#ifdef HAVE_STRERROR
                                      " - %3."
#else
                                      "."
#endif
                                      "<nl/><nl/>Your document directory <filename>%2</filename>"
                                      "<nl/>will be used."
                                      "<nl/><nl/>Check the document directory setting and permissions.",
                                    d.absolutePath(), docs, reason),
                               i18n("Error creating gallery"));

            if (success != nullptr) *success = false;
            return (docs);
        }
    }

    if (success != nullptr) {
        *success = true;
    }
    return (d.absolutePath());
}


static QString findGalleryRoot()
{
    QString galleryName = KookaSettings::galleryName();	// may be base name or absolute path
    if (galleryName.isEmpty())
    {
        qCWarning(KOOKA_LOG) << "Gallery name not configured";
        return (QString());
    }

    QString oldpath = QStandardPaths::locate(QStandardPaths::AppDataLocation, "ScanImages", QStandardPaths::LocateDirectory);
    bool oldexists = !oldpath.isEmpty();

    QString newpath(galleryName);
    if (!QDir::isAbsolutePath(galleryName))
    {
        QString docpath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        newpath = docpath+'/'+galleryName;		// already an absolute path
    }

    QDir newdir(newpath);
    bool newexists = newdir.exists();

    qCDebug(KOOKA_LOG) << "old" << oldpath << "exists" << oldexists;
    qCDebug(KOOKA_LOG) << "new" << newpath << "exists" << newexists;

    QString dir;

    if (!oldexists && !newexists) {			// no directories present
        dir = createGallery(newdir);			// create and use new
    } else if (!oldexists && newexists) {		// only new exists
        dir = newpath;					// fine, just use that
    } else if (oldexists && !newexists) {		// only old exists
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5, 100, 0)
        if (KMessageBox::questionTwoActions(nullptr,
#else
        if (KMessageBox::questionYesNo(nullptr,
#endif
                                       xi18nc("@info",
                                              "An old Kooka gallery was found at <filename>%1</filename>."
                                              "<nl/>The preferred new location is now <filename>%2</filename>."
                                              "<nl/><nl/>Do you want to create a new gallery at the new location?",
                                              oldpath, newpath),
                                       i18n("Create New Gallery"),
                                       KGuiItem(i18nc("@action:button", "Create New"), QStringLiteral("folder-new")),
                                       KGuiItem(i18nc("@action:button", "Continue With Old"), QStringLiteral("dialog-cancel")),
                                       "GalleryNoMigrate")
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5, 100, 0)
            == KMessageBox::PrimaryAction) {
#else
            == KMessageBox::Yes) {
#endif
            // yes, create new
            bool created;
            dir = createGallery(newdir, &created);
            if (created) {				// new created OK
                KMessageBox::information(nullptr,
                                         xi18nc("@info",
                                                "Kooka will use the new gallery, <link url=\"file:%1\"><filename>%1</filename></link>."
                                                "<nl/><nl/>If you wish to add the images from your old gallery <link url=\"file:%2\"><filename>%2</filename></link>,"
                                                "<nl/>then you may do so by simply copying or moving the files.",
                                                newpath, oldpath),
                                         i18n("New Gallery Created"),
                                         QString(),
                                         KMessageBox::Notify | KMessageBox::AllowLink);
            }
        } else {					// no, don't create
            dir = oldpath;				// stay with old location
        }
    } else {						// both exist
        KMessageBox::information(nullptr,
                                 xi18nc("@info",
                                        "Kooka will use the new gallery, <link url=\"file:%1\"><filename>%1</filename></link>."
                                        "<nl/><nl/>If you wish to add the images from your old gallery <link url=\"file:%2\"><filename>%2</filename></link>,"
                                        "<nl/>then you may do so by simply copying or moving the files.",
                                        newpath, oldpath),
                                 i18n("Old Gallery Exists"),
                                 "GalleryNoRemind",
                                 KMessageBox::Notify | KMessageBox::AllowLink);
        dir = newpath;					// just use new one
    }

    if (!dir.endsWith("/")) dir += "/";
    qCDebug(KOOKA_LOG) << "using" << dir;
    return (dir);
}


QUrl GalleryRoot::root()
{
    if (!sGalleryLocated)
    {
        sGalleryRoot = QUrl::fromLocalFile(findGalleryRoot());
        if (!sGalleryRoot.isValid()) qCWarning(KOOKA_LOG) << "root not valid!";
        sGalleryLocated = true;
    }

    return (sGalleryRoot);
}
