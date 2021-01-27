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

#ifndef FORMATDIALOG_H
#define FORMATDIALOG_H

#include "kookacore_export.h"

#include "dialogbase.h"

#include "imgsaver.h"
#include "imageformat.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;


/**
 *  Class FormatDialog:
 *  Asks the user for the image-Format and gives help for
 *  selecting it.
 **/

class KOOKACORE_EXPORT FormatDialog : public DialogBase
{
    Q_OBJECT

public:
    explicit FormatDialog(QWidget *parent, ScanImage::ImageType type,
                          bool askForFormat, const ImageFormat &format,
                          bool askForFilename, const QString &filename);

    ImageFormat getFormat() const;
    QByteArray getSubFormat() const;
    QString getFilename() const;
    bool alwaysUseFormat() const;
    bool useAssistant() const    			{ return mWantAssistant; }

    static void forgetRemembered();

    static bool isCompatible(const QMimeType &mime, ScanImage::ImageType type, bool recOnly = true);

protected:
    void showEvent(QShowEvent *ev) override;
protected slots:
    void slotOk();
    void slotUser1();

private slots:
    void checkValid();
    void buildFormatList(bool recOnly);
    void formatSelected(QListWidgetItem *item);

private:
    void setSelectedFormat(const ImageFormat &format);
    void check_subformat(const ImageFormat &format);
    void showExtension(const ImageFormat &format);

    ScanImage::ImageType mImageType;

    QComboBox *mSubformatCombo;
    QListWidget *mFormatList;
    QCheckBox *mDontAskCheck;
    QCheckBox *mRecOnlyCheck;
    QLabel *mHelpLabel;
    QLabel *mSubformatLabel;

    QLineEdit *mFilenameEdit;
    QLabel *mExtensionLabel;

    ImageFormat mFormat;
    QString mFilename;
    bool mWantAssistant;
};

#endif							// FORMATDIALOG_H
