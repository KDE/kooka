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

//#include "imgsaver.h"
#include "kookaimage.h"


struct formatInfo
{
    const char *format;
    const char *helpString;
    int recForTypes;
};


// TODO: there are more types now, this list needs extending.  Also the list
// (even after cleaning up) includes some synonyms, e.g. TIF for TIFF.  Need
// to get information for TIFF when TIF is requested, preferably without having
// to duplicate the help strings.
static struct formatInfo formats[] =
{
    { "BMP", I18N_NOOP(
"<b>Bitmap Picture</b> is a widely used format for images under MS Windows. \
It is suitable for color, grayscale and line art images.\
<p>This format is widely supported but is not recommended, use an open format \
instead."),
      ImgSaver::ImgNone },

    { "PBM", I18N_NOOP(
"<b>Portable Bitmap</b>, as used by Netpbm, is an uncompressed format for line art \
(bitmap) images. Only 1 bit per pixel depth is supported."),
      ImgSaver::ImgBW },

    { "PGM", I18N_NOOP(
"<b>Portable Greymap</b>, as used by Netpbm, is an uncompressed format for grayscale \
images. Only 8 bit per pixel depth is supported."),
      ImgSaver::ImgGray },

    { "PPM", I18N_NOOP(
"<b>Portable Pixmap</b>, as used by Netpbm, is an uncompressed format for full color \
images. Only 24 bit per pixel RGB is supported."),
      ImgSaver::ImgColor|ImgSaver::ImgHicolor },

      { "PCX", I18N_NOOP(
"This is a lossless compressed format which is often supported by PC imaging \
applications, although it is rather old and unsophisticated.  It is suitable for \
color and grayscale images.\
<p>This format is not recommended, use an open format instead."),
        ImgSaver::ImgNone },

    { "XBM", I18N_NOOP(
"<b>X Bitmap</b> is often used by the X Window System to store cursor and icon bitmaps.\
<p>Unless required for this purpose, use a general purpose format instead."),
      ImgSaver::ImgNone },

    { "XPM", I18N_NOOP(
"<b>X Pixmap</b> is often used by the X Window System for color icons and other images.\
<p>Unless required for this purpose, use a general purpose format instead."),
      ImgSaver::ImgNone },

    { "PNG", I18N_NOOP(
"<b>Portable Network Graphics</b> is a lossless compressed format designed to be \
portable and extensible. It is suitable for any type of color or grayscale images, \
indexed or true color.\
<p>PNG is an open format which is widely supported."),
      ImgSaver::ImgBW|ImgSaver::ImgColor|ImgSaver::ImgGray|ImgSaver::ImgHicolor },

    { "JPEG", I18N_NOOP(
"<b>JPEG</b> is a compressed format suitable for true color or grayscale images. \
It is a lossy format, so it is not recommended for archiving or for repeated loading \
and saving.\
<p>This is an open format which is widely supported."),
      ImgSaver::ImgHicolor|ImgSaver::ImgGray },

    { "JP2", I18N_NOOP(
"<b>JPEG 2000</b> was intended as an update to the JPEG format, with the option of \
lossless compression, but so far is not widely supported. It is suitable for true \
color or grayscale images."),
      ImgSaver::ImgNone },

    { "EPS", I18N_NOOP(
"<b>Encapsulated PostScript</b> is derived from the PostScript&trade; \
page description language.  Use this format for importing into other \
applications, or to use with (e.g.) TeX."),
      ImgSaver::ImgNone },

    { "TGA", I18N_NOOP(
"<b>Truevision Targa</b> can store full colour images with an alpha channel, and is \
used extensively by animation and video applications.\
<p>This format is not recommended, use an open format instead."),
      ImgSaver::ImgNone },

    { "GIF", I18N_NOOP(					// writing may not be supported
"<b>Graphics Interchange Format</b> is a popular but patent-encumbered format often \
used for web graphics.  It uses lossless compression with up to 256 colors and \
optional transparency.\
<p>For legal reasons this format is not recommended, use an open format instead."),
      ImgSaver::ImgNone },

    { "TIFF", I18N_NOOP(				// writing may not be supported
"<b>Tagged Image File Format</b> is a versatile and extensible file format widely \
supported by imaging and publishing applications. It supports indexed and true color \
images with alpha transparency.\
<p>Because there are many variations, there may sometimes be compatibility problems.\
Unless required for use with other applications, use an open format instead."),
      ImgSaver::ImgBW|ImgSaver::ImgColor|ImgSaver::ImgGray|ImgSaver::ImgHicolor },

    { "XV", NULL, ImgSaver::ImgNone },

    { "RGB", NULL, ImgSaver::ImgNone },

    { NULL, NULL, ImgSaver::ImgNone }
};



FormatDialog::FormatDialog(QWidget *parent, ImgSaver::ImageType type,
                           bool askForFormat, const QString &format,
                           bool askForFilename, const QString &filename)
    : KDialog(parent)
{
    setObjectName("FormatDialog");

    kDebug();

    setModal(true);
    setButtons(KDialog::Ok|KDialog::Cancel|KDialog::User1);
    setCaption(askForFormat ? i18n("Save Assistant") : i18n("Save Scan"));

    QWidget *page = new QWidget(this);
    setMainWidget(page);

    l_help = NULL;
    cb_subf = NULL;
    lb_format = NULL;
    l_subf = NULL;
    cbDontAsk = NULL;
    cbRecOnly = NULL;
    l_ext = NULL;
    le_filename = NULL;

    m_format = format;					// save these to return, if
    m_filename = filename;				// they are not requested
    if (m_format.isEmpty()) askForFormat = true;	// must ask if none

    m_wantAssistant = false;

    QGridLayout *gl = new QGridLayout(page);
    gl->setSpacing(KDialog::spacingHint());
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
        l1 = new QLabel(i18n("Image file &format:"),page);
        gl->addWidget(l1,row,0,Qt::AlignLeft);

        lb_format = new QListWidget(page);

        // Clean up the list that we get from KImageIO (=Qt).  The raw list
        // contains mixed case, spaces and duplicates.
        QStringList supportedTypes = KImageIO::types(KImageIO::Writing);
        formatList.clear();
        for (QStringList::const_iterator it = supportedTypes.constBegin();
             it!=supportedTypes.constEnd(); ++it)
        {
            QString type = (*it).toUpper().trimmed();
            if (!formatList.contains(type)) formatList.append(type);
        }
        kDebug() << "have" << formatList.count() << "image types"
                 << "from" << supportedTypes.count() << "supported";
							// list box is filled later
        imgType = type;
        connect(lb_format, SIGNAL(currentItemChanged(QListWidgetItem *,QListWidgetItem *)), SLOT(formatSelected(QListWidgetItem *)));
        l1->setBuddy(lb_format);
        gl->addWidget(lb_format,row+1,0);
        gl->setRowStretch(row+1,1);

        // Insert label for help text
        l_help = new QLabel(page);
        l_help->setFrameStyle(QFrame::Panel|QFrame::Sunken);
        l_help->setAlignment(Qt::AlignLeft|Qt::AlignTop);
        l_help->setMinimumSize(230,200);
        l_help->setMargin(4);
        l_help->setWordWrap(true);
        gl->addWidget(l_help, row, 1, 4, 2);

        // Insert selection box for subformat
        l_subf = new QLabel(i18n("Image sub-format:"),page);
        gl->addWidget(l_subf,row+2,0,Qt::AlignLeft);

        cb_subf = new QComboBox(page);
        gl->addWidget(cb_subf,row+3,0);
        l_subf->setBuddy(cb_subf);
        row += 4;

        sep = new KSeparator(Qt::Horizontal, page);
        gl->addWidget(sep, row, 0, 1, 3);
        ++row;

        // Checkbox to store setting
        cbRecOnly = new QCheckBox(i18n("Only show recommended formats for this image type"),page);
        gl->addWidget(cbRecOnly, row, 0, 1, 3, Qt::AlignLeft);
        ++row;

        KConfigGroup grp = KGlobal::config()->group(OP_SAVER_GROUP);
        cbRecOnly->setChecked(grp.readEntry(OP_SAVER_REC_FMT, true));
        connect(cbRecOnly, SIGNAL(toggled(bool)), SLOT(buildFormatList(bool)));

        cbDontAsk  = new QCheckBox(i18n("Always use this save format for this image type"),page);
        gl->addWidget(cbDontAsk, row, 0, 1, 3, Qt::AlignLeft);
        ++row;

        buildFormatList(cbRecOnly->isChecked());	// now have this setting

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
        l1 = new QLabel(i18n("&Image file name:"), page);
        gl->addWidget(l1, row, 0, 1, 3);
        ++row;

        le_filename = new QLineEdit(filename, page);
        connect(le_filename, SIGNAL(textChanged(const QString &)), SLOT(checkValid()));
        l1->setBuddy(le_filename);
        gl->addWidget(le_filename, row, 0, 1, 2);

        l_ext = new QLabel("",page);
        gl->addWidget(l_ext, row, 2, Qt::AlignLeft);
        ++row;

        if (!askForFormat) setButtonText(KDialog::User1, i18n("Select Format..."));
    }

    if (lb_format!=NULL)				// have the format selector
    {							// preselect the remembered format
        QList<QListWidgetItem *> items = lb_format->findItems(format, Qt::MatchFixedString);
        if (!items.isEmpty())
        {
            QListWidgetItem *format_item = items.first();
            if (format_item!=NULL) lb_format->setCurrentItem(format_item);
        }
    }
    else						// no format selector, but
    {							// asking for a file name
        showExtension(format);				// show extension it will have
    }

    connect(this, SIGNAL(okClicked()), SLOT(slotOk()));
    connect(this, SIGNAL(user1Clicked()), SLOT(slotUser1()));
}


void FormatDialog::show()
{
    KDialog::show();

    if (le_filename!=NULL)				// asking for a file name
    {
        le_filename->setFocus();			// set focus to that
        le_filename->selectAll();			// highight for editing
    }
}


void FormatDialog::showExtension(const QString &format)
{
    if (l_ext==NULL) return;				// no UI for this
    l_ext->setText("."+KookaImage::extensionForFormat(format));
}							// show extension it will have


void FormatDialog::formatSelected(QListWidgetItem *item)
{
    if (l_help==NULL) return;				// not showing this

    if (item==NULL)					// nothing is selected
    {
	l_help->setText(i18n("No format selected."));
	enableButtonOk(false);

        lb_format->clearSelection();
        if (l_ext!=NULL) l_ext->setText(".???");
	return;
    }

    lb_format->setCurrentItem(item);			// focus highlight -> select

    QString itxt = item->text();
    const char *helptxt = NULL;
    for (formatInfo *ip = &formats[0]; ip->format!=NULL; ++ip)
    {							// locate help text for format
	if (ip->format==itxt)
	{
	    helptxt = ip->helpString;
	    break;
	}
    }

    if (helptxt!=NULL)					// found some help text
    {
	l_help->setText(i18n(helptxt));			// set the hint
	check_subformat(itxt);				// and check subformats
    }
    else l_help->setText(i18n("No information is available for this format."));

    if (cbDontAsk!=NULL) cbDontAsk->setChecked(ImgSaver::isRememberedFormat(imgType,itxt));

    showExtension(itxt);
    checkValid();
}


void FormatDialog::check_subformat(const QString &format)
{
    if (cb_subf==NULL) return;				// not showing this
    // not yet implemented
    //kdDebug(28000) << "This is format in check_subformat: " << format << endl;
    cb_subf->setEnabled(false);
    // l_subf = Label "select subformat" ->bad name :-|
    l_subf->setEnabled(false);
}


void FormatDialog::setSelectedFormat(const QString &fo)
{
    if (lb_format==NULL) return;			// not showing this

    QListWidgetItem *item = lb_format->findItems(fo, Qt::MatchFixedString).first();
    if (item!=NULL) lb_format->setCurrentItem(item);
}


QString FormatDialog::getFormat() const
{
    if (lb_format==NULL) return (m_format);		// no UI for this

    const QListWidgetItem *item = lb_format->currentItem();
    if (item!=NULL) return(item->text());

    return ("BMP");					// a sort of default
}


QString FormatDialog::getFilename() const
{
    if (le_filename==NULL) return (m_filename);		// no UI for this
    return (le_filename->text());
}


QByteArray FormatDialog::getSubFormat( ) const
{
   // Not yet...
   return( "" );
}


void FormatDialog::checkValid()
{
    bool ok = true;					// so far, anyway

    if (lb_format!=NULL && lb_format->selectedItems().count()==0) ok = false;
    if (le_filename!=NULL && le_filename->text().isEmpty()) ok = false;
    enableButtonOk(ok);
}


void FormatDialog::buildFormatList(bool recOnly)
{
    if (lb_format==NULL) return;			// not showing this

    kDebug() << "only" << recOnly << "type" << imgType;

    lb_format->clear();
    for (QStringList::const_iterator it = formatList.constBegin();
         it!=formatList.constEnd(); ++it)
    {
        QString fmt = (*it);
	if (recOnly)					// only want recommended
	{
	    bool formatOk = false;
	    for (formatInfo *ip = &formats[0]; ip->format!=NULL; ++ip)
	    {						// search for this format
		if (ip->format!=fmt) continue;

                if (ip->recForTypes & imgType)		// recommended for this type?
		{
                    formatOk = true;			// this format to be shown
                    break;				// no more to do
		}
	    }

	    if (!formatOk) continue;			// this format not to be shown
	}
							// add format to list
	lb_format->addItem(new QListWidgetItem(KIcon(KookaImage::iconForFormat(fmt)),
                                               fmt.toUpper(), lb_format));
    }

    formatSelected(NULL);				// selection has been cleared
}


void FormatDialog::slotOk()
{
    if (cbRecOnly!=NULL)				// have UI for this
    {
        KConfigGroup grp = KGlobal::config()->group(OP_SAVER_GROUP);
        grp.writeEntry(OP_SAVER_REC_FMT, cbRecOnly->isChecked());
    }							// save state of this option

    accept();
}


void FormatDialog::slotUser1()
{
    m_wantAssistant = true;
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
