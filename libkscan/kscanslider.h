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

#ifndef KSCANSLIDER_H
#define KSCANSLIDER_H

#include <qframe.h>
#include <qstrlist.h>
#include <qhbox.h>
#include <qcombobox.h>
#include <qslider.h>
#include <qlineedit.h>
/**
  *@author Klaas Freitag
  */

class QPushButton;
class QSpinBox;
class QLabel;

/**
 * a kind of extended slider which has a spinbox beside the slider offering
 * the possibility to enter an exact numeric value to the slider. If
 * desired, the slider has a neutral button by the side. A descriptional
 * text is handled automatically.
 *
 * @author Klaas Freitag <freitag@suse.de>
 */
class KScanSlider : public QFrame
{
   Q_OBJECT
   Q_PROPERTY( int slider_val READ value WRITE slSetSlider )

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
   int value( ) const
      { return( slider->value()); }

public slots:
  /**
   * sets the slider value
   */
   void		slSetSlider( int );

   /**
    * enables the complete slider.
    */
   void		setEnabled( bool b );

protected slots:
    /**
     * reverts the slider back to the standard value given in the constructor
     */
     void         slRevertValue();

   signals:
    /**
     * emitted if the slider value changes
     */
     void	  valueChanged( int );

private slots:
   void		slSliderChange( int );

private:
   QSlider	*slider;
   QLabel	*l1, *numdisp;
   QSpinBox     *m_spin;
   int          m_stdValue;
   QPushButton  *m_stdButt;
   class KScanSliderPrivate;
   KScanSliderPrivate *d;

};

/**
 * a entry field with a prefix text for description.
 */
class KScanEntry : public QFrame
{
   Q_OBJECT
   Q_PROPERTY( QString text READ text WRITE slSetEntry )

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
   void		slSetEntry( const QString& t );
   /**
    * enable or disable the text entry.
    * @param b set enabled if true, else disabled.
    */
   void		setEnabled( bool b ){ if( entry) entry->setEnabled( b ); }

protected slots:
   void         slReturnPressed( void );

signals:
   void		valueChanged( const QCString& );
   void         returnPressed( const QCString& );

private slots:
   void		slEntryChange( const QString& );

private:
   QLineEdit 	*entry;

   class KScanEntryPrivate;
   KScanEntryPrivate *d;

};


/**
 * a combobox filled with a decriptional text.
 */
class KScanCombo : public QHBox
{
   Q_OBJECT
   Q_PROPERTY( QString cbEntry READ currentText WRITE slSetEntry )

public:
   /**
    * create a combobox with prepended text.
    *
    * @param parent parent widget
    * @param text the text the combobox is prepended by
    * @param list a stringlist with values the list should contain.
    */
   KScanCombo( QWidget *parent, const QString& text, const QStrList& list );
   KScanCombo( QWidget *parent, const QString& text, const QStringList& list );
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
   void		slSetEntry( const QString &);

   /**
    * enable or disable the combobox.
    * @param b enables the combobox if true.
    */
   void		setEnabled( bool b){ if(combo) combo->setEnabled( b ); };

   /**
    * set an icon for a string in the combobox
    * @param pix the pixmap to set.
    * @param str the string for which the pixmap should be set.
    */
   void         slSetIcon( const QPixmap& pix, const QString& str);

   /**
    * set the current item of the combobox.
    */
   void         setCurrentItem( int i );

private slots:
   void         slFireActivated( int);
   void		slComboChange( const QString & );

signals:
   void		valueChanged( const QCString& );
   void         activated(int);

private:
    void createCombo( const QString& text );
   QComboBox	*combo;
   QStrList	combolist;

   class KScanComboPrivate;
   KScanComboPrivate *d;
};

#endif
