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

#include "kookapref.h"

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

#include <qlayout.h>
#include <qicon.h>
#include <qdir.h>
#include <qdebug.h>
#include <qpushbutton.h>
#include <qstandardpaths.h>

#include <klocalizedstring.h>
#include <kmessagebox.h>
#include <kstandardguiitem.h>

#include "prefspages.h"
#include "kookasettings.h"
#include "dialogstatesaver.h"


KookaPref::KookaPref(QWidget *parent)
    : KPageDialog(parent)
{
    setObjectName("KookaPref");

    setModal(true);
    setStandardButtons(QDialogButtonBox::Help | QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults);
    buttonBox()->button(QDialogButtonBox::Ok)->setDefault(true);
    buttonBox()->button(QDialogButtonBox::RestoreDefaults)->setIcon(QIcon::fromTheme("edit-undo"));
    setWindowTitle(i18n("Preferences"));

    createPage(new KookaGeneralPage(this), i18n("General"), i18n("General Options"), "configure");
    createPage(new KookaStartupPage(this), i18n("Startup"), i18n("Startup Options"), "system-run");
    createPage(new KookaSavingPage(this), i18n("Image Saving"), i18n("Image Saving Options"), "document-save");
    createPage(new KookaThumbnailPage(this), i18n("Gallery & Thumbnails"), i18n("Image Gallery and Thumbnail View"), "view-list-icons");
    createPage(new KookaOcrPage(this), i18n("OCR"), i18n("Optical Character Recognition"), "ocr");

    connect(buttonBox()->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &KookaPref::slotSaveSettings);
    connect(buttonBox()->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &KookaPref::slotSaveSettings);
    connect(buttonBox()->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this, &KookaPref::slotSetDefaults);

    setMinimumSize(670, 380);
    new DialogStateSaver(this);
}

int KookaPref::createPage(KookaPrefsPage *page,
                          const QString &name,
                          const QString &header,
                          const char *icon)
{
    QVBoxLayout *top = static_cast<QVBoxLayout *>(page->layout());
    if (top != NULL) {
        top->addStretch(1);
    }

    KPageWidgetItem *item = addPage(page, name);
    item->setHeader(header);
    item->setIcon(QIcon::fromTheme(icon));

    int idx = mPages.count();               // index of new item
    mPages.append(item);
    return (idx);                   // index of item added
}

void KookaPref::slotSaveSettings()
{
    for (int i = 0; i < mPages.size(); ++i) {
        KookaPrefsPage *page = static_cast<KookaPrefsPage *>(mPages[i]->widget());
        page->saveSettings();
    }

    KSharedConfig::openConfig()->sync();
    emit dataSaved();
}

void KookaPref::slotSetDefaults()
{
    for (int i = 0; i < mPages.size(); ++i) {
        KookaPrefsPage *page = static_cast<KookaPrefsPage *>(mPages[i]->widget());
        page->defaultSettings();
    }
}

void KookaPref::showPageIndex(int page)
{
    if (page >= 0 && page < mPages.size()) {
        setCurrentPage(mPages[page]);
    }
}

int KookaPref::currentPageIndex()
{
    return (mPages.indexOf(currentPage()));
}

static QString tryFindBinary(const QString &bin, const QString &configPath)
{
    // First check for a full path in the config file.
    // Not sure what the point of the 'contains' test is here.
    if (!configPath.isEmpty() && configPath.contains(bin))
    {
        QFileInfo fi(configPath);			// check for valid executable
        if (fi.exists() && fi.isExecutable() && !fi.isDir()) return (fi.absoluteFilePath());
    }

    // Otherwise try to find the program on the user's search PATH
    return (QStandardPaths::findExecutable(bin));
}

QString KookaPref::tryFindGocr()
{
    return (tryFindBinary("gocr", KookaSettings::ocrGocrBinary()));
}

QString KookaPref::tryFindOcrad(void)
{
    return (tryFindBinary("ocrad", KookaSettings::ocrOcradBinary()));
}

// Support for the gallery location - moved here from Previewer class in libkscan

QString KookaPref::sGalleryRoot = QString::null;    // global resolved location

// The static variable above ensures that the user is only asked
// at most once in an application run.

QString KookaPref::galleryRoot()
{
    if (sGalleryRoot.isNull()) {
        sGalleryRoot = findGalleryRoot();
    }
    return (sGalleryRoot);
}

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
        docpath = getenv("HOME");
    }
    return (docpath);
}

// TODO: maybe save a .directory file there which shows a 'scanner' logo?
static QString createGallery(const QDir &d, bool *success = NULL)
{
    if (!d.exists()) {                  // does not already exist
        if (mkdir(d.path().toLocal8Bit(), 0755) != 0) { // using mkdir(2) so that we can
            // get the errno if it fails
#ifdef HAVE_STRERROR
            const char *reason = strerror(errno);
#else
            const char *reason = "";
#endif
            QString docs = docsPath();
            KMessageBox::error(NULL,
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

            if (success != NULL) *success = false;
            return (docs);
        }
    }

    if (success != NULL) {
        *success = true;
    }
    return (d.absolutePath());
}

QString KookaPref::findGalleryRoot()
{
    QString galleryName = KookaSettings::galleryName();	// may be base name or absolute path
    if (galleryName.isEmpty())
    {
        qWarning() << "Gallery name not configured";
        return (QString::null);
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

    qDebug() << "old" << oldpath << "exists" << oldexists;
    qDebug() << "new" << newpath << "exists" << newexists;

    QString dir;

    if (!oldexists && !newexists) {			// no directories present
        dir = createGallery(newdir);			// create and use new
    } else if (!oldexists && newexists) {		// only new exists
        dir = newpath;					// fine, just use that
    } else if (oldexists && !newexists) {		// only old exists
        if (KMessageBox::questionYesNo(NULL,
                                       xi18nc("@info",
                                              "An old Kooka gallery was found at <filename>%1</filename>."
                                              "<nl/>The preferred new location is now <filename>%2</filename>."
                                              "<nl/><nl/>Do you want to create a new gallery at the new location?",
                                              oldpath, newpath),
                                       i18n("Create New Gallery"),
                                       KStandardGuiItem::yes(), KStandardGuiItem::no(),
                                       "GalleryNoMigrate") == KMessageBox::Yes) {
            // yes, create new
            bool created;
            dir = createGallery(newdir, &created);
            if (created) {				// new created OK
                KMessageBox::information(NULL,
                                         xi18nc("@info",
                                                "Kooka will use the new gallery, <link url=\"file:%1\"><filename>%1</filename></link>."
                                                "<nl/><nl/>If you wish to add the images from your old gallery <link url=\"file:%2\"><filename>%2</filename></link>,"
                                                "<nl/>then you may do so by simply copying or moving the files.",
                                                newpath, oldpath),
                                         i18n("New Gallery Created"),
                                         QString::null,
                                         KMessageBox::Notify | KMessageBox::AllowLink);
            }
        } else {					// no, don't create
            dir = oldpath;				// stay with old location
        }
    } else {						// both exist
        KMessageBox::information(NULL,
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
    qDebug() << "using" << dir;
    return (dir);
}
