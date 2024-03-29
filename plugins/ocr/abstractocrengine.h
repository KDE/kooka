/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2000-2018 Klaas Freitag <freitag@suse.de>		*
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

#ifndef ABSTRACTOCRENGINE_H
#define ABSTRACTOCRENGINE_H

#include <qobject.h>
#include <qtextformat.h>
#include <qprocess.h>

#include "scanimage.h"
#include "abstractplugin.h"

/**
  *@author Klaas Freitag
  */

class QProcess;
class KConfigSkeletonItem;
class ImageFormat;
class ImageCanvas;
class AbstractOcrDialogue;

#ifndef PLUGIN_EXPORT
#define PLUGIN_EXPORT
#endif

// Using a QTextDocument for the OCR results, with the source image rectangle
// and other OCR information held as text properties.

class OcrWordData : public QTextCharFormat
{
public:
    enum DataType {
        Rectangle = QTextFormat::UserProperty,		// QRect
        Alternatives,					// QStringList
        KNode						// int - not sure what this ever did
    };

    OcrWordData() : QTextCharFormat()			{}
};


class PLUGIN_EXPORT AbstractOcrEngine : public AbstractPlugin
{
    Q_OBJECT

public:
    virtual ~AbstractOcrEngine();

    bool openOcrDialogue(QWidget *pnt = nullptr);

    /**
     * Sets an image canvas that displays the result image of the OCR.
     * If this is set to @c nullptr (or never set) no result image is displayed.
     * The OCR fabric passes a new image to the canvas which is a copy of
     * the image to OCR.
     */
    void setImageCanvas(ImageCanvas *canvas);
    void setTextDocument(QTextDocument *doc);

    /**
     * Specify the image to be OCRed.
     *
     * @param img The image
     **/
    void setImage(ScanImage::Ptr img);

    /**
     * Check whether the image to be OCRed is black/white (just a bitmap).
     *
     * @return @c true if the image is pure black/white
     **/
    bool isBW();

    QString findExecutable(QString (*settingFunc)(), KConfigSkeletonItem *settingItem);

    void setErrorText(const QString &msg)			{ m_errorText.append(msg); }

    /**
     * Check whether the engine has advanced settings:  for example, the
     * pathname of an executable which performs the OCR.  The actual dialogue
     * will be requested by @c openAdvancedSettings().
     *
     * @return @c true if the engine has advanced settings
     **/
    virtual bool hasAdvancedSettings() const			{ return (false); }

    /**
     * Open a dialogue for advanced engine settings.  This will only be
     * called if the engine has indicated that it has advanced settings,
     * by returning @c true from @c hasAdvancedSettings().
     **/
    virtual void openAdvancedSettings()				{}

protected:
    explicit AbstractOcrEngine(QObject *pnt, const char *name);

    virtual AbstractOcrDialogue *createOcrDialogue(AbstractOcrEngine *plugin, QWidget *pnt) = 0;

    virtual QStringList tempFiles(bool retain) = 0;

    /**
     * Save an image to a temporary file.
     *
     * @param img The image to save
     * @param format The image format to save in
     * @param colors The colour depth (bits per pixel) required.  If specified,
     * this must be either 1, 8, 24 or 32.  The default is for no colour
     * conversion.
     *
     * @return The file name as saved, or @c QString() if there was
     *         an error.
     **/
    QString tempSaveImage(ScanImage::Ptr img, const ImageFormat &format, int colors = -1);

    /**
     * Get a name to use for a temporary file.
     *
     * @param suffix File name suffix, no leading '.' is required
     * @return The temporary file name, or @c QString() if the file could not be created
     *
     * @note The temporary file is created and is left in place under the returned name,
     * but is not opened.  Its name should be saved and eventually returned in the
     * @c tempFiles() list so that it will be removed.
     **/
    QString tempFileName(const QString &suffix, const QString &baseName = "ocrtemp");

    QTextDocument *startResultDocument();
    void finishResultDocument();

    void startLine();
    void addWord(const QString &word, const OcrWordData &data);
    void finishLine();

    bool verboseDebug() const;

    virtual bool createOcrProcess(AbstractOcrDialogue *dia, ScanImage::Ptr img) = 0;

    QProcess *initOcrProcess();
    QProcess *ocrProcess() const				{ return (m_ocrProcess); }
    bool runOcrProcess();
    virtual bool finishedOcrProcess(QProcess *proc)		{ Q_UNUSED(proc); return (true); }

    void setResultImage(const QString &file)			{ m_ocrResultFile = file; }

signals:
    void newOCRResultText();
    void openOcrPrefs();

    void setSpellCheckConfig(const QString &configFile);
    void startSpellCheck(bool interactive, bool background);

    /**
     * Indicates that the text editor holding the text that came through
     * newOCRResultText should be set to readonly or not. Can be connected
     * to QTextEdit::setReadOnly directly.
     */
    void readOnlyEditor(bool isReadOnly);

    /**
     * Progress of the OCR process. The first integer is the main progress,
     * the second the sub progress. If there is only one progress, it is the
     * first parameter while the second should be -1.
     * Both have a range from 0..100.
     *
     * Note that this signal may not be emitted if the engine does not support
     * progress.
     */
    void ocrProgress(int progress, int subprogress);

    /**
     * Select a word in the editor corresponding to the position within
     * the result image.
     */
    void selectWord(const QPoint &p);

public slots:
    void slotHighlightWord(const QRect &r);
    void slotScrollToWord(const QRect &r);

private slots:
    void slotStartOCR();
    void slotStopOCR();
    void slotClose();

    void slotProcessExited(int exitCode, QProcess::ExitStatus exitStatus);

    /**
     * Handle mouse double clicks on the image viewer showing the
     * OCR result image.
     */
    void slotImagePosition(const QPoint &p);

private:
    void stopOcrProcess(bool tellUser);
    void removeTempFiles();
    void finishedOcr(bool success);

    QString collectErrorMessages(const QString &starter, const QString &ender);

private:
    QWidget *m_parent;

    QProcess *m_ocrProcess;
    bool m_ocrRunning;
    AbstractOcrDialogue *m_ocrDialog;
    QStringList m_errorText;
    QString m_ocrStderrLog;

    QString m_ocrResultFile;
    ImageCanvas *m_imgCanvas;

    ScanImage::Ptr m_introducedImage;
    bool m_resolvedBW;
    bool m_isBW;

    QTextDocument *m_document;
    QTextCursor *m_cursor;
    int m_currHighlight;
    bool m_trackingActive;

    int m_wordCount;
};

#endif							// ABSTRACTOCRENGINE_H
