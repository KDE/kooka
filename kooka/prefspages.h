/* This file is part of the KDE Project				-*- mode:c++ -*-

   Copyright (C) 2000 Klaas Freitag <freitag@suse.de>
   Copyright (C) 2010 Jonathan Marten <jjm@keelhaul.me.uk>  

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   As a special exception, permission is given to link this program
   with any version of the KADMOS ocr/icr engine of reRecognition GmbH,
   Kreuzlingen and distribute the resulting executable without
   including the source code for KADMOS in the source distribution.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.

*/

#ifndef PREFSPAGES_H
#define PREFSPAGES_H

#include <qwidget.h>

#include <kconfiggroup.h>

#include "ocrengine.h"

class QCheckBox;
class QPushButton;
class QLabel;
class QVBoxLayout;

class KUrlRequester;
class KComboBox;
class KPageDialog;


class KookaPrefsPage : public QWidget
{
    Q_OBJECT

public:
    KookaPrefsPage(KPageDialog *parent, const char *configGroup = NULL);
    virtual ~KookaPrefsPage();

    virtual void saveSettings() = 0;
    virtual void defaultSettings() = 0;

protected:
    KConfigGroup mConfig;
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
    QCheckBox *mAskSaveFormat;
    QCheckBox *mAskFileName;
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
    bool checkOcrBinary(const QString &cmd, const QString &bin, bool showMsg);

    KUrlRequester *mOcrBinaryReq;
    KComboBox *mEngineCombo;
    QLabel *mDescLabel;

    OcrEngine::EngineType mSelectedEngine;
};


#endif							// PREFSPAGES_H
