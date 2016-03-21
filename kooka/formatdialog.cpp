/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2008-2016 Jonathan Marten <jjm@keelhaul.me.uk>	*
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

#include "formatdialog.h"

#include <qlayout.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qlineedit.h>
#include <qlistwidget.h>
#include <qdebug.h>
#include <qicon.h>
#include <qmimetype.h>
#include <qmimedatabase.h>
#include <qimagewriter.h>

#include <kseparator.h>
#include <klocalizedstring.h>
#include <kconfigskeleton.h>

#include "imageformat.h"
#include "kookasettings.h"


struct formatInfo {
    const char *mime;
    const char *helpString;
    int recForTypes;
// TODO: also need an "allowed for types" field (a superset of recForTypes),
// so that, e.g. PGM or PPM will not be shown for a 1-bit image.
// Make recForTypes and this a QFlags
};

static struct formatInfo formats[] = {
    {
        "image/bmp",                  // BMP
        I18N_NOOP(
            "<b>Bitmap Picture</b> is a widely used format for images under MS Windows. \
It is suitable for color, grayscale and line art images.\
<p>This format is widely supported but is not recommended, use an open format \
instead."),
        0
    },

    {
        "image/x-portable-bitmap",            // PBM
        I18N_NOOP(
            "<b>Portable Bitmap</b>, as used by Netpbm, is an uncompressed format for line art \
(bitmap) images. Only 1 bit per pixel depth is supported."),
        ImageMetaInfo::BlackWhite
    },

    {
        "image/x-portable-graymap",           // PGM
        I18N_NOOP(
            "<b>Portable Greymap</b>, as used by Netpbm, is an uncompressed format for grayscale \
images. Only 8 bit per pixel depth is supported."),
        ImageMetaInfo::Greyscale
    },

    {
        "image/x-portable-pixmap",            // PPM
        I18N_NOOP(
            "<b>Portable Pixmap</b>, as used by Netpbm, is an uncompressed format for full color \
images. Only 24 bit per pixel RGB is supported."),
        ImageMetaInfo::LowColour | ImageMetaInfo::HighColour
    },

    {
        "image/x-pcx",                    // PCX
        I18N_NOOP(
            "<b>PCX</b> is a lossless compressed format which is often supported by PC imaging \
applications, although it is rather old and unsophisticated.  It is suitable for \
color and grayscale images.\
<p>This format is not recommended, use an open format instead."),
        0
    },

    {
        "image/x-xbitmap",                // XBM
        I18N_NOOP(
            "<b>X Bitmap</b> is often used by the X Window System to store cursor and icon bitmaps.\
<p>Unless required for this purpose, use a general purpose format instead."),
        0
    },

    {
        "image/x-xpixmap",                // XPM
        I18N_NOOP(
            "<b>X Pixmap</b> is often used by the X Window System for color icons and other images.\
<p>Unless required for this purpose, use a general purpose format instead."),
        0
    },

    {
        "image/png",                  // PNG
        I18N_NOOP(
            "<b>Portable Network Graphics</b> is a lossless compressed format designed to be \
portable and extensible. It is suitable for any type of color or grayscale images, \
indexed or true color.\
<p>PNG is an open format which is widely supported."),
        ImageMetaInfo::BlackWhite | ImageMetaInfo::LowColour | ImageMetaInfo::Greyscale | ImageMetaInfo::HighColour
    },

    {
        "image/jpeg",                 // JPEG JPG
        I18N_NOOP(
            "<b>JPEG</b> is a compressed format suitable for true color or grayscale images. \
It is a lossy format, so it is not recommended for archiving or for repeated loading \
and saving.\
<p>This is an open format which is widely supported."),
        ImageMetaInfo::HighColour | ImageMetaInfo::Greyscale
    },

    {
        "image/jp2",                  // JP2
        I18N_NOOP(
            "<b>JPEG 2000</b> was intended as an update to the JPEG format, with the option of \
lossless compression, but so far is not widely supported. It is suitable for true \
color or grayscale images."),
        0
    },

    {
        "image/x-eps",                    // EPS EPSF EPSI
        I18N_NOOP(
            "<b>Encapsulated PostScript</b> is derived from the PostScript&trade; \
page description language.  Use this format for importing into other \
applications, or to use with (e.g.) TeX."),
        0
    },

    {
        "image/x-tga",                    // TGA
        I18N_NOOP(
            "<b>Truevision Targa</b> can store full color images with an alpha channel, and is \
used extensively by animation and video applications.\
<p>This format is not recommended, use an open format instead."),
        0
    },

    {
        "image/gif",                  // GIF
        I18N_NOOP(     // writing may not be supported
            "<b>Graphics Interchange Format</b> is a popular but patent-encumbered format often \
used for web graphics.  It uses lossless compression with up to 256 colors and \
optional transparency.\
<p>For legal reasons this format is not recommended, use an open format instead."),
        0
    },

    {
        "image/tiff",                 // TIF TIFF
        I18N_NOOP(     // writing may not be supported
            "<b>Tagged Image File Format</b> is a versatile and extensible file format widely \
supported by imaging and publishing applications. It supports indexed and true color \
images with alpha transparency.\
<p>Because there are many variations, there may sometimes be compatibility problems. \
Unless required for use with other applications, use an open format instead."),
        ImageMetaInfo::BlackWhite | ImageMetaInfo::LowColour | ImageMetaInfo::Greyscale | ImageMetaInfo::HighColour
    },

    {
        "video/x-mng",                    // MNG
        I18N_NOOP(
            "<b>Multiple-image Network Graphics</b> is derived from the PNG standard and is \
intended for animated images.  It is an open format suitable for all types of \
images.\
<p>Images produced by a scanner will not be animated, so unless specifically \
required for use with other applications use PNG instead."),
        0
    },

    {
        "image/x-sgi",                    // SGI
        I18N_NOOP(
            "This is the <b>Silicon Graphics</b> native image file format, supporting 24 bit \
true color images with optional lossless compression.\
<p>Unless specifically required, use an open format instead."),
        0
    },

    { NULL, NULL, 0 }
};

static QString sLastFormat = QString::null;		// format last used, whether
							// remembered or not

FormatDialog::FormatDialog(QWidget *parent, ImageMetaInfo::ImageType type,
                           bool askForFormat, const ImageFormat &format,
                           bool askForFilename, const QString &filename)
    : DialogBase(parent),
      mFormat(format),					// save these to return, if
      mFilename(filename)				// they are not requested
{
    setObjectName("FormatDialog");

    //qDebug();

    setModal(true);
    // KDE4 buttons: Ok  Cancel  User1=Select Format
    // KF5 buttons:  Ok  Cancel  Yes=SelectFormat
    setButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::Yes);
    setWindowTitle(askForFormat ? i18n("Save Assistant") : i18n("Save Scan"));

    setButtonSeparatorShown(false);			// we'll add our own later,
							// otherwise length isn't consistent
    QWidget *page = new QWidget(this);
    setMainWidget(page);

    mHelpLabel = NULL;
    mSubformatCombo = NULL;
    mFormatList = NULL;
    mSubformatLabel = NULL;
    mDontAskCheck = NULL;
    mRecOnlyCheck = NULL;
    mExtensionLabel = NULL;
    mFilenameEdit = NULL;

    if (!mFormat.isValid()) askForFormat = true;	// must ask if none
    mWantAssistant = false;

    QGridLayout *gl = new QGridLayout(page);
    gl->setMargin(0);
    int row = 0;

    QLabel *l1;
    KSeparator *sep;

    if (askForFormat)					// format selector section
    {
        l1 = new QLabel(xi18nc("@info", "Select a format to save the scanned image.<nl/>This is a <emphasis strong=\"1\">%1</emphasis>.",
                               ImgSaver::picTypeAsString(type)), page);
        gl->addWidget(l1, row, 0, 1, 3);
        ++row;

        sep = new KSeparator(Qt::Horizontal, page);
        gl->addWidget(sep, row, 0, 1, 3);
        ++row;

        // Insert scrolled list for formats
        l1 = new QLabel(i18n("File format:"), page);
        gl->addWidget(l1, row, 0, Qt::AlignLeft);

        mFormatList = new QListWidget(page);

        // TODO: the output from all this is mMimeTypes, so move to ImageFormat
        // and make it a once-only initialisation.

        // Each of the lists that we get from Qt (supported image formats and
        // supported MIME types) is sorted alphabetically.  That means that
        // there is no correlation between the two lists.  The formats list
        // may also contain duplicates, e.g. "jpeg" and "jpg" both correspond
        // to MIME type image/jpeg.
        //
        // However, it appears that the Qt image format is actually a file
        // name extension.  That means that we can obtain the MIME type by
        // finding the type for a file with that extension.

        QList<QByteArray> supportedFormats = QImageWriter::supportedImageFormats();
        //qDebug() << "have" << supportedFormats.count() << "image formats:" << supportedFormats;
        QList<QByteArray> supportedTypes = QImageWriter::supportedMimeTypes();
        //qDebug() << "have" << supportedTypes.count() << "mime types:" << supportedTypes;

        // Although the format list from standard Qt 5 (unlike Qt 4) does not
        // appear to contain any duplicates or mixed case, it is always possible
        // that a plugin could introduce some.  So the apparently pointless loop
        // here filters the list.
        QList<QByteArray> formatList;
        foreach (const QByteArray &format, supportedFormats)
        {
            const QByteArray f = format.toLower();
            if (!formatList.contains(f)) formatList.append(f);
        }
        qDebug() << "have" << formatList.count() << "image types"
                 << "from" << supportedFormats.count() << "supported";

        // Even after filtering the list as above, there will be MIME type
        // duplicates (e.g. JPG and JPEG both map to image/jpeg and produce
        // the same results).  So the list is filtered again to eliminate
        // duplicate MIME types.
        //
        // As a side effect, this completely eliminates any formats that do
        // not have a defined MIME type.  None of those affected (currently
        // BW, RGBA, XV) seem to be of any use.

        QMimeDatabase db;
        mMimeTypes.clear();
        foreach (const QByteArray &format, formatList)
        {
            QMimeType mime = db.mimeTypeForFile(QString("a.")+format, QMimeDatabase::MatchExtension);
            if (!mime.isValid() || mime.isDefault())
            {
                qDebug() << "No MIME type for format" << format;
                continue;
            }

            // ImageFormat::formatForMime() should now work even in the presence
            // of MIME aliases, but double check that it works at this stage.
            ImageFormat fmt = ImageFormat::formatForMime(mime);
            if (!fmt.isValid())
            {
                qDebug() << "MIME type" << mime.name() << "does not map back to format" << format;
                continue;
            }

            bool seen = false;
            foreach (const QMimeType &mt, mMimeTypes)
            {
                if (mime.inherits(mt.name()))
                {
                    seen = true;
                    break;
                }
            }

            if (!seen) mMimeTypes.append(mime);
        }
        qDebug() << "have" << mMimeTypes.count() << "unique MIME types";

        // The list box is filled later.

        mImageType = type;
        connect(mFormatList, &QListWidget::currentItemChanged, this, &FormatDialog::formatSelected);
        l1->setBuddy(mFormatList);
        gl->addWidget(mFormatList, row + 1, 0);
        gl->setRowStretch(row + 1, 1);

        // Insert label for help text
        mHelpLabel = new QLabel(page);
        mHelpLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
        mHelpLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        mHelpLabel->setMinimumSize(230, 200);
        mHelpLabel->setWordWrap(true);
        mHelpLabel->setMargin(4);
        gl->addWidget(mHelpLabel, row, 1, 4, 2);

        // Insert selection box for subformat
        mSubformatLabel = new QLabel(i18n("Image sub-format:"), page);
        mSubformatLabel->setEnabled(false);
        gl->addWidget(mSubformatLabel, row + 2, 0, Qt::AlignLeft);

        mSubformatCombo = new QComboBox(page);
        mSubformatCombo->setEnabled(false);     // not yet implemented
        gl->addWidget(mSubformatCombo, row + 3, 0);
        mSubformatLabel->setBuddy(mSubformatCombo);
        row += 4;

        sep = new KSeparator(Qt::Horizontal, page);
        gl->addWidget(sep, row, 0, 1, 3);
        ++row;

        // Checkbox to store setting
        const KConfigSkeletonItem *ski = KookaSettings::self()->saverOnlyRecommendedTypesItem();
        Q_ASSERT(ski!=NULL);
        mRecOnlyCheck = new QCheckBox(ski->label(), page);
        mRecOnlyCheck->setToolTip(ski->toolTip());
        mRecOnlyCheck->setChecked(KookaSettings::saverOnlyRecommendedTypes());
        connect(mRecOnlyCheck, &QCheckBox::toggled, this, &FormatDialog::buildFormatList);
        gl->addWidget(mRecOnlyCheck, row, 0, 1, 3, Qt::AlignLeft);
        ++row;

        ski = KookaSettings::self()->saverAlwaysUseFormatItem();
        Q_ASSERT(ski!=NULL);
        mDontAskCheck  = new QCheckBox(ski->label(), page);
        mDontAskCheck->setToolTip(ski->toolTip());
        gl->addWidget(mDontAskCheck, row, 0, 1, 3, Qt::AlignLeft);
        ++row;

        buildFormatList(mRecOnlyCheck->isChecked());	// now have this setting
							// don't want this button
        buttonBox()->button(QDialogButtonBox::Yes)->setVisible(false);
    }

    gl->setColumnStretch(1, 1);
    gl->setColumnMinimumWidth(1, horizontalSpacing());

    if (askForFormat && askForFilename) {
        sep = new KSeparator(Qt::Horizontal, page);
        gl->addWidget(sep, row, 0, 1, 3);
        ++row;
    }

    if (askForFilename) {				// file name section
        l1 = new QLabel(i18n("File name:"), page);
        gl->addWidget(l1, row, 0, 1, 3);
        ++row;

        mFilenameEdit = new QLineEdit(filename, page);
        connect(mFilenameEdit, &QLineEdit::textChanged, this, &FormatDialog::checkValid);
        l1->setBuddy(mFilenameEdit);
        gl->addWidget(mFilenameEdit, row, 0, 1, 2);

        mExtensionLabel = new QLabel("", page);
        gl->addWidget(mExtensionLabel, row, 2, Qt::AlignLeft);
        ++row;

        if (!askForFormat) {
            buttonBox()->button(QDialogButtonBox::Yes)->setText(i18n("Select Format..."));
        }
    }

    sep = new KSeparator(Qt::Horizontal, page);
    gl->addWidget(sep, row, 0, 1, 3);
    ++row;

    if (mFormatList != NULL)				// have the format selector
    {
        setSelectedFormat(format);			// preselect the remembered format
    }
    else						// no format selector, but
    {							// asking for a file name
        showExtension(format);				// show extension it will have
    }

    connect(buttonBox()->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &FormatDialog::slotOk);
    connect(buttonBox()->button(QDialogButtonBox::Yes), &QPushButton::clicked, this, &FormatDialog::slotUser1);
}

void FormatDialog::showEvent(QShowEvent *ev)
{
    DialogBase::showEvent(ev);

    if (mFilenameEdit != NULL) {			// asking for a file name
        mFilenameEdit->setFocus();			// set focus to that
        mFilenameEdit->selectAll();			// highight for editing
    }
}

void FormatDialog::showExtension(const ImageFormat &format)
{
    if (mExtensionLabel == NULL) return;		// not showing this
    mExtensionLabel->setText("." + format.extension());	// show extension it will have
}

void FormatDialog::formatSelected(QListWidgetItem *item)
{
    if (mHelpLabel == NULL) return;			// not showing this

    if (item == NULL) {					// nothing is selected
        mHelpLabel->setText(i18n("No format selected."));
        setButtonEnabled(QDialogButtonBox::Ok, false);

        mFormatList->clearSelection();
        if (mExtensionLabel != NULL) {
            mExtensionLabel->setText(".???");
        }
        return;
    }

    mFormatList->setCurrentItem(item);			// focus highlight -> select

    const char *helptxt = NULL;
    const QString mimename = item->data(Qt::UserRole).toString();
    for (formatInfo *ip = &formats[0]; ip->mime != NULL; ++ip) {
        // locate help text for format
        if (ip->mime == mimename) {
            helptxt = ip->helpString;
            break;
        }
    }

    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForName(mimename);
    ImageFormat format = ImageFormat::formatForMime(mime);

    if (helptxt != NULL) {				// found some help text
        mHelpLabel->setText(i18n(helptxt));		// set the hint
        check_subformat(format);			// and check subformats
    } else {
        mHelpLabel->setText(i18n("No information is available for this format."));
    }

    if (mDontAskCheck != NULL) {
        mDontAskCheck->setChecked(ImgSaver::isRememberedFormat(mImageType, format));
    }

    showExtension(format);
    checkValid();
}

void FormatDialog::check_subformat(const ImageFormat &format)
{
    if (mSubformatCombo == NULL) return;		// not showing this

    mSubformatCombo->setEnabled(false);			// not yet implemented
    mSubformatLabel->setEnabled(false);
}

void FormatDialog::setSelectedFormat(const ImageFormat &format)
{
    if (mFormatList == NULL) return;			// not showing this

    if (format.isValid()) {				// valid format to select
        const QMimeType mime = format.mime();
        if (!mime.isValid()) return;

        for (int i = 0; i < mFormatList->count(); ++i) {
            QListWidgetItem *item = mFormatList->item(i);
            if (item == NULL) {
                continue;
            }
            QString mimename = item->data(Qt::UserRole).toString();
            if (mime.inherits(mimename)) {
                mFormatList->setCurrentItem(item);
                return;
            }
        }
    }

    // If that selected nothing, then select the last-used format (regardless
    // of the "always use" option setting).  The last-used format is saved
    // in getFormat() for that purpose.  This helps when scanning a series of
    // similar images yet where the user doesn't want to permanently save the
    // format for some reason.
    //
    // This is safe if the last-used format is not applicable to and so is not
    // displayed for the current image type - it just does nothing.
    if (!sLastFormat.isEmpty()) {
        for (int i = 0; i < mFormatList->count(); ++i) {
            QListWidgetItem *item = mFormatList->item(i);
            if (item == NULL) {
                continue;
            }
            QString mimename = item->data(Qt::UserRole).toString();
            // We know the MIME name is canonical here, so can use string compare
            if (mimename == sLastFormat) {
                mFormatList->setCurrentItem(item);
                return;
            }
        }
    }
}

ImageFormat FormatDialog::getFormat() const
{
    if (mFormatList == NULL) return (mFormat);		// no UI for this

    QMimeDatabase db;
    const QListWidgetItem *item = mFormatList->currentItem();
    if (item != NULL) {
        QString mimename = item->data(Qt::UserRole).toString();
        const QMimeType mime = db.mimeTypeForName(mimename);
        if (mime.isValid()) {
            sLastFormat = mime.name();
            return (ImageFormat::formatForMime(mime));
        }
    }

    return (ImageFormat("PNG"));			// a sensible default
}

QString FormatDialog::getFilename() const
{
    if (mFilenameEdit == NULL) return (mFilename);	// no UI for this
    return (mFilenameEdit->text());
}

QByteArray FormatDialog::getSubFormat() const
{
    return ("");					// Not supported yet...
}

void FormatDialog::checkValid()
{
    bool ok = true;					// so far, anyway

    if (mFormatList != NULL && mFormatList->selectedItems().count() == 0) ok = false;
    if (mFilenameEdit != NULL && mFilenameEdit->text().isEmpty()) ok = false;
    setButtonEnabled(QDialogButtonBox::Ok, ok);
}

void FormatDialog::buildFormatList(bool recOnly)
{
    if (mFormatList == NULL) return;			// not showing this
    qDebug() << "recOnly" << recOnly << "for type" << mImageType;

    mFormatList->clear();
    foreach (const QMimeType &mime, mMimeTypes)
    {
        if (recOnly) {					// only want recommended
            bool formatOk = false;
            for (formatInfo *ip = &formats[0]; ip->mime != NULL; ++ip) {
                // search for this format
                if (!mime.inherits(ip->mime)) continue;

                if (ip->recForTypes & mImageType) {	// recommended for this type?
                    formatOk = true;			// this format to be shown
                    break;				// no more to do
                }
            }

            if (!formatOk) continue;			// this format not to be shown
        }

        // add format to list
        QListWidgetItem *item = new QListWidgetItem(QIcon::fromTheme(mime.iconName()),
                mime.comment(), mFormatList);
        // Not sure whether a QMimeType can safely be stored in a
        // QVariant, so storing the MIME type name instead.
        item->setData(Qt::UserRole, mime.name());
        mFormatList->addItem(item);
    }

    formatSelected(NULL);				// selection has been cleared
}

void FormatDialog::slotOk()
{
    if (mRecOnlyCheck != NULL) {			// have UI for this
        KookaSettings::setSaverOnlyRecommendedTypes(mRecOnlyCheck->isChecked());
        KookaSettings::self()->save();			// save state of this option
    }
}

void FormatDialog::slotUser1()
{
    mWantAssistant = true;
    accept();
}

void FormatDialog::forgetRemembered()
{
    const KConfigSkeletonItem *ski = KookaSettings::self()->saverOnlyRecommendedTypesItem();
    Q_ASSERT(ski!=NULL);
    KConfigGroup grp = KookaSettings::self()->config()->group(ski->group());
    grp.deleteGroup();
    grp.sync();
}


bool FormatDialog::alwaysUseFormat() const
{
    return (mDontAskCheck != NULL ? mDontAskCheck->isChecked() : false);
}
