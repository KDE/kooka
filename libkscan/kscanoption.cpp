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

#include <qwidget.h>
#include <qobject.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qtooltip.h>
#include <qregexp.h>

#include <kdebug.h>
#include <klocale.h>

extern "C" {
#include <sane/sane.h>
}

#include "kgammatable.h"
#include "kscandevice.h"
#include "kscancontrols.h"

#include "kscanoption.h"
#include "kscanoption.moc"


// #define MIN_PREVIEW_DPI 20
// this is nowhere used!? Additionally, it's defined to 75 in kscandevice.cpp

/* switch to show some from time to time useful alloc-messages */
#undef MEM_DEBUG
#undef GETSET_DEBUG

#undef APPLY_IN_SITU


//  This defines the possible resolutions that will be shown by the combo.
//  Only resolutions from this list falling within the scanner's allowed range
//  will be included.
static const int resList[] = { 50, 75, 100, 150, 200, 300, 600, 900, 1200, 1800, 2400, 4800, 9600, 0 };



/** inline-access to the option descriptor, object to changes with global vars. **/
inline const SANE_Option_Descriptor *getOptionDesc( const QCString& name )
{
   int *idx = KScanDevice::option_dic->find(name);
   
   const SANE_Option_Descriptor *d = 0;
   // debug( "<< for option %s >>", name );
   if ( idx && *idx > 0 )
   {
      d = sane_get_option_descriptor( KScanDevice::scanner_handle, *idx );
      // debug( "retrieving Option %s", d->name );
   }
   else
   {
      kdDebug(29000) << "no option descriptor for <" << name << ">" << endl;
      // debug( "Name survived !" );
   }
   // debug( "<< leaving option %s >>", name );

   return( d );
}



/* ************************************************************************ */
/* KScan Option                                                             */
/* ************************************************************************ */

KScanOption::KScanOption(const QCString& new_name)
{
    if (!initOption(new_name))
    {
        kdDebug(29000) << k_funcinfo << "initOption for <" << new_name << "> failed!" << endl;
        return;
    }

    int *num = KScanDevice::option_dic->find(name);
    if (num==NULL || buffer.isNull()) return;

    SANE_Status sane_stat = sane_control_option(KScanDevice::scanner_handle, *num,
                                                SANE_ACTION_GET_VALUE,
                                                buffer.data(),NULL);
    if (sane_stat==SANE_STATUS_GOOD) buffer_untouched = false;
}


bool KScanOption::initOption(const QCString& new_name)
{
    desc = NULL;
    if (new_name.isEmpty()) return (false);

    name = new_name;
    desc = getOptionDesc(name);
    buffer.resize(0);
    buffer_untouched = true;
    internal_widget = NULL;

    if (desc==NULL) return (false);

    /* Gamma-Table - initial values */
    gamma = 0; /* marks as unvalid */
    brightness = 0;
    contrast = 0;
    gamma = 100;
	
    allocForDesc();

    KScanOption *gtOption = (*KScanDevice::gammaTables)[ new_name ];
    if( gtOption )
    {
        kdDebug(29000) << "Is older GammaTable!" << endl;
        KGammaTable gt;
        gtOption->get( &gt );
	
        gamma = gt.getGamma();
        contrast = gt.getContrast();
        brightness = gt.getBrightness();
    }

   return (true);
}


KScanOption::KScanOption(const KScanOption &so)
    : QObject()
{
    desc = so.desc;					// stored by sane-lib -> may be copied
    name = so.name;					// QCString explicit sharing -> shallow copy
    internal_widget = NULL;				// the widget is not copied!

    gamma = so.gamma;
    brightness = so.brightness;
    contrast = so.contrast;

    if (desc==NULL && name.isNull())
    {
        kdWarning(29000) << "Trying to copy a not healthy option (no name nor desc)" << endl;
        return;
    }

    buffer = so.buffer.copy();				// QByteArray -> deep copy
    buffer_untouched = so.buffer_untouched;
}


const QString KScanOption::configLine()
{
    QCString strval = this->get();
    kdDebug(29000) << "configLine returns <" << strval << ">" << endl;
    return (strval);
}


const KScanOption &KScanOption::operator=(const KScanOption &so)
{
    if (this==&so) return (*this);

    desc = so.desc;					// stored by sane-lib -> may be copied
    name = so.name;					// QCString explicit sharing -> shallow copy

    gamma = so.gamma;
    brightness = so.brightness;
    contrast = so.contrast;

    delete internal_widget;				// widget *is* copied here
    internal_widget = so.internal_widget;

    buffer.resize(0);					// throw away old buffer
    buffer = so.buffer.copy();				// QByteArray -> deep copy
    buffer_untouched = so.buffer_untouched;

    return (*this);
}


void KScanOption::slWidgetChange( const QCString& t )
{
    //kdDebug(29000) << k_funcinfo << "received WidgetChange for " << getName() << endl;
    set( t );
    emit( guiChange( this ) );
    // emit( optionChanged( this ));
}

void KScanOption::slWidgetChange( void )
{
    //kdDebug(29000) << k_funcinfo << "received WidgetChange for " << getName() << endl;
    /* If Type is bool, the widget is a checkbox. */
    if( type() == KScanOption::Bool )
    {
	bool b = ((QCheckBox*) internal_widget)->isChecked();
	set( b );
    }
    emit( guiChange( this ) );
    // emit( optionChanged( this ));

}

void KScanOption::slWidgetChange( int i )
{
    //kdDebug(29000) << k_funcinfo << "received WidgetChange for " << getName() << endl;
    set( i );
    emit( guiChange( this ) );
    // emit( optionChanged( this ));
}

// TODO: eliminate redundant 'so' parameter, see comment below!
/* this slot is called on a widget change, if a widget was created.
 * In normal case, it is internally connected, so the param so and
 * this are equal !
 */
void KScanOption::slRedrawWidget( KScanOption *so )
{
    int i = 0;
    QWidget *w = so->widget();
    QString string;
	
    if (!so->valid() || w==NULL || so->getBuffer()==NULL) return;

    switch (so->type())
    {
case KScanOption::Bool:					/* Widget Type is Toggle Button */
        if (so->get(&i)) static_cast<QCheckBox *>(w)->setChecked((bool) i);
        break;

case KScanOption::SingleValue: 	 			/* Widget Type is Entry Field */
        break;						/* not implemented yet */

case KScanOption::Range: 	 	  		/* Widget Type is Slider */
        if (so->get(&i)) static_cast<KScanSlider *>(w)->slSetSlider(i);
        break;

case KScanOption::Resolution: 	 	  		/* Widget Type is Resolution Combo */
        static_cast<KScanCombo *>(w)->slSetEntry(so->get());
        break;

case KScanOption::GammaTable: 	 	  		/* Widget Type is Gamma Table */
        break;

case KScanOption::StringList:				/* Widget Type is Selection Box */
        static_cast<KScanCombo *>(w)->slSetEntry(so->get());
        break;

case KScanOption::String:				/* Widget Type is String */
        static_cast<KScanEntry *>(w)->slSetEntry(so->get());
        break;

case KScanOption::File: 	  	 		/* Widget Type is File */
        static_cast<KScanFileRequester *>(w)->slSetEntry(so->get());
        break;

default:
        break;
    }
}

/* In this slot, the option queries the scanner for current values. */

void KScanOption::slReload()
{
    int *num = KScanDevice::option_dic->find(name);
    if (!valid() || num==NULL) return;
    desc = getOptionDesc(name);	
		
    if (widget()!=NULL)
    {
        if (!active())
        {
            kdDebug(29000) << desc->name << " is not active" << endl;
            widget()->setEnabled(false);
        }
        else if (!softwareSetable())
        {
            kdDebug(29000) << desc->name << " is not software setable" << endl;
            widget()->setEnabled(false);
        }
        else widget()->setEnabled(true);
    }
	
   if (buffer.isNull())					/* first get mem if needed */
   {
       kdDebug(29000) << k_funcinfo << "need to allocate now" << endl;
       allocForDesc();
   }

   if (!active()) return;

   if (((size_t) desc->size)>buffer.size())
   {
       kdDebug(29000) << "ERROR: Buffer too small for <" << name << ">"
                      << " size " << buffer.size() << " need " << desc->size << endl;
       return;
   }

   SANE_Status sane_stat = sane_control_option( KScanDevice::scanner_handle, *num,
                                                SANE_ACTION_GET_VALUE, buffer.data(),NULL);
   if( sane_stat != SANE_STATUS_GOOD )
   {
       kdDebug(29000) << "ERROR: Can't get value for <" << name << "> " << sane_strstatus( sane_stat ) << endl;
       return;
   }

   //kdDebug(29000) << "Setting buffer untouched to FALSE" << endl;
   buffer_untouched = false;
}


KScanOption::~KScanOption()
{
#ifdef MEM_DEBUG
    if (!buffer.isNull()) kdDebug(29000) << k_funcinfo << "Freeing " << buffer.size() << " bytes for <" << name << ">" << endl;
#endif
}


bool KScanOption::autoSetable( void )
{
  /* Refresh description */
  desc = getOptionDesc( name );

  return( desc && ((desc->cap & SANE_CAP_AUTOMATIC) > 0 ) );
}

bool KScanOption::commonOption( void )
{
  /* Refresh description */
  desc = getOptionDesc( name );

  return( desc && ((desc->cap & SANE_CAP_ADVANCED) == 0) );
}


bool KScanOption::active( void )
{
  bool ret = false;
  /* Refresh description */
  desc = getOptionDesc( name );
  if( desc )
    ret = SANE_OPTION_IS_ACTIVE( desc->cap );
 			
  return( ret );
}

bool KScanOption::softwareSetable( void )
{
  /* Refresh description */
  desc = getOptionDesc( name );
  if( desc )
  {
     if( SANE_OPTION_IS_SETTABLE(desc->cap) == SANE_TRUE )
	return( true );
  }
  return( false );
}


KScanOption::WidgetType KScanOption::type() const
{
    KScanOption::WidgetType ret = KScanOption::Invalid;
	
    if (valid())
    {
        switch (desc->type)
        {	
case SANE_TYPE_BOOL:
            ret = KScanOption::Bool;
            break;

case SANE_TYPE_INT:
case SANE_TYPE_FIXED:
            if (desc->constraint_type == SANE_CONSTRAINT_RANGE)
	    {
                if (desc->unit==SANE_UNIT_DPI) ret = KScanOption::Resolution;
                else
                {
                    /* FIXME ! Dies scheint nicht wirklich so zu sein */
                    /* in other words: This does not really seem to be the case */
                    if (desc->size==sizeof(SANE_Word)) ret = KScanOption::Range;
                    else ret = KScanOption::GammaTable;
                }
            }
	    else if(desc->constraint_type==SANE_CONSTRAINT_NONE)
	    {
                ret = KScanOption::SingleValue;
	    }
	    else if (desc->constraint_type==SANE_CONSTRAINT_WORD_LIST)
	    {
		ret = KScanOption::StringList;
	    }
	    else
	    {
                ret = KScanOption::Invalid;
	    }
	    break;

case SANE_TYPE_STRING:
            if (QCString(desc->name)=="filename") ret = KScanOption::File;
            else if (desc->constraint_type==SANE_CONSTRAINT_STRING_LIST) ret = KScanOption::StringList;
	    else ret = KScanOption::String;
	    break;

default:    ret = KScanOption::Invalid;
	    break;
        }
    }

    //kdDebug(29000) << k_funcinfo << "for SANE type " << (desc ? desc->type : -1) << " returning " << ret << endl;
    return (ret);
}


bool KScanOption::set(int val)
{
    if (!valid() || buffer.isNull()) return (false);
#ifdef GETSET_DEBUG
    kdDebug(29000) << k_funcinfo << "Setting <" << name << "> to " << val << endl;
#endif

    int word_size;
    QMemArray<SANE_Word> qa;
    SANE_Word sw;

    switch (desc->type)
    {
case SANE_TYPE_BOOL:					// Assign a Boolean value
        sw = (val ? SANE_TRUE : SANE_FALSE);
        buffer.duplicate(((const char *) &sw),sizeof(SANE_Word));
        break;

case SANE_TYPE_INT:					// Fill the whole buffer with that value
	word_size = desc->size/sizeof(SANE_Word);
	qa.resize(word_size);
        sw = static_cast<SANE_Word>(val);
	qa.fill(sw);
        buffer.duplicate(((const char *) qa.data()),desc->size);
	break;

case SANE_TYPE_FIXED:					// Fill the whole buffer with that value
        word_size = desc->size/sizeof(SANE_Word);
        qa.resize(word_size);
        sw = SANE_FIX(static_cast<double>(val));
        qa.fill(sw);
        buffer.duplicate(((const char *) qa.data()),desc->size);
        break;

default:
	kdDebug(29000) << k_funcinfo << "Can't set <" << name << "> with this type" << endl;
        return (false);
        break;
    }

    buffer_untouched = false;
#ifdef APPLY_IN_SITU
    applyVal();
#endif
    //emit( optionChanged( this ));
    return (true);
}



bool KScanOption::set(double val)
{
    if (!valid() || buffer.isNull()) return (false);
#ifdef GETSET_DEBUG
    kdDebug(29000) << k_funcinfo << "Setting <" << name << "> to " << val << endl;
#endif

    int word_size;
    QMemArray<SANE_Word> qa;
    SANE_Word sw;

    switch (desc->type)
    {
case SANE_TYPE_BOOL:					// Assign a Boolean value
        sw = (val>0 ? SANE_TRUE : SANE_FALSE);
        buffer.duplicate(((const char *) &sw),sizeof(SANE_Word));
        break;

case SANE_TYPE_INT:					// Fill the whole buffer with that value
        word_size = desc->size/sizeof(SANE_Word);
        qa.resize(word_size);
        sw = static_cast<SANE_Word>(val);
        qa.fill(sw);
        buffer.duplicate(((const char *) qa.data()),desc->size);
        break;

case SANE_TYPE_FIXED:					// Fill the whole buffer with that value
        word_size = desc->size/sizeof(SANE_Word);
        qa.resize(word_size);
        sw = SANE_FIX(val);
        qa.fill(sw);
        buffer.duplicate(((const char *) qa.data()),desc->size);
        break;

default:
	kdDebug(29000) << k_funcinfo << "Can't set <" << name << "> with this type" << endl;
        return (false);
        break;
   }

    buffer_untouched = false;
#ifdef APPLY_IN_SITU
    applyVal();
#endif
    //emit optionChanged( this );
    return (true);
}


bool KScanOption::set(int *val,int size)
{
    if (!valid() || buffer.isNull()) return (false);
    if (val==NULL) return (false);
#ifdef GETSET_DEBUG
    kdDebug(29000) << k_funcinfo << "Setting <" << name << "> to [" << size << "]" << endl;
#endif

    int offset = 0;
    int word_size = desc->size/sizeof(SANE_Word);
    QMemArray<SANE_Word> qa(1+word_size);		/* add 1 in case offset is needed */

#if 0
    if( desc->constraint_type == SANE_CONSTRAINT_WORD_LIST )
    {
	/* That means that the first entry must contain the size */
	kdDebug(29000) << "Size: " << size << ", word_size: " << word_size << ", descr-size: "<< desc->size << endl;
	qa[0] = (SANE_Word) 1+size;
	kdDebug(29000) << "set length field to " << qa[0] <<endl;
	offset = 1;
    }
#endif

    switch (desc->type)
    {
case SANE_TYPE_INT:
        for (int i = 0; i<word_size; i++)
        {
            if (i<size) qa[offset+i] = (SANE_Word) *(val++);
            else qa[offset+i] = (SANE_Word) *val;
        }
        break;

case SANE_TYPE_FIXED:
        for (int i = 0; i<word_size; i++)
        {
            if (i<size) qa[offset+i] = SANE_FIX((double) *(val++));
            else qa[offset+i] = SANE_FIX((double) *val);
        }
        break;

default:
	kdDebug(29000) << k_funcinfo << "Can't set <" << name << "> with this type" << endl;
        return (false);
        break;
    }

    int copybyte = desc->size;
    if (offset) copybyte += sizeof(SANE_Word);

    //kdDebug(29000) << "Copying " << copybyte << " byte to options buffer" << endl;
    buffer.duplicate(((const char *) qa.data()),copybyte);
    buffer_untouched = false;
#ifdef APPLY_IN_SITU
    applyVal();
#endif
    //emit( optionChanged( this ));
    return (true);
}


bool KScanOption::set(const QCString &c_string)
{
    if (!valid() || buffer.isNull()) return (false);
#ifdef GETSET_DEBUG
    kdDebug(29000) << k_funcinfo << "Setting <" << name << "> to '" << c_string << "'" << endl;
#endif

    int val;
   
    /* Check if it is a gammatable. If it is, convert to KGammaTable and call
     *  the approbiate set method.
     */
    QRegExp re( "\\d+, \\d+, \\d+" );
    re.setMinimal(true);
    if (QString(c_string).contains(re))
    {
        QStringList relist = QStringList::split( ", ", QString(c_string) );
      
        int brig = (relist[0]).toInt();
        int contr = (relist[1]).toInt();
        int gamm = (relist[2]).toInt();

        KGammaTable gt( brig, contr, gamm );
        kdDebug(29000) << "Setting GammaTable with int vals " << brig << "," << contr << "," << gamm << endl;
        return (set(&gt));
    }

    /* On String-type the buffer gets malloced in Constructor */
    switch (desc->type)
    {
case SANE_TYPE_STRING:
        buffer.duplicate(c_string.data(),(c_string.length()+1));
        break;

case SANE_TYPE_INT:
case SANE_TYPE_FIXED:
        bool ok;
        val = c_string.toInt(&ok);
        if (ok) set(&val,1);
        else
        {
            kdDebug(29000) << k_funcinfo << "Conversion of string value [" << c_string << "] failed!" << endl;
            return (false);
        }
        break;

case SANE_TYPE_BOOL:
        val = (c_string=="true") ? 1 : 0;
        set(val);
        break;

default:
	kdDebug(29000) << k_funcinfo << "Can't set <" << name << "> with this type" << endl;
        return (false);
        break;
    }

    buffer_untouched = false;
#ifdef APPLY_IN_SITU
    applyVal();
#endif
    //emit( optionChanged( this ));
    return (true);
}


bool KScanOption::set(KGammaTable *gt)
{
    if (!valid() || buffer.isNull()) return (false);
#ifdef GETSET_DEBUG
    kdDebug(29000) << k_funcinfo << "Setting <" << name << ">" << endl;
#endif

    int size = gt->tableSize();
    SANE_Word *run = gt->getTable();

    int word_size = desc->size/sizeof(SANE_Word);
    QMemArray<SANE_Word> qa(word_size);

    switch (desc->type)
    {
case SANE_TYPE_INT:
        for (int i = 0; i<word_size; i++)
        {
	    if (i<size) qa[i] = *run++;
	    else qa[i] = *run;
        }
        break;

case SANE_TYPE_FIXED:
        for (int i = 0; i<word_size; i++)
        {
	    if (i<size) qa[i] = SANE_FIX((double) *(run++));
	    else qa[i] = SANE_FIX((double) *run);
        }
        break;

default:
	kdDebug(29000) << k_funcinfo << "Can't set <" << name << "> with this type" << endl;
        return (false);
        break;
    }

    /* Remember raw values */
    gamma      = gt->getGamma();
    brightness = gt->getBrightness();
    contrast   = gt->getContrast();

    buffer.duplicate(((const char *) qa.data()),desc->size);
    buffer_untouched = false;
#ifdef APPLY_IN_SITU
    applyVal();
#endif
    //emit( optionChanged( this ));
    return (true);
}


bool KScanOption::get(int *val) const
{
    if (!valid() || buffer.isNull()) return (false);

    SANE_Word sane_word;
    double d;
			
    switch (desc->type)
    {
case SANE_TYPE_BOOL:					/* Buffer has a SANE_Word */
        sane_word = *((SANE_Word *) buffer.data());
        *val = (sane_word==SANE_TRUE ? 1 : 0);
        break;

case SANE_TYPE_INT:					/* reading just the first is OK */
        sane_word = *((SANE_Word *) buffer.data());
        *val = sane_word;
        break;

case SANE_TYPE_FIXED:					/* reading just the first is OK */
        d = SANE_UNFIX(*((SANE_Word *) buffer.data()));
        *val = static_cast<int>(d);
        break;

default:
	kdDebug(29000) << k_funcinfo << "Can't get <" << name << "> as this type" << endl;
        return (false);
    }

#ifdef GETSET_DEBUG
    kdDebug(29000) << k_funcinfo << "Returning <" << name << "> as " << *val << endl;
#endif
    return (true);
}


QCString KScanOption::get() const
{
    if (!valid() || buffer.isNull()) return ("parametererror");

    QCString retstr;
    SANE_Word sane_word;

    /* Handle gamma-table correctly */
    if (type()==KScanOption::GammaTable)
    {
        retstr.sprintf( "%d, %d, %d", gamma, brightness, contrast );
    }
    else
    {
        switch (desc->type)
        {
case SANE_TYPE_BOOL:
            sane_word = *((SANE_Word *) buffer.data());
            retstr = (sane_word==SANE_TRUE ? "true" : "false");
            break;

case SANE_TYPE_STRING:
            retstr = (const char*) buffer.data();
            break;

case SANE_TYPE_INT:
            sane_word = *((SANE_Word *) buffer.data());
            retstr.setNum(sane_word);
            break;

case SANE_TYPE_FIXED:
            sane_word = (SANE_Word) SANE_UNFIX(*((SANE_Word *) buffer.data()));
            retstr.setNum(sane_word);
            break;
	
default:
            kdDebug(29000) << k_funcinfo << "Can't get <" << name << "> as this type" << endl;
            retstr = "unknown";
            break;
        }
    }

#ifdef GETSET_DEBUG
    kdDebug(29000) << k_funcinfo << "Returning <" << name << "> as '" << retstr << "'" << endl;
#endif
    return (retstr);
}


/* Caller needs to have the space ;) */
bool KScanOption::get( KGammaTable *gt ) const
{
    if (gt==NULL) return (false);

    gt->setAll( gamma, brightness, contrast );
    // gt->calcTable();
    return (true);
}



QStrList KScanOption::getList() const
{
    if (desc==NULL) return (false);
    const char **sstring = NULL;
    QStrList strList;

    if (desc->constraint_type==SANE_CONSTRAINT_STRING_LIST)
    {
        sstring = (const char **)desc->constraint.string_list;

        while (*sstring!=NULL)
        {
            strList.append(*sstring);
            sstring++;
        }
   }
   else if (desc->constraint_type==SANE_CONSTRAINT_WORD_LIST)
   {
       const SANE_Int *sint = desc->constraint.word_list;
       int amount_vals = *sint;
       sint++;
       QString s;

       for (int i = 0; i<amount_vals; i++)
       {
           if (desc->type==SANE_TYPE_FIXED) s.sprintf("%f",SANE_UNFIX(*sint));
           else s.sprintf("%d",*sint);
           sint++;
           strList.append(s.local8Bit());
       }
   }
   else if (desc->constraint_type==SANE_CONSTRAINT_RANGE && type()==KScanOption::Resolution)
   {
       double min,max,q;
       int imin,imax;
       getRange( &min, &max, &q );
       imin = static_cast<int>(min);
       imax = static_cast<int>(max);

       for (const int *ip = resList; *ip!=0; ++ip)
       {
           if (*ip<imin) continue;
           if (*ip>imax) continue;
           strList.append(QString::number(*ip).ascii());
       }
   }

   return (strList);
}



bool KScanOption::getRangeFromList( double *min, double *max, double *q ) const
{
   if( !desc ) return( false );
   bool ret = true;

   if ( desc->constraint_type == SANE_CONSTRAINT_WORD_LIST ) {
	    // Try to read resolutions from word list
	    kdDebug(29000) << "Resolutions are in a word list" << endl;
            const SANE_Int *sint =  desc->constraint.word_list;
            int amount_vals = *sint; sint ++;
	    double value;
	    *min = 0;
	    *max = 0;
	    *q = -1; // What is q?

            for( int i=0; i < amount_vals; i++ ) {
		if( desc->type == SANE_TYPE_FIXED ) {
		    value = (double) SANE_UNFIX( *sint );
		} else {
		    value = *sint;
		}
		if ((*min > value) || (*min == 0)) *min = value;
		if ((*max < value) || (*max == 0)) *max = value;
		
		if( min != 0 && max != 0 && max > min ) {
		    double newq = max - min;
		    *q = newq;
		}
		sint++;
            }
   } else {
      kdDebug(29000) << "getRangeFromList: No list type " << desc->name << endl;
      ret = false;
    }
    return( ret );
}



bool KScanOption::getRange( double *min, double *max, double *q ) const
{
   if( !desc ) return( false );
   bool ret = true;

   if( desc->constraint_type == SANE_CONSTRAINT_RANGE ||
	   desc->constraint_type == SANE_CONSTRAINT_WORD_LIST ) {
      const SANE_Range *r = desc->constraint.range;

      if( desc->type == SANE_TYPE_FIXED ) {
	  *min = (double) SANE_UNFIX( r->min );
 	  *max = (double) SANE_UNFIX( r->max );
	  *q   = (double) SANE_UNFIX( r->quant );
      } else {
	  *min = r->min ;
	  *max = r->max ;
	  *q   = r->quant ;
      }
   } else {
      kdDebug(29000) << "getRange: No range type " << desc->name << endl;
      ret = false;
   }
   return( ret );
}



QWidget *KScanOption::createWidget( QWidget *parent, const QString& w_desc,
                                    const QString& tooltip )
{
    QStrList list;
    if (!valid())
    {
        kdDebug(29000) << "The option is not valid!" << endl;
	return (NULL);
    }

    delete internal_widget; internal_widget = NULL;	/* free the old widget */
 	
    text = w_desc;					/* check for text */
    if (text.isEmpty() && desc!=NULL) text = QString::fromLocal8Bit( desc->title );
    if (type()!=KScanOption::Bool) text += ":";

    kdDebug(29000) << k_funcinfo << "type=" << type() << " text=[" << text << "]" << endl;
 	
    QWidget *w = NULL;
    switch (type())
    {
case KScanOption::Bool:					/* Widget Type is ToggleButton */
	w = createToggleButton( parent, text );
	break;

case KScanOption::SingleValue:				/* Widget Type is Entry-Field */
	kdDebug(29000) << k_funcinfo << "SingleValue not implemented yet" << endl;
	break;

case KScanOption::Range:				/* Widget Type is Slider */
	w = createSlider( parent, text );
	break;

case KScanOption::Resolution:				/* Widget Type is Resolution */
	w = createComboBox( parent, text );
	break;

case KScanOption::GammaTable:				/* Widget Type is Gamma Table */
	kdDebug(29000) << k_funcinfo << "GammaTable not implemented here" << endl;
	break;						// No widget, needs to be a set !

case KScanOption::StringList:	 		
	w = createComboBox( parent, text );		/* Widget Type is Selection Box */
	break;

case KScanOption::String:
	w = createEntryField( parent, text );		/* Widget Type is String Entry */
	break;

case KScanOption::File:
	w = createFileField( parent, text );		/* Widget Type is Filename */
        break;

default:
	kdDebug(29000) << k_funcinfo << "cannot create widget for unknown type " << type() << endl;
	break;
    }
 	
    if (w!=NULL)
    {
        internal_widget = w;
	connect(this, SIGNAL(optionChanged(KScanOption *)),SLOT(slRedrawWidget(KScanOption *)));

	QString tt = tooltip;
	if (tt.isEmpty() && desc!=NULL) tt = QString::fromLocal8Bit( desc->desc );
	if (!tt.isEmpty()) QToolTip::add(internal_widget, i18n(tt.utf8()));
    }
 	
    slReload();						/* Check if option is active, setEnabled etc. */
    if (w!=NULL) slRedrawWidget(this);
    return (w);
}


inline QWidget *KScanOption::createToggleButton( QWidget *parent, const QString& text )
{
    QWidget *cb = new QCheckBox( i18n(text.utf8()), parent);
    connect( cb, SIGNAL(clicked()), SLOT(slWidgetChange()));
    return (cb);
}


inline QWidget *KScanOption::createComboBox( QWidget *parent, const QString& text )
{
    QStrList list = getList();
    KScanCombo *cb = new KScanCombo( parent, text, list);
    connect(cb, SIGNAL(valueChanged(const QCString&)), SLOT(slWidgetChange(const QCString&)));
    return (cb);
}


inline QWidget *KScanOption::createEntryField( QWidget *parent, const QString& text )
{
    KScanEntry *ent = new KScanEntry( parent, text );
    connect(ent, SIGNAL(valueChanged(const QCString &)), SLOT(slWidgetChange(const QCString &)));
    return (ent);
}


inline QWidget *KScanOption::createSlider( QWidget *parent, const QString& text )
{
    double min, max, quant;
    getRange( &min, &max, &quant );

    KScanSlider *slider = new KScanSlider( parent, text, min, max,true );
    connect(slider, SIGNAL(valueChanged(int)), SLOT(slWidgetChange(int)));
    return (slider);
}


inline QWidget *KScanOption::createFileField( QWidget *parent, const QString& text )
{
    KScanFileRequester *req = new KScanFileRequester(parent,text);
    connect( req, SIGNAL(valueChanged(const QCString &)), SLOT(slWidgetChange(const QCString &)));
    return (req);
}


void KScanOption::allocForDesc()
{
    if (desc==NULL) return;
    switch (desc->type)
    {
case SANE_TYPE_INT:
case SANE_TYPE_FIXED:
case SANE_TYPE_STRING:		
        allocBuffer(desc->size);
        break;

case SANE_TYPE_BOOL:
        allocBuffer(sizeof(SANE_Word));
        break;

default:
        if (desc->size>0) allocBuffer(desc->size);
        break;
    }
}


void KScanOption::allocBuffer(long size)
{
    if (size<1) return;

#ifdef MEM_DEBUG
    kdDebug(29000) << k_funcinfo << "Allocating " << size << " bytes for <" << name << ">" << endl;
#endif
    if (!buffer.resize(size))				// set buffer size
    {
        kdDebug(29000) << k_funcinfo << "FATAL: Allocating " << size << " bytes for <" << name << "> failed!" << endl;
        return;
    }

    buffer.fill(0);					// clear allocated buffer
}


#ifdef APPLY_IN_SITU
bool KScanOption::applyVal( void )
{
   bool res = true;
   int *idx = KScanDevice::option_dic->find(name);

   if( *idx == 0 ) return( false );
   if(buffer.isNull() )  return( false );

   SANE_Status stat = sane_control_option ( KScanDevice::scanner_handle, *idx,
					    SANE_ACTION_SET_VALUE, buffer.data(),
					    NULL);
   if( stat != SANE_STATUS_GOOD )
   {
      kdDebug(29000) << "Error in in situ appliance " << getName() << ": " << sane_strstatus( stat ) << endl;
      res = false;
   }
   else
   {
      kdDebug(29000) << "IN SITU appliance " << getName() << ": OK" << endl;

   }
   return( res );
}	
#endif


QLabel *KScanOption::getLabel(QWidget *parent) const
{
    QString t = text;
    if (internal_widget->isA("QCheckBox")) t = QString::null;
    return (new QLabel(t,parent));
}
