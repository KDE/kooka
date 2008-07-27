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
#include <qlineedit.h>

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
    const char *helpString;
    int recForTypes;
};

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
"<b>Portable Pixmap</b>, as used by Netpbm, is an uncompressed format for full colour \
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
"<b>Tagged Image File Format</b> is a versatile and extensible file format often \
supported by imaging and publishing applications. It supports indexed and true color \
images with alpha transparency.\
<p>Because there are many variations, there may sometimes be compatibility problems.\
Unless required for use with other applications, use an open format instead."),
      ImgSaver::ImgBW|ImgSaver::ImgColor|ImgSaver::ImgGray|ImgSaver::ImgHicolor },

    { "XV", NULL, ImgSaver::ImgNone },

    { "RGB", NULL, ImgSaver::ImgNone },

    { NULL, NULL, ImgSaver::ImgNone }
};



FormatDialog::FormatDialog(QWidget *parent,ImgSaver::ImageType type,
                           bool askForFormat,const QString &format,
                           bool askForFilename,const QString &filename)
        : KDialogBase(parent,NULL,true,QString::null,
                      KDialogBase::Ok|KDialogBase::Cancel|KDialogBase::User1)
{
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

    setCaption(askForFormat ? i18n("Save Assistant") : i18n("Save Scan"));

    QGridLayout *gl = new QGridLayout(page,1,3,0,KDialog::spacingHint());
    int row = 0;

    QLabel *l1;
    KSeparator *sep;

    if (askForFormat)					// format selector section
    {
        l1 = new QLabel(i18n("<qt>Select a format to save the scanned image.<br>This is a <b>%1</b>.")
                                .arg(ImgSaver::picTypeAsString(type)),page);
        gl->addMultiCellWidget(l1,row,row,0,2);
        ++row;

        sep = new KSeparator(KSeparator::HLine,page);
        gl->addMultiCellWidget(sep,row,row,0,2);
        ++row;

        // Insert scrolled list for formats
        l1 = new QLabel(i18n("Image file &format:"),page);
        gl->addWidget(l1,row,0,Qt::AlignLeft);

        lb_format = new QListBox(page);
#ifdef USE_KIMAGEIO
        formatList = KImageIO::types();
#else
        formatList = QImage::outputFormatList();
#endif
        kdDebug(28000) << k_funcinfo << "have " << formatList.count() << " image types" << endl;
        imgType = type;
							// list is built later
        connect(lb_format,SIGNAL(highlighted(QListBoxItem *)),SLOT(formatSelected(QListBoxItem *)));
        l1->setBuddy(lb_format);
        gl->addWidget(lb_format,row+1,0);
        gl->setRowStretch(row+1,1);

        // Insert label for help text
        l_help = new QLabel(page);
        l_help->setFrameStyle( QFrame::Panel|QFrame::Sunken );
        l_help->setAlignment(Qt::AlignLeft|Qt::AlignTop);
        l_help->setMinimumSize(230,200);
        l_help->setMargin(4);
        gl->addMultiCellWidget(l_help,row,row+3,1,2);

        // Insert selection box for subformat
        l_subf = new QLabel(i18n("Image sub-format:"),page);
        gl->addWidget(l_subf,row+2,0,Qt::AlignLeft);

        cb_subf = new QComboBox(page);
        gl->addWidget(cb_subf,row+3,0);
        l_subf->setBuddy(cb_subf);
        row += 4;

        sep = new KSeparator(KSeparator::HLine,page);
        gl->addMultiCellWidget(sep,row,row,0,2);
        ++row;

        // Checkbox to store setting
        cbRecOnly = new QCheckBox(i18n("Only show recommended formats for this image type"),page);
        gl->addMultiCellWidget(cbRecOnly,row,row,0,2,Qt::AlignLeft);
        ++row;

        KConfig *conf = KGlobal::config();
        conf->setGroup(OP_SAVER_GROUP);
        cbRecOnly->setChecked(conf->readBoolEntry(OP_SAVER_REC_FMT,true));
        connect(cbRecOnly,SIGNAL(toggled(bool)),SLOT(buildFormatList(bool)));

        cbDontAsk  = new QCheckBox(i18n("Always use this save format for this image type"),page);
        gl->addMultiCellWidget(cbDontAsk,row,row,0,2,Qt::AlignLeft);
        ++row;

        buildFormatList(cbRecOnly->isChecked());	// now have this setting

        showButton(KDialogBase::User1,false);		// don't want this button
    }

    gl->setColStretch(1,1);
    gl->setColSpacing(1,KDialog::marginHint());

    if (askForFormat && askForFilename)
    {
        sep = new KSeparator(KSeparator::HLine,page);
        gl->addMultiCellWidget(sep,row,row,0,2);
        ++row;
    }

    if (askForFilename)					// file name section
    {
        l1 = new QLabel(i18n("&Image file name:"),page);
        gl->addMultiCellWidget(l1,row,row,0,2);
        ++row;

        le_filename = new QLineEdit(filename,page);
        connect(le_filename,SIGNAL(textChanged(const QString &)),SLOT(checkValid()));
        l1->setBuddy(le_filename);
        gl->addMultiCellWidget(le_filename,row,row,0,1);

        l_ext = new QLabel("",page);
        gl->addWidget(l_ext,row,2,Qt::AlignLeft);
        ++row;

        if (!askForFormat) setButtonText(KDialogBase::User1,i18n("Select Format..."));
    }

    if (lb_format!=NULL)				// have the format selector
    {							// preselect the remembered format
        QListBoxItem *format_item = lb_format->findItem(format);
        if (format_item!=NULL) lb_format->setSelected(format_item,true);
    }
    else						// no format selector, but
    {							// asking for a file name
        showExtension(format);				// show extension it will have
    }
}


void FormatDialog::show()
{
    KDialogBase::show();

    if (le_filename!=NULL)				// asking for a file name
    {
        le_filename->setFocus();			// set focus to that
        le_filename->selectAll();			// highight for editing
    }
}


void FormatDialog::showExtension(const QString &format)
{
    if (l_ext==NULL) return;				// no UI for this

    QString suf = KImageIO::suffix(format);
    if (suf.isEmpty()) suf = format.lower();
    l_ext->setText(QString(".%1").arg(suf));		// show extension it will have
}


void FormatDialog::formatSelected(QListBoxItem *item)
{
    if (l_help==NULL) return;				// not showing this

    if (item==NULL)					// nothing is selected
    {
	l_help->setText(i18n("No format selected."));
	enableButtonOK(false);

        lb_format->clearSelection();
        if (l_ext!=NULL) l_ext->setText(".???");
	return;
    }

    lb_format->setSelected(item,true);			// focus highlight -> select

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


void FormatDialog::check_subformat( const QString & format )
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

    QListBoxItem *item = lb_format->findItem(fo);
    if (item!=NULL) lb_format->setSelected(lb_format->index(item),true);
}


QString FormatDialog::getFormat() const
{
    if (lb_format==NULL) return (m_format);		// no UI for this

    int item = lb_format->currentItem();
    if (item>-1) return(lb_format->text(item));

    return ("BMP");					// a sort of default
}


QString FormatDialog::getFilename() const
{
    if (le_filename==NULL) return (m_filename);		// no UI for this
    return (le_filename->text());
}


QCString FormatDialog::getSubFormat( ) const
{
   // Not yet...
   return( "" );
}


void FormatDialog::checkValid()
{
    bool ok = true;					// so far, anyway

    if (lb_format!=NULL && lb_format->selectedItem()==NULL) ok = false;
    if (le_filename!=NULL && le_filename->text().isEmpty()) ok = false;
    enableButtonOK(ok);
}


void FormatDialog::buildFormatList(bool recOnly)
{
    if (lb_format==NULL) return;			// not showing this

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

    formatSelected(NULL);				// selection has been cleared
}


void FormatDialog::slotOk()
{
    if (cbRecOnly!=NULL)				// have UI for this
    {
        KConfig *conf = KGlobal::config();
        conf->setGroup(OP_SAVER_GROUP);			// save state of this option
        conf->writeEntry(OP_SAVER_REC_FMT,cbRecOnly->isChecked());
    }

    KDialogBase::slotOk();
}


void FormatDialog::slotUser1()
{
    m_wantAssistant = true;
    KDialogBase::slotOk();
}



void FormatDialog::forgetRemembered()
{
    KConfig *conf = KGlobal::config();
    conf->setGroup(OP_SAVER_GROUP);
							// reset all options to default
    conf->deleteEntry(OP_SAVER_REC_FMT);
    conf->deleteEntry(OP_SAVER_ASK_FORMAT);
    conf->deleteEntry(OP_SAVER_ASK_FILENAME);
    conf->deleteEntry(OP_FORMAT_HICOLOR);
    conf->deleteEntry(OP_FORMAT_COLOR);
    conf->deleteEntry(OP_FORMAT_GRAY);
    conf->deleteEntry(OP_FORMAT_BW);
}
