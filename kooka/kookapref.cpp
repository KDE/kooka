/* This file is part of the KDE Project

   Copyright (C) 2000 Klaas Freitag <freitag@suse.de>
   Copyright (C) 2010 Jonathan Marten <jjm@keelhaul.me.uk>  

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   As a special exception, permission is given to link this program
   with any version of the KADMOS ocr/icr engine of reRecognition GmbH,
   Kreuzlingen and distribute the resulting executable without
   including the source code for KADMOS in the source distribution.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.

*/

#include "kookapref.h"
#include "kookapref.moc"

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
#include <qdir.h>

#include <klocale.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <kstandardguiitem.h>

#include "prefspages.h"


KookaPref::KookaPref(QWidget *parent)
    : KPageDialog(parent)
{
    setObjectName("KookaPref");

    setModal(true);
    setButtons(KDialog::Help|KDialog::Default|KDialog::Ok|KDialog::Apply|KDialog::Cancel);
    setDefaultButton(KDialog::Ok);
    setCaption(i18n("Preferences"));
    showButtonSeparator(true);

    createPage(new KookaGeneralPage(this), i18n("General"), i18n("General Options"), "configure");
    createPage(new KookaStartupPage(this), i18n("Startup"), i18n("Startup Options"), "system-run");
    createPage(new KookaSavingPage(this), i18n("Image Saving"), i18n("Image Saving Options"), "document-save");
    createPage(new KookaThumbnailPage(this), i18n("Thumbnail View"), i18n("Thumbnail Gallery View"), "view-list-icons");
    createPage(new KookaOcrPage(this), i18n("OCR"), i18n("Optical Character Recognition"), "ocr");

    connect(this, SIGNAL(okClicked()), SLOT(slotSaveSettings()));
    connect(this, SIGNAL(applyClicked()), SLOT(slotSaveSettings()));
    connect(this, SIGNAL(defaultClicked()), SLOT(slotSetDefaults()));

    setMinimumSize(670, 380);
}


int KookaPref::createPage(KookaPrefsPage *page,
                          const QString &name,
                          const QString &header,
                          const char *icon)
{
    QVBoxLayout *top = static_cast<QVBoxLayout *>(page->layout());
    if (top!=NULL) top->addStretch(10);

    KPageWidgetItem *item = addPage(page, name);
    item->setHeader(header);
    item->setIcon(KIcon(icon));

    int idx = mPages.count();				// index of new item
    mPages.append(item);
    return (idx);					// index of item added
}


void KookaPref::slotSaveSettings()
{
    kDebug();
    for (int i = 0; i<mPages.size(); ++i)
    {
        KookaPrefsPage *page = static_cast<KookaPrefsPage *>(mPages[i]->widget());
        page->saveSettings();
    }

    KGlobal::config()->sync();
    emit dataSaved();
}


void KookaPref::slotSetDefaults()
{
    kDebug();
    for (int i = 0; i<mPages.size(); ++i)
    {
        KookaPrefsPage *page = static_cast<KookaPrefsPage *>(mPages[i]->widget());
        page->defaultSettings();
    }
}


void KookaPref::showPageIndex(int page)
{
    if (page>=0 && page<mPages.size()) setCurrentPage(mPages[page]);
}


int KookaPref::currentPageIndex()
{
    return (mPages.indexOf(currentPage()));
}


QString tryFindBinary(const QString &bin, const QString &configKey)
{
    KConfigGroup grp = KGlobal::config()->group(CFG_GROUP_OCR_DIA);

    /* First check the config files for an entry */
    QString exe = grp.readPathEntry(configKey, QString::null);	// try from config file

    // Why do we do the second test here?  checkOcrBinary() does the same, why also?
    if (!exe.isEmpty() && exe.contains(bin))
    {
        QFileInfo fi(exe);				// check for valid executable
        if (fi.exists() && fi.isExecutable() && !fi.isDir()) return (exe);
    }

    /* Otherwise find the program on the user's search path */
    return (KGlobal::dirs()->findExe(bin));		// search using $PATH
}


QString KookaPref::tryFindGocr()
{
    return (tryFindBinary("gocr", CFG_GOCR_BINARY));
}


QString KookaPref::tryFindOcrad( void )
{
    return (tryFindBinary("ocrad", CFG_OCRAD_BINARY));
}


// Support for the gallery location - moved here from Previewer class in libkscan

QString KookaPref::sGalleryRoot = QString::null;	// global resolved location


// The static variable above ensures that the user is only asked
// at most once in an application run.

QString KookaPref::galleryRoot()
{
    if (sGalleryRoot.isNull()) sGalleryRoot = findGalleryRoot();
    return (sGalleryRoot);
}


// Get the user's configured KDE documents path.  It may not exist yet, in
// which case QDir::canonicalPath() will fail - try QDir::absolutePath() in
// this case.  If all else fails then the last resort is the home directory.

static QString docsPath()
{
    QString docpath = QDir(KGlobalSettings::documentPath()).canonicalPath();
    if (docpath.isEmpty()) docpath = QDir(KGlobalSettings::documentPath()).absolutePath();
    if (docpath.isEmpty()) docpath = getenv("HOME");
    return (docpath);
}


// TODO: maybe save a .directory file there which shows a 'scanner' logo?
static QString createGallery(const QDir &d, bool *success = NULL)
{
    if (!d.exists())					// does not already exist
    {
        if (mkdir(d.path().toLocal8Bit(), 0755)!=0)	// using mkdir(2) so that we can
        {						// get the errno if it fails
#ifdef HAVE_STRERROR
            const char *reason = strerror(errno);
#else
            const char *reason = "";
#endif
            QString docs = docsPath();
            KMessageBox::error(NULL,
                               i18n("<qt>"
                                    "<p>Unable to create the directory<br>"
                                    "<filename>%1</filename><br>"
                                    "for the Kooka gallery"
#ifdef HAVE_STRERROR
                                    " - %3."
#else
                                    "."
#endif
                                    "<p>Your document directory,<br>"
                                    "<filename>%2</filename><br>"
                                    "will be used."
                                    "<p>"
                                    "Check the document directory setting and permissions.",
                                    d.absolutePath(), docs, reason),
                               i18n("Error creating gallery"));
            if (success!=NULL) *success = false;
            return (docs);
        }
    }

    if (success!=NULL) *success = true;
    return (d.absolutePath());
}


QString KookaPref::findGalleryRoot()
{
    const KConfigGroup grp = KGlobal::config()->group(GROUP_GALLERY);
    QString galleryName = grp.readEntry(GALLERY_LOCATION, GALLERY_DEFAULT_LOC);
    if (galleryName.isEmpty()) galleryName = GALLERY_DEFAULT_LOC;

    QString oldloc = KGlobal::dirs()->saveLocation("data", "ScanImages", false);
    QDir olddir(oldloc);
    QString oldpath = olddir.absolutePath();
    bool oldexists = (olddir.exists());

    QString newloc = galleryName;
    QDir newdir(newloc);
    if (newdir.isRelative()) newdir.setPath(docsPath()+QDir::separator()+galleryName);
    QString newpath = newdir.absolutePath();
    bool newexists = (newdir.exists());

    kDebug() << "old" << oldpath << "exists" << oldexists;
    kDebug() << "new" << newpath << "exists" << newexists;

    QString dir;

    if (!oldexists && !newexists)			// no directories present
    {
        dir = createGallery(newdir);			// create and use new
    }
    else if (!oldexists && newexists)			// only new exists
    {
        dir = newpath;					// fine, just use that
    }
    else if (oldexists && !newexists)			// only old exists
    {
        if (KMessageBox::questionYesNo(NULL,
                                       i18n("<qt>"
                                            "<p>An old Kooka gallery was found at<br>"
                                            "<filename>%1</filename>."
                                            "<p>The preferred new location is now<br>"
                                            "<filename>%2</filename>."
                                            "<p>Do you want to create a new gallery at the new location?",
                                            oldpath, newpath),
                                       i18n("Create New Gallery"),
                                       KStandardGuiItem::yes(), KStandardGuiItem::no(),
                                       "GalleryNoMigrate")==KMessageBox::Yes)
        {						// yes, create new
            bool created;
            dir = createGallery(newdir, &created);
            if (created)				// new created OK
            {
                KMessageBox::information(NULL,
                                         i18n("<qt>"
                                              "<p>Kooka will use the new gallery,<br>"
                                              "<a href=\"file:%1\"><filename>%1</filename></a>."
                                              "<p>If you wish to add the images from your old gallery,<br>"
                                              "<a href=\"file:%2\"><filename>%2</filename></a>,<br>"
                                              "then you may do so by simply copying or moving the files.",
                                              newpath, oldpath),
                                         i18n("New Gallery Created"),
                                         QString::null,
                                         KMessageBox::Notify|KMessageBox::AllowLink);
            }
        }
        else						// no, don't create
        {
            dir = oldpath;				// stay with old location
        }
    }
    else						// both exist
    {
        KMessageBox::information(NULL,
                                 i18n("<qt>"
                                      "<p>Kooka will use the new gallery,<br>"
                                      "<a href=\"file:%1\"><filename>%1</filename></a>."
                                      "<p>If you wish to add the images from your old gallery,<br>"
                                      "<a href=\"file:%2\"><filename>%2</filename></a>,<br>"
                                      "then you may do so by simply copying or moving the files.",
                                      newpath, oldpath),
                                 i18n("Old Gallery Exists"),
                                 "GalleryNoRemind",
                                 KMessageBox::Notify|KMessageBox::AllowLink);
        dir = newpath;					// just use new one
    }

    if (!dir.endsWith("/")) dir += "/";
    kDebug() << "returning" << dir;
    return (dir);
}
