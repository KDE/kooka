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

#ifndef __FORMATDIALOG_H__
#define __FORMATDIALOG_H__

#include <qcheckbox.h>

#include <kdialogbase.h>

#include "imgsaver.h"

class QComboBox;
class QListBox;
class QLabel;
class QCheckBox;

/**
 *  Class FormatDialog:
 *  Asks the user for the image-Format and gives help for
 *  selecting it.
 **/

class FormatDialog : public KDialogBase
{
Q_OBJECT

public:
   FormatDialog( QWidget *parent, picType type);

   QString      getFormat( ) const;
   QCString      getSubFormat( ) const;

   bool         askForFormat( ) const
      { return( ! cbDontAsk->isChecked()); }

   static void forgetRemembered();

public slots:
    void        setSelectedFormat( QString );


protected slots:
   void 	showHelp( const QString& item );
   void buildFormatList(bool recOnly);
   void slotOk();

private:
   void		check_subformat( const QString & format );

   QStringList formatList;
   picType imgType;

   QComboBox   	*cb_subf;
   QListBox    	*lb_format;
   QLabel      	*l_help;
   QLabel	*l2;
   QCheckBox    *cbRemember;
   QCheckBox    *cbDontAsk;
   QCheckBox    *cbRecOnly;
};

#endif
