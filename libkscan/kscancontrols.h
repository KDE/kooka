/* This file is part of the KDE Project
   Copyright (C) 2000 Klaas Freitag <freitag@suse.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KSCANCONTROLS_H
#define KSCANCONTROLS_H

#include "libkscanexport.h"

#include <qframe.h>
#include <qstringlist.h>

#include <khbox.h>

/**
  *@author Klaas Freitag
  */

class QPushButton;
class QSpinBox;
class QLabel;
class QComboBox;
class QSlider;
class QLineEdit;

class KUrlRequester;


/**
 * a kind of extended slider which has a spinbox beside the slider offering
 * the possibility to enter an exact numeric value to the slider. If
 * desired, the slider has a neutral button by the side. A descriptional
 * text is handled automatically.
 *
 * @author Klaas Freitag <freitag@suse.de>
 */
class KSCAN_EXPORT KScanSlider : public QFrame
{
   Q_OBJECT
   Q_PROPERTY( int slider_val READ value WRITE slotSetSlider )

public:
   /**
    * Create the slider.
    *
    * @param parent parent widget
    * @param text is the text describing the the slider value. If the text
    *        contains a '&', a buddy for the slider will be created.
    * @param min minimum slider value
    * @param max maximum slider value
    * @param haveStdButt make a 'set to standard'-button visible. The button
    *        appears on the left of the slider.
    * @param stdValue the value to which the standard button resets the slider.
    */
   KScanSlider( QWidget *parent, const QString& text,
		double min, double max, bool haveStdButt=false,
		int stdValue=0);
   /**
    * Destructor
    */
   ~KScanSlider();

   /**
    * @return the current slider value
    */
   int value( ) const;

public slots:
  /**
   * sets the slider value
   */
   void		slotSetSlider( int );

   /**
    * enables the complete slider.
    */
   void		setEnabled( bool b );

protected slots:
    /**
     * reverts the slider back to the standard value given in the constructor
     */
     void         slotRevertValue();

   signals:
    /**
     * emitted if the slider value changes
     */
     void	  valueChanged( int );

private slots:
   void		slotSliderChange( int );

private:
   QSlider	*slider;
   QLabel	*l1, *numdisp;
   QSpinBox     *m_spin;
   int          m_stdValue;
   QPushButton  *m_stdButt;
};

/**
 * a entry field with a prefix text for description.
 */
class KSCAN_EXPORT KScanEntry : public KHBox
{
   Q_OBJECT
   Q_PROPERTY( QString text READ text WRITE slotSetEntry )

public:
   /**
    * create a new entry field prepended by text.
    * @param parent the parent widget
    * @text the prefix text
    */
   KScanEntry( QWidget *parent, const QString& text );
   // ~KScanEntry();

   /**
    * @return the current entry field contents.
    */
   QString text( ) const;

public slots:
   /**
    * set the current text
    * @param t the new string
    */
   void		slotSetEntry( const QString& t );
   /**
    * enable or disable the text entry.
    * @param b set enabled if true, else disabled.
    */
   void		setEnabled( bool b );

protected slots:
   void         slotReturnPressed( void );

signals:
   void		valueChanged( const QByteArray& );
   void         returnPressed( const QByteArray& );

private slots:
   void		slotEntryChange( const QString& );

private:
   QLineEdit 	*entry;
};


/**
 * a combobox filled with a decriptional text.
 */
class KSCAN_EXPORT KScanCombo : public KHBox
{
   Q_OBJECT
   Q_PROPERTY( QString cbEntry READ currentText WRITE slotSetEntry )

public:
   /**
    * create a combobox with prepended text.
    *
    * @param parent parent widget
    * @param text the text the combobox is prepended by
    * @param list a stringlist with values the list should contain.
    */
   KScanCombo( QWidget *parent, const QString& text, const QList<QByteArray> &list );
   KScanCombo( QWidget *parent, const QString& text, const QStringList &list );
   // ~KScanCombo();

   /**
    * @return the current selected text
    */
   QString      currentText( ) const;

   /**
    * @return the text a position i
    */
   QString      text( int i ) const;

   /**
    * @return the amount of list entries.
    */
   int  	count( ) const;

public slots:
   /**
    * set the current entry to the given string if it is member of the list.
    * if not, the entry is not changed.
    */
   void		slotSetEntry( const QString &);

   /**
    * enable or disable the combobox.
    * @param b enables the combobox if true.
    */
   void		setEnabled( bool b);

   /**
    * set an icon for a string in the combobox
    * @param pix the pixmap to set.
    * @param str the string for which the pixmap should be set.
    */
   void         slotSetIcon( const QPixmap& pix, const QString& str);

   /**
    * set the current item of the combobox.
    */
   void         setCurrentItem( int i );

private slots:
   void         slotFireActivated( int);
   void		slotComboChange( const QString & );

signals:
   void		valueChanged( const QByteArray& );
   void         activated(int);

private:
    void createCombo( const QString& text );
   QComboBox	*combo;
   QList<QByteArray>	combolist;
};







/**
 * a entry field with a prefix text for description.
 */
class KSCAN_EXPORT KScanFileRequester : public KHBox
{
   Q_OBJECT
   Q_PROPERTY( QString text READ text WRITE slotSetEntry )

public:
   /**
    * create a new entry field prepended by text.
    * @param parent the parent widget
    * @text the prefix text
    */
   KScanFileRequester( QWidget *parent, const QString& text );
   // ~KScanFileRequester();

   /**
    * @return the current entry field contents.
    */
   QString text( ) const;

public slots:
   /**
    * set the current text
    * @param t the new string
    */
   void		slotSetEntry( const QString& t );
   /**
    * enable or disable the text entry.
    * @param b set enabled if true, else disabled.
    */
   void		setEnabled( bool b );

protected slots:
   void         slotReturnPressed( void );

signals:
   void		valueChanged( const QByteArray& );
   void         returnPressed( const QByteArray& );

private slots:
   void		slotEntryChange( const QString& );

private:
   KUrlRequester 	*entry;
};

#endif							// KSCANCONTROLS_H
