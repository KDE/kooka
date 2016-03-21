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

#include "ocrbasedialog.h"

#include <qlabel.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qprogressbar.h>
#include <qapplication.h>
#include <qradiobutton.h>
#include <qpushbutton.h>
#include <qdebug.h>
#include <qicon.h>
#include <qstandardpaths.h>

#include <klocalizedstring.h>
#include <kstandardguiitem.h>
#include <kseparator.h>

#include <kio/job.h>
#include <kio/previewjob.h>

#include <sonnet/configdialog.h>

#include "ocrengine.h"
#include "kookaimage.h"
#include "kookasettings.h"

#include "imagecanvas.h"
#include "dialogbase.h"


OcrBaseDialog::OcrBaseDialog(QWidget *parent)
    : KPageDialog(parent),
      m_setupPage(NULL),
      m_sourcePage(NULL),
      m_enginePage(NULL),
      m_spellPage(NULL),
      m_debugPage(NULL),
      m_previewPix(NULL),
      m_previewLabel(NULL),
      m_wantDebugCfg(true),
      m_cbRetainFiles(NULL),
      m_cbVerboseDebug(NULL),
      m_lVersion(NULL),
      m_progress(NULL)
{
    //qDebug();

    setObjectName("OcrBaseDialog");

    setModal(true);
    // original KDE4 buttons: User1=Start User2=Stop Close
    // new Qt5 buttons:       Yes=Start   No=Stop    Close
    QDialogButtonBox *bb = buttonBox();
    setStandardButtons(QDialogButtonBox::Yes|QDialogButtonBox::No|QDialogButtonBox::Close);
    bb->button(QDialogButtonBox::Yes)->setDefault(true);
    setWindowTitle(i18n("Optical Character Recognition"));

    KGuiItem::assign(bb->button(QDialogButtonBox::Yes), KGuiItem(i18n("Start OCR"), "system-run", i18n("Start the Optical Character Recognition process")));
    KGuiItem::assign(bb->button(QDialogButtonBox::No), KGuiItem(i18n("Stop OCR"), "process-stop", i18n("Stop the Optical Character Recognition process")));

    // Signals which tell our caller what the user is doing
    connect(bb->button(QDialogButtonBox::Yes), &QAbstractButton::clicked, this, &OcrBaseDialog::slotStartOCR);
    connect(bb->button(QDialogButtonBox::No), &QAbstractButton::clicked, this, &OcrBaseDialog::slotStopOCR);
    connect(bb->button(QDialogButtonBox::Close), &QAbstractButton::clicked, this, &OcrBaseDialog::slotCloseOCR);

    m_previewSize.setWidth(380);			// minimum preview size
    m_previewSize.setHeight(250);

    bb->button(QDialogButtonBox::Yes)->setEnabled(true);  	// Start OCR
    bb->button(QDialogButtonBox::No)->setEnabled(false);	// Stop OCR
    bb->button(QDialogButtonBox::Close)->setEnabled(true);	// Close
}

OcrBaseDialog::~OcrBaseDialog()
{
    //qDebug();
}

OcrEngine::EngineError OcrBaseDialog::setupGui()
{
    setupSetupPage();
    setupSpellPage();
    setupSourcePage();
    setupEnginePage();
    // TODO: preferences option for whether debug is shown
    if (m_wantDebugCfg) {
        setupDebugPage();
    }

    return (OcrEngine::ENG_OK);
}

void OcrBaseDialog::setupSetupPage()
{
    QWidget *w = new QWidget(this);
    QGridLayout *gl = new QGridLayout(w);

    m_progress = new QProgressBar(this);
    m_progress->setVisible(false);

    m_setupPage = addPage(w, i18n("Setup"));
    m_setupPage->setHeader(i18n("Optical Character Recognition using %1", ocrEngineName()));
    m_setupPage->setIcon(QIcon::fromTheme("ocr"));
}

QWidget *OcrBaseDialog::addExtraPageWidget(KPageWidgetItem *page, QWidget *wid, bool stretchBefore)
{
    QGridLayout *gl = static_cast<QGridLayout *>(page->widget()->layout());
    int nextrow = gl->rowCount();
    // rowCount() seems to return 1 even if the layout is empty...
    if (gl->itemAtPosition(0, 0) == NULL) {
        nextrow = 0;
    }

    if (stretchBefore) {                // stretch before new row
        gl->setRowStretch(nextrow, 1);
        ++nextrow;
    } else if (nextrow > 0) {           // something there already,
        // add separator line
        gl->addWidget(new KSeparator(Qt::Horizontal, this), nextrow, 0, 1, 2);
        ++nextrow;
    }

    if (wid == NULL) {
        wid = new QWidget(this);
    }
    gl->addWidget(wid, nextrow, 0, 1, 2);

    return (wid);
}

QWidget *OcrBaseDialog::addExtraSetupWidget(QWidget *wid, bool stretchBefore)
{
    return (addExtraPageWidget(m_setupPage, wid, stretchBefore));
}

void OcrBaseDialog::ocrShowInfo(const QString &binary, const QString &version)
{
    QWidget *w = addExtraEngineWidget();        // engine path/version/icon
    QGridLayout *gl = new QGridLayout(w);

    if (!binary.isNull()) {
        QLabel *l = new QLabel(i18n("Executable:"), w);
        gl->addWidget(l, 0, 0, Qt::AlignLeft | Qt::AlignTop);

        l = new QLabel(binary, w);
        gl->addWidget(l, 0, 1, Qt::AlignLeft | Qt::AlignTop);

        l = new QLabel(i18n("Version:"), w);
        gl->addWidget(l, 1, 0, Qt::AlignLeft | Qt::AlignTop);

        m_lVersion = new QLabel((!version.isEmpty() ? version : i18n("unknown")), w);
        gl->addWidget(m_lVersion, 1, 1, Qt::AlignLeft | Qt::AlignTop);
    }

    // Find the logo and display if available
    QString logoFile = QStandardPaths::locate(QStandardPaths::AppDataLocation, "pics/"+ocrEngineLogo());
    if (!logoFile.isEmpty())
    {
        QPixmap pix(logoFile);
        QLabel *l = new QLabel(w);
        l->setPixmap(pix);
        gl->addWidget(l, 0, 3, 2, 1, Qt::AlignRight);
    }

    gl->setColumnStretch(2, 1);

}

void OcrBaseDialog::ocrShowVersion(const QString &version)
{
    if (m_lVersion != NULL) {
        m_lVersion->setText(version);
    }
}

void OcrBaseDialog::setupSourcePage()
{
    QWidget *w = new QWidget(this);
    QGridLayout *gl = new QGridLayout(w);

    // These labels are filled with the preview pixmap and image
    // information in introduceImage()
    m_previewPix = new QLabel(i18n("No preview available"), w);
    m_previewPix->setPixmap(QPixmap());
    m_previewPix->setMinimumSize(m_previewSize.width() + 2*DialogBase::horizontalSpacing(),
                                 m_previewSize.height() + 2*DialogBase::verticalSpacing());
    m_previewPix->setAlignment(Qt::AlignCenter);
    m_previewPix->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    gl->addWidget(m_previewPix, 0, 0);
    gl->setRowStretch(0, 1);

    m_previewLabel = new QLabel(i18n("No information available"), w);
    gl->addWidget(m_previewLabel, 1, 0, Qt::AlignHCenter);

    m_sourcePage = addPage(w, i18n("Source"));
    m_sourcePage->setHeader(i18n("Source Image Information"));
    m_sourcePage->setIcon(QIcon::fromTheme("dialog-information"));
}

void OcrBaseDialog::setupEnginePage()
{
    QWidget *w = new QWidget(this);         // engine title/logo/description
    QGridLayout *gl = new QGridLayout(w);

    // row 0: engine description
    QLabel *l = new QLabel(ocrEngineDesc(), w);
    l->setWordWrap(true);
    l->setOpenExternalLinks(true);
    gl->addWidget(l, 0, 0, 1, 2, Qt::AlignTop);

    gl->setRowStretch(0, 1);
    gl->setColumnStretch(0, 1);

    m_enginePage = addPage(w, i18n("OCR Engine"));
    m_enginePage->setHeader(i18n("OCR Engine Information"));
    m_enginePage->setIcon(QIcon::fromTheme("application-x-executable"));
}

QWidget *OcrBaseDialog::addExtraEngineWidget(QWidget *wid, bool stretchBefore)
{
    return (addExtraPageWidget(m_enginePage, wid, stretchBefore));
}

void OcrBaseDialog::setupSpellPage()
{
    QWidget *w = new QWidget(this);
    QGridLayout *gl = new QGridLayout(w);

    // row 0: background checking group box
    m_gbBackgroundCheck = new QGroupBox(i18n("Highlight misspelled words"), w);
    m_gbBackgroundCheck->setCheckable(true);

    QGridLayout *gl1 = new QGridLayout(m_gbBackgroundCheck);
    m_gbBackgroundCheck->setLayout(gl1);

    m_rbGlobalSpellSettings = new QRadioButton(i18n("Use the system spell configuration"), w);
    gl1->addWidget(m_rbGlobalSpellSettings, 0, 0);
    m_rbCustomSpellSettings = new QRadioButton(i18n("Use custom spell configuration"), w);
    gl1->addWidget(m_rbCustomSpellSettings, 1, 0);
    m_pbCustomSpellDialog = new QPushButton(i18n("Custom Spell Configuration..."), w);
    gl1->addWidget(m_pbCustomSpellDialog, 2, 0, Qt::AlignRight);
    connect(m_rbCustomSpellSettings, SIGNAL(toggled(bool)),
            m_pbCustomSpellDialog, SLOT(setEnabled(bool)));
    connect(m_pbCustomSpellDialog, SIGNAL(clicked()),
            SLOT(slotCustomSpellDialog()));
    gl->addWidget(m_gbBackgroundCheck, 0, 0);

    // row 1: space
    gl->setRowMinimumHeight(1, 2*DialogBase::verticalSpacing());

    // row 2: interactive checking group box
    m_gbInteractiveCheck = new QGroupBox(i18n("Start interactive spell check"), w);
    m_gbInteractiveCheck->setCheckable(true);

    QGridLayout *gl2 = new QGridLayout(m_gbInteractiveCheck);
    m_gbInteractiveCheck->setLayout(gl2);

    QLabel *l = new QLabel(i18n("Custom spell settings above do not affect this spelling check, use the language setting in the dialog to change the dictionary language."), w);
    l->setWordWrap(true);
    gl2->addWidget(l, 0, 0);

    gl->addWidget(m_gbInteractiveCheck, 2, 0);

    // row 3: stretch
    gl->setRowStretch(3, 1);

    // Apply settings
    m_gbBackgroundCheck->setChecked(KookaSettings::ocrSpellBackgroundCheck());
    m_gbInteractiveCheck->setChecked(KookaSettings::ocrSpellInteractiveCheck());

#ifndef KF5
    const bool customSettings = KookaSettings::ocrSpellCustomSettings();
#else
    const bool customSettings = false;
    m_rbCustomSpellSettings->setEnabled(false);
#endif
    m_rbGlobalSpellSettings->setChecked(!customSettings);
    m_rbCustomSpellSettings->setChecked(customSettings);
    m_pbCustomSpellDialog->setEnabled(customSettings);

    m_spellPage = addPage(w, i18n("Spell Check"));
    m_spellPage->setHeader(i18n("OCR Result Spell Checking"));
    m_spellPage->setIcon(QIcon::fromTheme("tools-check-spelling"));
}

void OcrBaseDialog::setupDebugPage()
{
    QWidget *w = new QWidget(this);
    QGridLayout *gl = new QGridLayout(w);

    m_cbRetainFiles = new QCheckBox(i18n("Retain temporary files"), w);
    gl->addWidget(m_cbRetainFiles, 0, 0, Qt::AlignTop);

    m_cbVerboseDebug = new QCheckBox(i18n("Verbose message output"), w);
    gl->addWidget(m_cbVerboseDebug, 1, 0, Qt::AlignTop);

    gl->setRowStretch(2, 1);

    m_debugPage = addPage(w, i18n("Debugging"));
    m_debugPage->setHeader(i18n("OCR Debugging"));
    m_debugPage->setIcon(QIcon::fromTheme("tools-report-bug"));
}

void OcrBaseDialog::stopAnimation()
{
    if (m_progress != NULL) {
        m_progress->setVisible(false);
    }
}

void OcrBaseDialog::startAnimation()
{
    if (!m_progress->isVisible()) {         // progress bar not added yet
        m_progress->setValue(0);
        addExtraSetupWidget(m_progress, true);
        m_progress->setVisible(true);
    }
}

// Not sure why this uses an asynchronous preview job for the image thumbnail
// (if it is possible, i.e. the image is file bound) as opposed to just scaling
// the image (which is always loaded at this point, i.e. it is already in memory).
// Possibly because scaling a potentially very large image could introduce a
// significant delay in opening the dialogue box, so making the GUI appear
// less responsive.  So we'll keep the preview job for now.
//
// We now bring you a mild rant...
//
// What on earth happened to KFileMetaInfo in KDE4?  This used to have a fairly
// reasonable API, returning a list of key-value pairs grouped into sensible
// categories with readable strings available for each.  Now the groups have
// gone (so for example methods such as preferredGroups(), albeit being marked as
// 'deprecated', return an empty list!) and the key of each entry is an ontology
// URL.  Not sure what to do with this URL (although I'm sure it must be of
// interest to something), and it doesn't even return the minimal useful
// information (e.g. the size/depth) for many image file types anyway.
//
// Could this be why the "Meta Info" tab of the file properties dialogue also
// seems to have disappeared?
//
// So forget about KFileMetaInfo here, just display a simple label with the
// image size and depth (which information we already have available).

void OcrBaseDialog::introduceImage(const KookaImage *img)
{
    if (img == NULL) {
        if (m_previewLabel != NULL) {
            m_previewLabel->setText(i18n("No image"));
        }
        return;
    }

    //qDebug() << "url" << img->url() << "filebound" << img->isFileBound();

    if (img->isFileBound()) {           // image backed by a file
        /* Start to create a preview job for the thumb */
        KFileItemList fileItems;
        fileItems.append(KFileItem(img->url()));

        KIO::PreviewJob *job = KIO::filePreview(fileItems, QSize(m_previewSize.width(), m_previewSize.height()));
        if (job!=NULL) {
            job->setIgnoreMaximumSize();
            connect(job, SIGNAL(gotPreview(KFileItem,QPixmap)),
                    SLOT(slotGotPreview(KFileItem,QPixmap)));
        }
    } else {                    // selection only in memory,
        // do the preview ourselves
        QImage qimg = img->scaled(m_previewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        slotGotPreview(KFileItem(), QPixmap::fromImage(qimg));
    }

    if (m_previewLabel != NULL) {
        KLocalizedString str = img->isFileBound() ? ki18n("Image: %1") : ki18n("Selection: %1");
        m_previewLabel->setText(str.subs(ImageCanvas::imageInfoString(img)).toString());
    }
}

bool OcrBaseDialog::keepTempFiles() const
{
    return (m_retainFiles);
}

bool OcrBaseDialog::verboseDebug() const
{
    return (m_verboseDebug);
}

void OcrBaseDialog::slotGotPreview(const KFileItem &item, const QPixmap &newPix)
{
    //qDebug() << "pixmap" << newPix.size();
    if (m_previewPix != NULL) {
        m_previewPix->setText(QString::null);
        m_previewPix->setPixmap(newPix);
    }
}

void OcrBaseDialog::slotWriteConfig()
{
    KookaSettings::setOcrSpellBackgroundCheck(m_gbBackgroundCheck->isChecked());
    KookaSettings::setOcrSpellInteractiveCheck(m_gbInteractiveCheck->isChecked());
    KookaSettings::setOcrSpellCustomSettings(m_rbCustomSpellSettings->isChecked());
    KookaSettings::self()->save();

    // deliberately not writing the OCR debug config
}

void OcrBaseDialog::slotStartOCR()
{
    setCurrentPage(m_setupPage);            // force back to first page
    enableGUI(true);                    // disable while running

    m_retainFiles = (m_cbRetainFiles != NULL && m_cbRetainFiles->isChecked());
    m_verboseDebug = (m_cbVerboseDebug != NULL && m_cbVerboseDebug->isChecked());

    slotWriteConfig();                  // save configuration
    emit signalOcrStart();              // start the OCR process
}

void OcrBaseDialog::slotStopOCR()
{
    emit signalOcrStop();               // stop the OCR process
    enableGUI(false);                   // enable everything again
}

void OcrBaseDialog::slotCloseOCR()
{
    emit signalOcrClose();              // indicate we're closed
}

void OcrBaseDialog::enableGUI(bool running)
{
    m_sourcePage->setEnabled(!running);
    m_enginePage->setEnabled(!running);
    if (m_spellPage != NULL) {
        m_spellPage->setEnabled(!running);
    }
    if (m_debugPage != NULL) {
        m_debugPage->setEnabled(!running);
    }
    enableFields(!running);				// engine's GUI widgets

    if (running) startAnimation();			// start our progress bar
    else stopAnimation();				// stop our progress bar

    QDialogButtonBox *bb = buttonBox();
    bb->button(QDialogButtonBox::Yes)->setEnabled(!running);  	// Start OCR
    bb->button(QDialogButtonBox::No)->setEnabled(running);	// Stop OCR
    bb->button(QDialogButtonBox::Close)->setEnabled(!running);	// Close

    QApplication::processEvents();			// ensure GUI up-to-date
}

bool OcrBaseDialog::wantInteractiveSpellCheck() const
{
    return (m_gbInteractiveCheck->isChecked());
}

bool OcrBaseDialog::wantBackgroundSpellCheck() const
{
    return (m_gbBackgroundCheck->isChecked());
}

QString OcrBaseDialog::customSpellConfigFile() const
{
    if (m_rbCustomSpellSettings->isChecked()) {		// our application config
        return (KSharedConfig::openConfig()->name());
    }
    return ("sonnetrc");				// Sonnet global settings
}

QProgressBar *OcrBaseDialog::progressBar() const
{
    return (m_progress);
}

void OcrBaseDialog::slotCustomSpellDialog()
{
#ifndef KF5
    // TODO: Sonnet in KF5 appears to no longer allow a custom configuration,
    // QSettings("KDE","Sonnet") is hardwired in Settings::restore() in
    // sonnet/src/core/settings.cpp
    // See also KookaView::slotSetOcrSpellConfig()
    // It may be possible, though, to configure only the language;
    // see http://api.kde.org/frameworks-api/frameworks5-apidocs/sonnet/html/classSonnet_1_1ConfigDialog.html
    Sonnet::ConfigDialog d(KSharedConfig::openConfig().data(), this);
    d.exec();						// save to our application config
#endif
}
