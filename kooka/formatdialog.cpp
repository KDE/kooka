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

#include <qlayout.h>
#include <qlabel.h>
#include <qcombobox.h>

#include <kdialog.h>
#include <kseparator.h>
#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>
#ifdef USE_KIMAGEIO
#include <kimageio.h>
#endif

#include "imgsaver.h"

#include "formatdialog.h"
#include "formatdialog.moc"


struct formatInfo
{
    const char *format;
    QString helpString;
    int recForTypes;
};

static struct formatInfo formats[] =
{
    { "BMP", I18N_NOOP(
"<b>Bitmap Picture</b> is a widely used format for images under MS Windows. \
It is suitable for color, grayscale and line art images. \
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
"<b>Portable Pixmap</b>, as used by Netpbm, is an uncompressed format for full colour \
images. Only 24 bit per pixel RGB is supported."),
      ImgSaver::ImgColor|ImgSaver::ImgHicolor },

      { "PCX", I18N_NOOP(
"This is a lossless compressed format which is often supported by PC imaging \
applications, although it is rather old and unsophisticated.  It is suitable for \
color and grayscale images. \
<p>This format is not recommended, use an open format instead."),
        ImgSaver::ImgNone },

    { "XBM", I18N_NOOP(
"<b>X Bitmap</b> is often used by the X Window System to store cursor and icon bitmaps. \
<p>Unless required for this purpose, use a general purpose format instead."),
      ImgSaver::ImgNone },

    { "XPM", I18N_NOOP(
"<b>X Pixmap</b> is often used by the X Window System for color icons and other images. \
<p>Unless required for this purpose, use a general purpose format instead."),
      ImgSaver::ImgNone },

    { "PNG", I18N_NOOP(
"<b>Portable Network Graphics</b> is a lossless compressed format designed to be \
portable and extensible. It is suitable for any type of color or grayscale images, \
indexed or true color. \
<p>PNG is an open format which is widely supported."),
      ImgSaver::ImgBW|ImgSaver::ImgColor|ImgSaver::ImgGray|ImgSaver::ImgHicolor },

    { "JPEG", I18N_NOOP(
"<b>JPEG</b> is a compressed format suitable for true color or grayscale images. \
It is a lossy format, so it is not recommended for archiving or for repeated loading \
and saving. \
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
used extensively by animation and video applications. \
<p>This format is not recommended, use an open format instead."),
      ImgSaver::ImgNone },

    { "GIF", I18N_NOOP(					// writing may not be supported
"<b>Graphics Interchange Format</b> is a popular but patent-encumbered format often \
used for web graphics.  It uses lossless compression with up to 256 colors and optional \
transparency.\
<p>For legal reasons this format is not recommended, use an open format instead."),
      ImgSaver::ImgNone },

    { "TIFF", I18N_NOOP(				// writing may not be supported
"<b>Tagged Image File Format</b> is a versatile and extensible file format often \
supported by imaging and publishing applications. It supports indexed and true colour \
images with alpha transparency. Because there are many variations, there may sometimes \
be compatibility problems.\
<p>Unless required for use with other applications, use an open format instead."),
      ImgSaver::ImgBW|ImgSaver::ImgColor|ImgSaver::ImgGray|ImgSaver::ImgHicolor },

    { "XV", "", ImgSaver::ImgNone },

    { "RGB", "", ImgSaver::ImgNone },

    { NULL, "", ImgSaver::ImgNone }
};


FormatDialog::FormatDialog( QWidget *parent, ImgSaver::ImageType type)
	: KDialogBase( parent, NULL, true,
                 /* Tabbed,*/ i18n( "Save Assistant" ),
		 KDialogBase::Ok|KDialogBase::Cancel)
{
    QWidget *page = new QWidget(this);
    setMainWidget(page);

    QGridLayout *gl = new QGridLayout(page,10,3,0,KDialog::spacingHint());

    // some nice words
    QLabel *l0 = new QLabel(i18n("<qt>Select a format to save the scanned image.<br>This is a <b>%1</b>.")
			    .arg(ImgSaver::picTypeAsString(type)),page);
    gl->addMultiCellWidget(l0,0,0,0,2);

    KSeparator *sep = new KSeparator(KSeparator::HLine,page);
    gl->addMultiCellWidget(sep,1,1,0,2);

    // Insert scrolled list for formats
    QLabel *l1 = new QLabel(i18n("Image file format:"),page);
    gl->addWidget(l1,2,0,Qt::AlignLeft);

    lb_format = new QListBox(page);
#ifdef USE_KIMAGEIO
    formatList = KImageIO::types();
#else
    formatList = QImage::outputFormatList();
#endif
    kdDebug(28000) << k_funcinfo << "have " << formatList.count() << " image types" << endl;
    imgType = type;
							// list is built later
    connect(lb_format,SIGNAL(highlighted(const QString &)),SLOT(showHelp(const QString &)));
    l1->setBuddy(lb_format);
    gl->addWidget(lb_format,3,0);

    // Insert label for help text
    l_help = new QLabel(page);
    l_help->setFrameStyle( QFrame::Panel|QFrame::Sunken );
    l_help->setAlignment(Qt::AlignLeft|Qt::AlignTop);
    l_help->setMinimumSize(230,200);
    l_help->setMargin(4);
    gl->addMultiCellWidget(l_help,2,5,2,2);

    // Insert selection box for subformat
    l2 = new QLabel(i18n("Image sub-format:"),page);
    gl->addWidget(l2,4,0,Qt::AlignLeft);

    cb_subf = new QComboBox(page);
    gl->addWidget(cb_subf,5,0);
    l2->setBuddy(cb_subf);

    sep = new KSeparator(KSeparator::HLine,page);
    gl->addMultiCellWidget(sep,6,6,0,2);

    // Checkbox to store setting
    cbRecOnly = new QCheckBox(i18n("Only show recommended formats for this image type"),page);
    gl->addMultiCellWidget(cbRecOnly,7,7,0,2,Qt::AlignLeft);

    KConfig *conf = KGlobal::config();
    conf->setGroup(OP_FILE_GROUP);
    cbRecOnly->setChecked(conf->readBoolEntry(OP_ONLY_REC_FMT,true));
    connect(cbRecOnly,SIGNAL(toggled(bool)),SLOT(buildFormatList(bool)));

    buildFormatList(cbRecOnly->isChecked());		// now have this setting

    cbDontAsk  = new QCheckBox(i18n("Always use this save format for this image type"),page);
    gl->addMultiCellWidget(cbDontAsk,8,8,0,2,Qt::AlignLeft);

    gl->addMultiCellWidget(new QLabel("",page),9,9,0,2);
							// need that for bottom margin
    gl->setRowSpacing(9,KDialog::marginHint());
    gl->setColSpacing(1,KDialog::marginHint());
    gl->setRowStretch(3,1);
    gl->setColStretch(2,1);
}


void FormatDialog::showHelp(const QString &item)
{
    if (item.isNull())					// nothing is selected
    {
	l_help->setText(i18n("No format selected."));
	enableButtonOK(false);
	return;
    }

    QString helptxt;
    for (formatInfo *ip = &formats[0]; ip->format!=NULL; ++ip)
    {
	if (ip->format==item)
	{
	    helptxt = ip->helpString;
	    break;
	}
    }

    if (!helptxt.isEmpty())
    {
	l_help->setText(helptxt);			// Set the hint
	check_subformat(helptxt);			// and check subformats
    }
    else l_help->setText(i18n("No information is available for this format."));
    enableButtonOK(true);
}


void FormatDialog::check_subformat( const QString & format )
{
   // not yet implemented
   //kdDebug(28000) << "This is format in check_subformat: " << format << endl;
   cb_subf->setEnabled(false);
   // l2 = Label "select subformat" ->bad name :-|
   l2->setEnabled(false);
}


void FormatDialog::setSelectedFormat(const QString &fo)
{
   QListBoxItem *item = lb_format->findItem(fo);
   if (item!=NULL) lb_format->setSelected(lb_format->index(item),true);
}


QString FormatDialog::getFormat( ) const
{
   int item = lb_format->currentItem();

   if( item > -1 )
   {
      const QString f = lb_format->text( item );
      return( f );
   }
   return( "BMP" );
}


QCString FormatDialog::getSubFormat( ) const
{
   // Not yet...
   return( "" );
}


void FormatDialog::buildFormatList(bool recOnly)
{
    kdDebug(28000) << k_funcinfo << "only=" << recOnly << " type=" << imgType << endl;

    lb_format->clear();
    for (QStringList::const_iterator it = formatList.constBegin();
         it!=formatList.constEnd(); ++it)
    {
	if (recOnly)					// only want recommended
	{
	    bool formatOk = false;
	    for (formatInfo *ip = &formats[0]; ip->format!=NULL; ++ip)
	    {						// search for this format
		if (ip->format!=(*it)) continue;

                if (ip->recForTypes & imgType)		// recommended for this type?
		{
                    formatOk = true;			// this format to be shown
                    break;				// no more to do
		}
	    }

	    if (!formatOk) continue;			// this format not to be shown
	}

	lb_format->insertItem((*it).upper());		// add format to list
    }

    showHelp(QString::null);				// selection has been cleared
}


void FormatDialog::slotOk()
{
    KConfig *conf = KGlobal::config();
    conf->setGroup(OP_FILE_GROUP);			// save state of this option
    conf->writeEntry(OP_ONLY_REC_FMT,cbRecOnly->isChecked());

    KDialogBase::slotOk();
}


void FormatDialog::forgetRemembered()
{
    KConfig *conf = KGlobal::config();
    conf->setGroup(OP_FILE_GROUP);
							// reset all options to default
    conf->deleteEntry(OP_ONLY_REC_FMT);
    conf->deleteEntry(OP_FILE_ASK_FORMAT);
    conf->deleteEntry(OP_ASK_FILENAME);
    conf->deleteEntry(OP_FORMAT_HICOLOR);
    conf->deleteEntry(OP_FORMAT_COLOR);
    conf->deleteEntry(OP_FORMAT_GRAY);
    conf->deleteEntry(OP_FORMAT_BW);
}
