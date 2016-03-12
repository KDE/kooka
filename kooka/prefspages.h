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

#ifndef PREFSPAGES_H
#define PREFSPAGES_H

#include <qwidget.h>

#include "ocrengine.h"

class QCheckBox;
class QPushButton;
class QRadioButton;
class QLabel;
class QVBoxLayout;

class KUrlRequester;
class KComboBox;
class KPageDialog;

class KookaPrefsPage : public QWidget
{
    Q_OBJECT

public:
    KookaPrefsPage(KPageDialog *parent);
    virtual ~KookaPrefsPage();

    virtual void saveSettings() = 0;
    virtual void defaultSettings() = 0;

protected:
    QVBoxLayout *mLayout;
};

class KookaGeneralPage : public KookaPrefsPage
{
    Q_OBJECT

public:
    KookaGeneralPage(KPageDialog *parent);

    void saveSettings();
    void defaultSettings();

private slots:
    void slotEnableWarnings();

private:
    QPushButton *mEnableMessagesButton;
};

class KookaStartupPage : public KookaPrefsPage
{
    Q_OBJECT

public:
    KookaStartupPage(KPageDialog *parent);

    void saveSettings();
    void defaultSettings();

private:
    void applySettings();

    QCheckBox *mNetQueryCheck;
    QCheckBox *mSelectScannerCheck;
    QCheckBox *mRestoreImageCheck;
};

class KookaSavingPage : public KookaPrefsPage
{
    Q_OBJECT

public:
    KookaSavingPage(KPageDialog *parent);

    void saveSettings();
    void defaultSettings();

private:
    void applySettings();

    QCheckBox *mAskSaveFormat;
    QCheckBox *mAskFileName;
    QRadioButton *mAskBeforeScan;
    QRadioButton *mAskAfterScan;
};

class KookaThumbnailPage : public KookaPrefsPage
{
    Q_OBJECT

public:
    KookaThumbnailPage(KPageDialog *parent);

    void saveSettings();
    void defaultSettings();

private slots:
    void slotCustomThumbBgndToggled(bool state);

private:
    void applySettings();

    QCheckBox *mAllowRenameCheck;
    KComboBox *mGalleryLayoutCombo;
    KUrlRequester *mTileSelector;
    KComboBox *mThumbSizeCombo;
    QCheckBox *mCustomBackgroundCheck;
};

class KookaOcrPage : public KookaPrefsPage
{
    Q_OBJECT

public:
    KookaOcrPage(KPageDialog *parent);

    void saveSettings();
    void defaultSettings();

private slots:
    void slotEngineSelected(int i);

private:
    void applySettings();
    bool checkOcrBinary(const QString &cmd, const QString &bin, bool showMsg);

    KUrlRequester *mOcrBinaryReq;
    KComboBox *mEngineCombo;
    QLabel *mDescLabel;

    OcrEngine::EngineType mSelectedEngine;
};

#endif                          // PREFSPAGES_H
