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
#include <qicon.h>
#include <qmimetype.h>
#include <qmimedatabase.h>
#include <qimagewriter.h>

#include <kseparator.h>
#include <klocalizedstring.h>
#include <kconfigskeleton.h>

#include "imageformat.h"
#include "kookasettings.h"
#include "kooka_logging.h"


// The ImageType can be OR-ed here to make a set of possible formats
Q_DECLARE_OPERATORS_FOR_FLAGS(ScanImage::ImageTypes)

// Information for a format.  Static, so only POD allowed.
struct FormatInfo {
    // A semicolon-separated list of MIME types that make up this format.
    const char *mime;
    // An ki18n()'ed help string for the format.
    KLocalizedString helpString;
    // An OR-ed set of image types that this format is recommended for.
    // ScanImage::None means that it is not recommended for any image type.
    ScanImage::ImageTypes recForTypes;
    // An OR-ed set of image types that this format can be used for.
    // ScanImage::None means that it can be used for any image type.
    ScanImage::ImageTypes okForTypes;
};

// Known formats with help text and compatibility recommendations
static struct FormatInfo formats[] =
{
    {
        "image/bmp",					// BMP
        ki18n(
            "<b>Bitmap Picture</b> is a widely used format for images under MS Windows. \
It is suitable for color, grayscale and line art images.\
<p>This format is widely supported but is not recommended, use an open format \
instead."),
        ScanImage::None,
        ScanImage::None
    },

    {
        "image/x-portable-bitmap",			// PBM
        ki18n(
            "<b>Portable Bitmap</b>, as used by Netpbm, is an uncompressed format for line art \
(bitmap) images. Only 1 bit per pixel depth is supported."),
        ScanImage::BlackWhite,
        ScanImage::BlackWhite
    },

    {
        "image/x-portable-graymap",			// PGM
        ki18n(
            "<b>Portable Greymap</b>, as used by Netpbm, is an uncompressed format for grayscale \
images. Only 8 bit per pixel depth is supported."),
        ScanImage::Greyscale,
        ScanImage::Greyscale
    },

    {
        "image/x-portable-pixmap",			// PPM
        ki18n(
            "<b>Portable Pixmap</b>, as used by Netpbm, is an uncompressed format for full color \
images. Only 24 bit per pixel RGB is supported."),
        ScanImage::LowColour | ScanImage::HighColour,
        ScanImage::LowColour | ScanImage::HighColour
    },

    {
        "image/x-pcx;image/vnd.zbrush.pcx",		// PCX
        ki18n(
            "<b>PCX</b> is a lossless compressed format which is often supported by PC imaging \
applications, although it is rather old and unsophisticated.  It is suitable for \
color and grayscale images.\
<p>This format is not recommended, use an open format instead."),
        ScanImage::None,
        ScanImage::None
    },

    {
        "image/x-xbitmap",				// XBM
        ki18n(
            "<b>X Bitmap</b> is often used by the X Window System to store cursor and icon bitmaps.\
<p>Unless required for this purpose, use a general purpose format instead."),
        ScanImage::None,
        ScanImage::BlackWhite
    },

    {
        "image/x-xpixmap",				// XPM
        ki18n(
            "<b>X Pixmap</b> is often used by the X Window System for color icons and other images.\
<p>Unless required for this purpose, use a general purpose format instead."),
        ScanImage::None,
        ScanImage::LowColour | ScanImage::HighColour
    },

    {
        "image/png",					// PNG
        ki18n(
            "<b>Portable Network Graphics</b> is a lossless compressed format designed to be \
portable and extensible. It is suitable for any type of color or grayscale images, \
indexed or true color.\
<p>PNG is an open format which is widely supported."),
        ScanImage::BlackWhite | ScanImage::LowColour | ScanImage::Greyscale | ScanImage::HighColour,
        ScanImage::None
    },

    {
        "image/jpeg",					// JPEG JPG
        ki18n(
            "<b>JPEG</b> is a compressed format suitable for true color or grayscale images. \
It is a lossy format, so it is not recommended for archiving or for repeated loading \
and saving.\
<p>This is an open format which is widely supported."),
        ScanImage::HighColour | ScanImage::Greyscale,
        ScanImage::LowColour | ScanImage::Greyscale | ScanImage::HighColour
    },

    {
        "image/jp2",					// JP2
        ki18n(
            "<b>JPEG 2000</b> was intended as an update to the JPEG format, with the option of \
lossless compression, but so far is not widely supported. It is suitable for true \
color or grayscale images."),
        ScanImage::None,
        ScanImage::LowColour | ScanImage::Greyscale | ScanImage::HighColour
    },

    {
        "image/x-eps",					// EPS EPSF EPSI
        ki18n(
            "<b>Encapsulated PostScript</b> is derived from the PostScript&trade; \
page description language.  Use this format for importing into other \
applications, or to use with (e.g.) TeX."),
        ScanImage::None,
        ScanImage::None
    },

    {
        "image/x-tga",					// TGA
        ki18n(
            "<b>Truevision Targa</b> can store full color images with an alpha channel, and is \
used extensively by animation and video applications.\
<p>This format is not recommended, use an open format instead."),
        ScanImage::None,
        ScanImage::LowColour | ScanImage::Greyscale | ScanImage::HighColour
    },

    {
        "image/gif",					// GIF
        ki18n(					// writing may not be supported
            "<b>Graphics Interchange Format</b> is a popular but patent-encumbered format often \
used for web graphics.  It uses lossless compression with up to 256 colors and \
optional transparency.\
<p>For legal reasons this format is not recommended, use an open format instead."),
        ScanImage::None,
        ScanImage::LowColour | ScanImage::Greyscale
    },

    {
        "image/tiff",					// TIF TIFF
        ki18n(					// writing may not be supported
            "<b>Tagged Image File Format</b> is a versatile and extensible file format widely \
supported by imaging and publishing applications. It supports indexed and true color \
images with alpha transparency.\
<p>Because there are many variations, there may sometimes be compatibility problems. \
Unless required for use with other applications, use an open format instead."),
        ScanImage::BlackWhite | ScanImage::LowColour | ScanImage::Greyscale | ScanImage::HighColour,
        ScanImage::None
    },

    {
        "video/x-mng",					// MNG
        ki18n(
            "<b>Multiple-image Network Graphics</b> is derived from the PNG standard and is \
intended for animated images.  It is an open format suitable for all types of \
images.\
<p>Images produced by a scanner will not be animated, so unless specifically \
required for use with other applications use PNG instead."),
        ScanImage::None,
        ScanImage::None
    },

    {
        "image/x-sgi",					// SGI
        ki18n(
            "This is the <b>Silicon Graphics</b> native image file format, supporting 24 bit \
true color images with optional lossless compression.\
<p>Unless specifically required, use an open format instead."),
        ScanImage::None,
        ScanImage::LowColour | ScanImage::HighColour
    },

    {
        "image/vnd.wap.wbmp",				// WBMP
        ki18n(
            "<b>Wireless Bitmap</b> is a monochrome bitmap format optimised \
for WAP mobile devices.  It supports black and white bitmaps only.\
<p>Unless specifically required, use an general purpose format instead."),
        ScanImage::None,
        ScanImage::BlackWhite
    },

    {
        "image/webp",					// WEBP
        ki18n(
            "<b>WebP</b> is intended to be an open standard for true colour \
images with optional transparency.  It is widely supported by modern \
web browsers and graphics software.\
<p>Various compression methods, both lossy and lossless, are supported. \
Lossy compression is not recommended for archiving or for repeated loading \
and saving."),
        ScanImage::HighColour | ScanImage::Greyscale,
        ScanImage::LowColour | ScanImage::Greyscale | ScanImage::HighColour
    },

    { nullptr, KLocalizedString(), ScanImage::None, ScanImage::None }
};

static QString sLastFormat;				// format last used, whether
							// remembered or not

FormatDialog::FormatDialog(QWidget *parent, ScanImage::ImageType type,
                           bool askForFormat, const ImageFormat &format,
                           bool askForFilename, const QString &filename)
    : DialogBase(parent),
      mFormat(format),					// save these to return, if
      mFilename(filename)				// they are not requested
{
    setObjectName("FormatDialog");

    setModal(true);
    // KDE4 buttons: Ok  Cancel  User1=SelectFormat
    // KF5 buttons:  Ok  Cancel  Yes=SelectFormat
    setButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::Yes);
    setWindowTitle(askForFormat ? i18n("Save Assistant") : i18n("Save Scan"));

    QWidget *page = new QWidget(this);
    setMainWidget(page);

    mHelpLabel = nullptr;
    mSubformatCombo = nullptr;
    mFormatList = nullptr;
    mSubformatLabel = nullptr;
    mDontAskCheck = nullptr;
    mRecOnlyCheck = nullptr;
    mExtensionLabel = nullptr;
    mFilenameEdit = nullptr;

    if (!mFormat.isValid()) askForFormat = true;	// must ask if none
    mWantAssistant = false;

    QGridLayout *gl = new QGridLayout(page);
    gl->setContentsMargins(0, 0, 0, 0);
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
        Q_ASSERT(ski!=nullptr);
        mRecOnlyCheck = new QCheckBox(ski->label(), page);
        mRecOnlyCheck->setToolTip(ski->toolTip());
        mRecOnlyCheck->setChecked(KookaSettings::saverOnlyRecommendedTypes());
        connect(mRecOnlyCheck, &QCheckBox::toggled, this, &FormatDialog::buildFormatList);
        gl->addWidget(mRecOnlyCheck, row, 0, 1, 3, Qt::AlignLeft);
        ++row;

        ski = KookaSettings::self()->saverAlwaysUseFormatItem();
        Q_ASSERT(ski!=nullptr);
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

    if (mFormatList != nullptr)				// have the format selector
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

    if (mFilenameEdit != nullptr) {			// asking for a file name
        mFilenameEdit->setFocus();			// set focus to that
        mFilenameEdit->selectAll();			// highlight for editing
    }
}

void FormatDialog::showExtension(const ImageFormat &format)
{
    if (mExtensionLabel == nullptr) return;		// not showing this
    mExtensionLabel->setText("." + format.extension());	// show extension it will have
}

void FormatDialog::formatSelected(QListWidgetItem *item)
{
    if (mHelpLabel == nullptr) return;			// not showing this

    if (item == nullptr) {					// nothing is selected
        mHelpLabel->setText(i18n("No format selected."));
        setButtonEnabled(QDialogButtonBox::Ok, false);

        mFormatList->clearSelection();
        if (mExtensionLabel != nullptr) {
            mExtensionLabel->setText(".???");
        }
        return;
    }

    mFormatList->setCurrentItem(item);			// focus highlight -> select

    // Locate the help text for the format
    KLocalizedString helptxt;
    const QByteArray mimename = item->data(Qt::UserRole).toByteArray();
    for (const FormatInfo *ip = &formats[0]; ip->mime!=nullptr; ++ip)
    {
        const QList<QByteArray> mimetypes = QByteArray(ip->mime).split(';');
        if (mimetypes.contains(mimename))
        {
            helptxt = ip->helpString;
            break;
        }
    }

    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForName(mimename);
    ImageFormat format = ImageFormat::formatForMime(mime);
    qCDebug(KOOKA_LOG) << "MIME" << mimename << "format" << format;

    if (!helptxt.isEmpty()) {				// found some help text
        mHelpLabel->setText(helptxt.toString());	// set the hint
        check_subformat(format);			// and check subformats
    } else {
        mHelpLabel->setText(i18n("No information is available for this format."));
    }

    if (mDontAskCheck != nullptr) {
        mDontAskCheck->setChecked(ImgSaver::isRememberedFormat(mImageType, format));
    }

    showExtension(format);
    checkValid();
}

// TODO: implement subtypes
void FormatDialog::check_subformat(const ImageFormat &format)
{
    if (mSubformatCombo == nullptr) return;		// not showing this

    mSubformatCombo->setEnabled(false);			// not yet implemented
    mSubformatLabel->setEnabled(false);
}

void FormatDialog::setSelectedFormat(const ImageFormat &format)
{
    if (mFormatList == nullptr) return;			// not showing this

    if (format.isValid())				// valid format to select
    {
        const QMimeType mime = format.mime();
        if (!mime.isValid()) return;

        for (int i = 0; i < mFormatList->count(); ++i) {
            QListWidgetItem *item = mFormatList->item(i);
            if (item == nullptr) {
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
            if (item == nullptr) {
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
    if (mFormatList == nullptr) return (mFormat);		// no UI for this

    QMimeDatabase db;
    const QListWidgetItem *item = mFormatList->currentItem();
    if (item != nullptr) {
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
    if (mFilenameEdit == nullptr) return (mFilename);	// no UI for this
    return (mFilenameEdit->text());
}

QByteArray FormatDialog::getSubFormat() const
{
    return ("");					// Not supported yet...
}

void FormatDialog::checkValid()
{
    bool ok = true;					// so far, anyway

    if (mFormatList != nullptr && mFormatList->selectedItems().count() == 0) ok = false;
    if (mFilenameEdit != nullptr && mFilenameEdit->text().isEmpty()) ok = false;
    setButtonEnabled(QDialogButtonBox::Ok, ok);
}


static const FormatInfo *findKnownFormat(const QMimeType &mime)
{
    for (const FormatInfo *fi = &formats[0]; fi->mime!=nullptr; ++fi)
    {							// search for format info
        const QList<QByteArray> mimetypes = QByteArray(fi->mime).split(';');
        for (const QByteArray &mimetype : mimetypes)
        {
            if (mime.inherits(mimetype)) return (fi);	// matching that MIME type
        }
    }

    return (nullptr);					// nothing found
}


void FormatDialog::buildFormatList(bool recOnly)
{
    if (mFormatList == nullptr) return;			// not showing this
    qCDebug(KOOKA_LOG) << "recOnly" << recOnly << "for type" << mImageType;

    mFormatList->clear();
    const QList<QMimeType> *mimeTypes = ImageFormat::mimeTypes();
    for (const QMimeType &mime : std::as_const(*mimeTypes))
    {							// for all known MIME types
        const FormatInfo *fi = findKnownFormat(mime);	// look for format information
        if (fi==nullptr)				// nothing for that MIME type
        {
            if (recOnly) continue;			// never show for recommended
        }						// but always show otherwise
        else
        {
            ScanImage::ImageTypes okTypes = fi->okForTypes;
            if (okTypes!=0)				// format has allowed types
            {
                if (!(okTypes & mImageType)) continue;	// but not for this image type
            }

            if (recOnly)				// want only recommended types
            {
                if (!(fi->recForTypes & mImageType))	// check for recommended format
                {
                    continue;				// not for this image type
                }
            }
        }

        // add format to list
        QListWidgetItem *item = new QListWidgetItem(QIcon::fromTheme(mime.iconName()),
                mime.comment(), mFormatList);
        // Not sure whether a QMimeType can safely be stored in a
        // QVariant, so storing the MIME type name instead.
        item->setData(Qt::UserRole, mime.name());
        mFormatList->addItem(item);
    }

    formatSelected(nullptr);				// selection has been cleared
}

void FormatDialog::slotOk()
{
    if (mRecOnlyCheck != nullptr) {			// have UI for this
        KookaSettings::setSaverOnlyRecommendedTypes(mRecOnlyCheck->isChecked());
        KookaSettings::self()->save();			// save state of this option
    }
}

void FormatDialog::slotUser1()
{
    mWantAssistant = true;
    accept();
}


/* static */ void FormatDialog::forgetRemembered()
{
    const KConfigSkeletonItem *ski = KookaSettings::self()->saverAlwaysUseFormatItem();
    Q_ASSERT(ski!=nullptr);
    KConfigGroup grp = KookaSettings::self()->config()->group(ski->group());
    grp.deleteGroup();

    KookaSettings::self()->save();			// ensure the config is saved
    KookaSettings::self()->read();			// and internal values are up to date
}


bool FormatDialog::alwaysUseFormat() const
{
    return (mDontAskCheck != nullptr ? mDontAskCheck->isChecked() : false);
}


/* static */ bool FormatDialog::isCompatible(const QMimeType &mime, ScanImage::ImageType type, bool recOnly)
{
    //qCDebug(KOOKA_LOG) << "mime" << mime << "type" << type << "recOnly" << recOnly;
    for (const FormatInfo *ip = &formats[0]; ip->mime!=nullptr; ++ip)
    {							// scan the format descriptions
        if (mime.inherits(ip->mime))			// found format for the MIME type
        {
            const ScanImage::ImageTypes types = (recOnly ? ip->recForTypes : ip->okForTypes);
            if (!recOnly && types==ScanImage::None) return (true);
							// allowed for all formats
            return (types & type);			// check image type mask
        }
    }

    return (false);
}
