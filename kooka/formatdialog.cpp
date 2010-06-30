/***************************************************************************
 *                                                                         *
 *  This file may be distributed and/or modified under the terms of the    *
 *  GNU General Public License version 2 as published by the Free Software *
 *  Foundation and appearing in the file COPYING included in the           *
 *  packaging of this file.                                                *
 *
 *  As a special exception, permission is given to link this program       *
 *  with any version of the KADMOS ocr/icr engine of reRecognition GmbH,   *
 *  Kreuzlingen and distribute the resulting executable without            *
 *  including the source code for KADMOS in the source distribution.       *
 *
 *  As a special exception, permission is given to link this program       *
 *  with any edition of Qt, and distribute the resulting executable,       *
 *  without including the source code for Qt in the source distribution.   *
 *                                                                         *
 ***************************************************************************/

#include "formatdialog.h"
#include "formatdialog.moc"

#include <qlayout.h>
#include <qlabel.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include <qlistwidget.h>

#include <kdialog.h>
#include <kseparator.h>
#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kimageio.h>

#include "imageformat.h"


struct formatInfo
{
    const char *mime;
    const char *helpString;
    int recForTypes;
};


static struct formatInfo formats[] =
{
    { "image/bmp",					// BMP
      I18N_NOOP(
"<b>Bitmap Picture</b> is a widely used format for images under MS Windows. \
It is suitable for color, grayscale and line art images.\
<p>This format is widely supported but is not recommended, use an open format \
instead."),
      ImgSaver::ImgNone },

    { "image/x-portable-bitmap",			// PBM
      I18N_NOOP(
"<b>Portable Bitmap</b>, as used by Netpbm, is an uncompressed format for line art \
(bitmap) images. Only 1 bit per pixel depth is supported."),
      ImgSaver::ImgBW },

    { "image/x-portable-graymap",			// PGM
      I18N_NOOP(
"<b>Portable Greymap</b>, as used by Netpbm, is an uncompressed format for grayscale \
images. Only 8 bit per pixel depth is supported."),
      ImgSaver::ImgGray },

    { "image/x-portable-pixmap",			// PPM
      I18N_NOOP(
"<b>Portable Pixmap</b>, as used by Netpbm, is an uncompressed format for full color \
images. Only 24 bit per pixel RGB is supported."),
      ImgSaver::ImgColor|ImgSaver::ImgHicolor },

    { "image/x-pcx",					// PCX
      I18N_NOOP(
"This is a lossless compressed format which is often supported by PC imaging \
applications, although it is rather old and unsophisticated.  It is suitable for \
color and grayscale images.\
<p>This format is not recommended, use an open format instead."),
      ImgSaver::ImgNone },

    { "image/x-xbitmap",				// XBM
      I18N_NOOP(
"<b>X Bitmap</b> is often used by the X Window System to store cursor and icon bitmaps.\
<p>Unless required for this purpose, use a general purpose format instead."),
      ImgSaver::ImgNone },

    { "image/x-xpixmap",				// XPM
      I18N_NOOP(
"<b>X Pixmap</b> is often used by the X Window System for color icons and other images.\
<p>Unless required for this purpose, use a general purpose format instead."),
      ImgSaver::ImgNone },

    { "image/png",					// PNG
      I18N_NOOP(
"<b>Portable Network Graphics</b> is a lossless compressed format designed to be \
portable and extensible. It is suitable for any type of color or grayscale images, \
indexed or true color.\
<p>PNG is an open format which is widely supported."),
      ImgSaver::ImgBW|ImgSaver::ImgColor|ImgSaver::ImgGray|ImgSaver::ImgHicolor },

    { "image/jpeg",					// JPEG JPG
      I18N_NOOP(
"<b>JPEG</b> is a compressed format suitable for true color or grayscale images. \
It is a lossy format, so it is not recommended for archiving or for repeated loading \
and saving.\
<p>This is an open format which is widely supported."),
      ImgSaver::ImgHicolor|ImgSaver::ImgGray },

    { "image/jp2",					// JP2
      I18N_NOOP(
"<b>JPEG 2000</b> was intended as an update to the JPEG format, with the option of \
lossless compression, but so far is not widely supported. It is suitable for true \
color or grayscale images."),
      ImgSaver::ImgNone },

    { "image/x-eps",					// EPS EPSF EPSI
      I18N_NOOP(
"<b>Encapsulated PostScript</b> is derived from the PostScript&trade; \
page description language.  Use this format for importing into other \
applications, or to use with (e.g.) TeX."),
      ImgSaver::ImgNone },

    { "image/x-tga",					// TGA
      I18N_NOOP(
"<b>Truevision Targa</b> can store full colour images with an alpha channel, and is \
used extensively by animation and video applications.\
<p>This format is not recommended, use an open format instead."),
      ImgSaver::ImgNone },

    { "image/gif",					// GIF
      I18N_NOOP(					// writing may not be supported
"<b>Graphics Interchange Format</b> is a popular but patent-encumbered format often \
used for web graphics.  It uses lossless compression with up to 256 colors and \
optional transparency.\
<p>For legal reasons this format is not recommended, use an open format instead."),
      ImgSaver::ImgNone },

    { "image/tiff",					// TIF TIFF
      I18N_NOOP(					// writing may not be supported
"<b>Tagged Image File Format</b> is a versatile and extensible file format widely \
supported by imaging and publishing applications. It supports indexed and true color \
images with alpha transparency.\
<p>Because there are many variations, there may sometimes be compatibility problems.\
Unless required for use with other applications, use an open format instead."),
      ImgSaver::ImgBW|ImgSaver::ImgColor|ImgSaver::ImgGray|ImgSaver::ImgHicolor },

    { "video/x-mng",					// MNG
      I18N_NOOP(
"<b>Multiple-image Network Graphics</b> is derived from the PNG standard and is \
intended for animated images.  It is an open format suitable for all types of \
images.\
<p>Images produced by a scanner will not be animated, so unless specifically \
required for use with other applications use PNG instead."),
      ImgSaver::ImgNone },

    { "image/x-sgi",					// SGI
      I18N_NOOP(
"This is the <b>Silicon Graphics</b> native image file format, supporting 24 bit \
true colour images with optional lossless compression.\
<p>Unless specifically required, use an open format instead."),
      ImgSaver::ImgNone },

    { NULL, NULL, ImgSaver::ImgNone }
};



FormatDialog::FormatDialog(QWidget *parent, ImgSaver::ImageType type,
                           bool askForFormat, const ImageFormat &format,
                           bool askForFilename, const QString &filename)
    : KDialog(parent),
      mFormat(format),					// save these to return, if
      mFilename(filename)				// they are not requested
{
    setObjectName("FormatDialog");

    kDebug();

    setModal(true);
    setButtons(KDialog::Ok|KDialog::Cancel|KDialog::User1);
    setCaption(askForFormat ? i18n("Save Assistant") : i18n("Save Scan"));
    showButtonSeparator(false);				// we'll add our own later,
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
    int row = 0;

    QLabel *l1;
    KSeparator *sep;

    if (askForFormat)					// format selector section
    {
        l1 = new QLabel(i18n("<qt>Select a format to save the scanned image.<br>This is a <b>%1</b>.",
                             ImgSaver::picTypeAsString(type)), page);
        gl->addWidget(l1, row, 0, 1, 3);
        ++row;

        sep = new KSeparator(Qt::Horizontal, page);
        gl->addWidget(sep, row, 0, 1, 3);
        ++row;

        // Insert scrolled list for formats
        l1 = new QLabel(i18n("File format:"),page);
        gl->addWidget(l1,row,0,Qt::AlignLeft);

        mFormatList = new QListWidget(page);

        // Clean up the list that we get from KImageIO (=Qt).  The raw list
        // contains mixed case, spaces and duplicates.
        QStringList supportedTypes = KImageIO::types(KImageIO::Writing);

        QStringList formatList;
        for (QStringList::const_iterator it = supportedTypes.constBegin();
             it!=supportedTypes.constEnd(); ++it)
        {
            QString type = (*it).toUpper().trimmed();
            if (!formatList.contains(type)) formatList.append(type);
        }
        kDebug() << "have" << formatList.count() << "image types"
                 << "from" << supportedTypes.count() << "supported";

        // Even after filtering the list as above, there will be MIME type
        // duplicates (e.g. JPG and JPEG both map to image/jpeg and produce
        // the same results).  So the list is filtered again to eliminate
        // duplicate MIME types.
        //
        // As a side effect, this completely eliminates any formats that do
        // not have a defined KDE MIME type.  None of those affected (currently
        // BW, RGBA, XV) seem to be of any use.
        mMimeTypes.clear();
        for (QStringList::const_iterator it = formatList.constBegin();
             it!=formatList.constEnd(); ++it)
        {
            const KMimeType::Ptr mime = ImageFormat((*it).toLocal8Bit()).mime();
            if (mime.isNull()) continue;
            if (mime->isDefault()) continue;

            bool seen = false;
            for (KMimeType::List::const_iterator it = mMimeTypes.constBegin();
                 it!=mMimeTypes.constEnd(); ++it)
            {
                KMimeType::Ptr p = (*it);
                if (mime->is(p->name()))
                {
                    seen = true;
                    break;
                }
            }

            if (!seen) mMimeTypes.append(mime);
        }
        kDebug() << "have" << mMimeTypes.count() << "unique MIME types";

        // The list box is filled later.

        mImageType = type;
        connect(mFormatList, SIGNAL(currentItemChanged(QListWidgetItem *,QListWidgetItem *)), SLOT(formatSelected(QListWidgetItem *)));
        l1->setBuddy(mFormatList);
        gl->addWidget(mFormatList,row+1,0);
        gl->setRowStretch(row+1,1);

        // Insert label for help text
        mHelpLabel = new QLabel(page);
        mHelpLabel->setFrameStyle(QFrame::Panel|QFrame::Sunken);
        mHelpLabel->setAlignment(Qt::AlignLeft|Qt::AlignTop);
        mHelpLabel->setMinimumSize(230,200);
        mHelpLabel->setWordWrap(true);
        mHelpLabel->setMargin(4);
        gl->addWidget(mHelpLabel, row, 1, 4, 2);

        // Insert selection box for subformat
        mSubformatLabel = new QLabel(i18n("Image sub-format:"),page);
        mSubformatLabel->setEnabled(false);
        gl->addWidget(mSubformatLabel,row+2,0,Qt::AlignLeft);

        mSubformatCombo = new QComboBox(page);
        mSubformatCombo->setEnabled(false);		// not yet implemented
        gl->addWidget(mSubformatCombo,row+3,0);
        mSubformatLabel->setBuddy(mSubformatCombo);
        row += 4;

        sep = new KSeparator(Qt::Horizontal, page);
        gl->addWidget(sep, row, 0, 1, 3);
        ++row;

        // Checkbox to store setting
        mRecOnlyCheck = new QCheckBox(i18n("Only show recommended formats for this image type"),page);
        gl->addWidget(mRecOnlyCheck, row, 0, 1, 3, Qt::AlignLeft);
        ++row;

        KConfigGroup grp = KGlobal::config()->group(OP_SAVER_GROUP);
        mRecOnlyCheck->setChecked(grp.readEntry(OP_SAVER_REC_FMT, true));
        connect(mRecOnlyCheck, SIGNAL(toggled(bool)), SLOT(buildFormatList(bool)));

        mDontAskCheck  = new QCheckBox(i18n("Always use this save format for this image type"),page);
        gl->addWidget(mDontAskCheck, row, 0, 1, 3, Qt::AlignLeft);
        ++row;

        buildFormatList(mRecOnlyCheck->isChecked());	// now have this setting

        showButton(KDialog::User1, false);		// don't want this button
    }

    gl->setColumnStretch(1, 1);
    gl->setColumnMinimumWidth(1, KDialog::marginHint());

    if (askForFormat && askForFilename)
    {
        sep = new KSeparator(Qt::Horizontal, page);
        gl->addWidget(sep, row, 0, 1, 3);
        ++row;
    }

    if (askForFilename)					// file name section
    {
        l1 = new QLabel(i18n("File name:"), page);
        gl->addWidget(l1, row, 0, 1, 3);
        ++row;

        mFilenameEdit = new QLineEdit(filename, page);
        connect(mFilenameEdit, SIGNAL(textChanged(const QString &)), SLOT(checkValid()));
        l1->setBuddy(mFilenameEdit);
        gl->addWidget(mFilenameEdit, row, 0, 1, 2);

        mExtensionLabel = new QLabel("",page);
        gl->addWidget(mExtensionLabel, row, 2, Qt::AlignLeft);
        ++row;

        if (!askForFormat) setButtonText(KDialog::User1, i18n("Select Format..."));
    }

    sep = new KSeparator(Qt::Horizontal, page);
    gl->addWidget(sep, row, 0, 1, 3);
    ++row;

    if (mFormatList!=NULL)				// have the format selector
    {							// preselect the remembered format
        setSelectedFormat(format);
    }
    else						// no format selector, but
    {							// asking for a file name
        showExtension(format);				// show extension it will have
    }

    connect(this, SIGNAL(okClicked()), SLOT(slotOk()));
    connect(this, SIGNAL(user1Clicked()), SLOT(slotUser1()));
}


void FormatDialog::showEvent(QShowEvent *ev)
{
    KDialog::showEvent(ev);

    if (mFilenameEdit!=NULL)				// asking for a file name
    {
        mFilenameEdit->setFocus();			// set focus to that
        mFilenameEdit->selectAll();			// highight for editing
    }
}


void FormatDialog::showExtension(const ImageFormat &format)
{
    if (mExtensionLabel==NULL) return;			// not showing this
    mExtensionLabel->setText("."+format.extension());	// show extension it will have
}


void FormatDialog::formatSelected(QListWidgetItem *item)
{
    if (mHelpLabel==NULL) return;			// not showing this

    if (item==NULL)					// nothing is selected
    {
	mHelpLabel->setText(i18n("No format selected."));
	enableButtonOk(false);

        mFormatList->clearSelection();
        if (mExtensionLabel!=NULL) mExtensionLabel->setText(".???");
	return;
    }

    mFormatList->setCurrentItem(item);			// focus highlight -> select

    const char *helptxt = NULL;

    QString mimename = item->data(Qt::UserRole).toString();
    KMimeType::Ptr mime = KMimeType::mimeType(mimename);
    if (!mime.isNull())
    {
        QString imime = mime->name();
        for (formatInfo *ip = &formats[0]; ip->mime!=NULL; ++ip)
        {						// locate help text for format
            if (ip->mime==mimename)
            {
                helptxt = ip->helpString;
                break;
            }
        }
    }

    ImageFormat format = ImageFormat::formatForMime(mime);

    if (helptxt!=NULL)					// found some help text
    {
	mHelpLabel->setText(i18n(helptxt));		// set the hint
	check_subformat(format);			// and check subformats
    }
    else mHelpLabel->setText(i18n("No information is available for this format."));

    if (mDontAskCheck!=NULL) mDontAskCheck->setChecked(ImgSaver::isRememberedFormat(mImageType, format));

    showExtension(format);
    checkValid();
}


void FormatDialog::check_subformat(const ImageFormat &format)
{
    if (mSubformatCombo==NULL) return;			// not showing this
    mSubformatCombo->setEnabled(false);			// not yet implemented
    mSubformatLabel->setEnabled(false);
}


void FormatDialog::setSelectedFormat(const ImageFormat &format)
{
    if (mFormatList==NULL) return;			// not showing this

    const KMimeType::Ptr ptr = format.mime();
    if (ptr.isNull()) return;

    for (int i = 0; i<mFormatList->count(); ++i)
    {
        QListWidgetItem *item = mFormatList->item(i);
        if (item==NULL) continue;
        QString mimename = item->data(Qt::UserRole).toString();
        if (ptr->is(mimename))
        {
            mFormatList->setCurrentItem(item);
            return;
        }
    }
}


ImageFormat FormatDialog::getFormat() const
{
    if (mFormatList==NULL) return (mFormat);		// no UI for this

    const QListWidgetItem *item = mFormatList->currentItem();
    if (item!=NULL)
    {
        QString mimename = item->data(Qt::UserRole).toString();
        const KMimeType::Ptr mime = KMimeType::mimeType(mimename);
        if (!mime.isNull()) return (ImageFormat::formatForMime(mime));
    }

    return (ImageFormat("PNG"));			// a sensible default
}


QString FormatDialog::getFilename() const
{
    if (mFilenameEdit==NULL) return (mFilename);	// no UI for this
    return (mFilenameEdit->text());
}


QByteArray FormatDialog::getSubFormat() const
{
   return ("");						// Not supported yet...
}


void FormatDialog::checkValid()
{
    bool ok = true;					// so far, anyway

    if (mFormatList!=NULL && mFormatList->selectedItems().count()==0) ok = false;
    if (mFilenameEdit!=NULL && mFilenameEdit->text().isEmpty()) ok = false;
    enableButtonOk(ok);
}


void FormatDialog::buildFormatList(bool recOnly)
{
    if (mFormatList==NULL) return;			// not showing this

    kDebug() << "recOnly" << recOnly << "for type" << mImageType;

    mFormatList->clear();
    for (KMimeType::List::const_iterator it = mMimeTypes.constBegin();
         it!=mMimeTypes.constEnd(); ++it)
    {
        KMimeType::Ptr mime = (*it);
	if (recOnly)					// only want recommended
	{
	    bool formatOk = false;
	    for (formatInfo *ip = &formats[0]; ip->mime!=NULL; ++ip)
	    {						// search for this format
		if (!mime->is(ip->mime)) continue;

                if (ip->recForTypes & mImageType)	// recommended for this type?
		{
                    formatOk = true;			// this format to be shown
                    break;				// no more to do
		}
	    }

	    if (!formatOk) continue;			// this format not to be shown
	}
							// add format to list
	QListWidgetItem *item = new QListWidgetItem(KIcon(mime->iconName()),
                                                    mime->comment(), mFormatList);
        // Not sure whether a KMimeType::Ptr can safely be stored in a
        // QVariant, so storing the MIME type name instead.
        item->setData(Qt::UserRole, mime->name());
	mFormatList->addItem(item);
    }

    formatSelected(NULL);				// selection has been cleared
}


void FormatDialog::slotOk()
{
    if (mRecOnlyCheck!=NULL)				// have UI for this
    {
        KConfigGroup grp = KGlobal::config()->group(OP_SAVER_GROUP);
        grp.writeEntry(OP_SAVER_REC_FMT, mRecOnlyCheck->isChecked());
    }							// save state of this option

    accept();
}


void FormatDialog::slotUser1()
{
    mWantAssistant = true;
    accept();
}


void FormatDialog::forgetRemembered()
{
    KConfigGroup grp = KGlobal::config()->group(OP_SAVER_GROUP);
							// reset all options to default
    grp.deleteEntry(OP_SAVER_REC_FMT);
    grp.deleteEntry(OP_SAVER_ASK_FORMAT);
    grp.deleteEntry(OP_SAVER_ASK_FILENAME);
    grp.deleteEntry(OP_FORMAT_HICOLOR);
    grp.deleteEntry(OP_FORMAT_COLOR);
    grp.deleteEntry(OP_FORMAT_GRAY);
    grp.deleteEntry(OP_FORMAT_BW);
    grp.sync();
}
