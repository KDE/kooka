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

#ifndef __FORMATDIALOG_H__
#define __FORMATDIALOG_H__

#include <qcheckbox.h>

#include <kdialogbase.h>

#include "imgsaver.h"

class QComboBox;
class QListBox;
class QLabel;
class QLineEdit;
class QListBoxItem;

/**
 *  Class FormatDialog:
 *  Asks the user for the image-Format and gives help for
 *  selecting it.
 **/

class FormatDialog : public KDialogBase
{
    Q_OBJECT

public:
    FormatDialog(QWidget *parent,ImgSaver::ImageType type,
                 bool askForFormat,const QString &format,
                 bool askForFilename,const QString &filename);

    QString getFormat() const;
    QCString getSubFormat() const;
    QString getFilename() const;
    bool alwaysUseFormat() const { return (cbDontAsk!=NULL ? cbDontAsk->isChecked() : false); }
    bool useAssistant() const { return (m_wantAssistant); }

    void setSelectedFormat(const QString &format);

    static void forgetRemembered();

protected:
    virtual void show();

protected slots:
    void slotOk();
    void slotUser1();

private slots:
    void checkValid();
    void buildFormatList(bool recOnly);
    void formatSelected(QListBoxItem *item);

private:
    void check_subformat(const QString &format);
    void showExtension(const QString &format);

    QStringList formatList;
    ImgSaver::ImageType imgType;

    QComboBox *cb_subf;
    QListBox *lb_format;
    QCheckBox *cbDontAsk;
    QCheckBox *cbRecOnly;
    QLabel *l_help;
    QLabel *l_subf;

    QLineEdit *le_filename;
    QLabel *l_ext;

    QString m_format;
    QString m_filename;
    bool m_wantAssistant;
};

#endif							// FORMATDIALOG_H
