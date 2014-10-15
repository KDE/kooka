/***************************************************** -*- mode:c++; -*- ***
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

#ifndef FORMATDIALOG_H
#define FORMATDIALOG_H

#include <qcheckbox.h>

#include <kdialog.h>
#include <kmimetype.h>

#include "imgsaver.h"
#include "imageformat.h"

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

class FormatDialog : public KDialog
{
    Q_OBJECT

public:
    FormatDialog(QWidget *parent, ImageMetaInfo::ImageType type,
                 bool askForFormat, const ImageFormat &format,
                 bool askForFilename, const QString &filename);

    ImageFormat getFormat() const;
    QByteArray getSubFormat() const;
    QString getFilename() const;
    bool alwaysUseFormat() const
    {
        return (mDontAskCheck != NULL ? mDontAskCheck->isChecked() : false);
    }
    bool useAssistant() const
    {
        return (mWantAssistant);
    }

    static void forgetRemembered();

protected:
    virtual void showEvent(QShowEvent *ev);

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

    KMimeType::List mMimeTypes;

    ImageMetaInfo::ImageType mImageType;

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

#endif                          // FORMATDIALOG_H
