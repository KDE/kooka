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

#include "kscanoption.h"
#include "kscanoption.moc"

#include <qwidget.h>
#include <qobject.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qregexp.h>

#include <kdebug.h>
#include <klocale.h>
#include <kacceleratormanager.h>
#include <ksqueezedtextlabel.h>

extern "C" {
#include <sane/sane.h>
}

#include "kgammatable.h"
#include "kscandevice.h"
#include "kscancontrols.h"
#include "kscanoptset.h"


//  Debugging options
#undef DEBUG_MEM
#undef DEBUG_GETSET
#undef DEBUG_RELOAD

#undef APPLY_IN_SITU


//  This defines the possible resolutions that will be shown by the combo.
//  Only resolutions from this list falling within the scanner's allowed range
//  will be included.
static const int resList[] = { 50, 75, 100, 150, 200, 300, 600, 900, 1200, 1800, 2400, 4800, 9600, 0 };


KScanOption::KScanOption(const QByteArray &name)
{
    if (!initOption(name))
    {
        kDebug() << "initOption for" << name << "failed!";
        return;
    }

    if (mBuffer.isNull()) return;
    SANE_Status sane_stat = sane_control_option(KScanDevice::instance()->scannerHandle(),
                                                mIndex,
                                                SANE_ACTION_GET_VALUE,
                                                mBuffer.data(), NULL);
    if (sane_stat==SANE_STATUS_GOOD) mBufferClean = false;
}


bool KScanOption::initOption(const QByteArray &name)
{
    mDesc = NULL;
    mControl = NULL;
    mIsGroup = false;
    mIsReadable = true;

    if (name.isEmpty()) return (false);
    mName = name;

    // Look up the option (which must already exist in the map) by name.
    //
    // The default-constructed index is 0 for an invalid option, this is OK
    // because although a SANE option with that index exists it is never
    // requested by name.
    mIndex = KScanDevice::instance()->getOptionIndex(mName);
    if (mIndex<=0)
    {
        kDebug() << "no option descriptor for" << mName;
        return (false);
    }

    mDesc = sane_get_option_descriptor(KScanDevice::instance()->scannerHandle(),
                                       mIndex);
    if (mDesc==NULL) return (false);

    mBuffer.resize(0);
    mBufferClean = true;

    if (mDesc->type==SANE_TYPE_GROUP) mIsGroup = true;
    if (mIsGroup || mDesc->type==SANE_TYPE_BUTTON) mIsReadable = false;
    if (!(mDesc->cap & SANE_CAP_SOFT_DETECT)) mIsReadable = false;

    /* Gamma-Table - initial values */
    gamma = 0; /* marks as unvalid */
    brightness = 0;
    contrast = 0;
    gamma = 100;
	
    allocForDesc();

    KScanOption *gtOption = KScanDevice::instance()->gammaTables()->get(name);
    if( gtOption!=NULL)
    {
        kDebug() << "Is older GammaTable!";
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
    mDesc = so.mDesc;					// stored by sane-lib -> may be copied
    mName = so.mName;					// QCString explicit sharing -> shallow copy
    mIsGroup = so.mIsGroup;
    mIsReadable = so.mIsReadable;
    mControl = NULL;					// the widget is not copied!

    gamma = so.gamma;
    brightness = so.brightness;
    contrast = so.contrast;

    if (mDesc==NULL && mName.isNull())
    {
        kWarning() << "Trying to copy a not healthy option (no name nor desc)";
        return;
    }

    mBuffer = so.mBuffer;				// QByteArray -> deep copy
    mBufferClean = so.mBufferClean;
}


const KScanOption &KScanOption::operator=(const KScanOption &so)
{
    if (this==&so) return (*this);

    mDesc = so.mDesc;					// stored by sane-lib -> may be copied
    mName = so.mName;					// QCString explicit sharing -> shallow copy
    mIsGroup = so.mIsGroup;
    mIsReadable = so.mIsReadable;

    gamma = so.gamma;
    brightness = so.brightness;
    contrast = so.contrast;

    delete mControl;					// widget *is* copied here
    mControl = so.mControl;

    mBuffer.resize(0);					// throw away old buffer
    mBuffer = so.mBuffer;				// QByteArray -> deep copy
    mBufferClean = so.mBufferClean;

    return (*this);
}


KScanOption::~KScanOption()
{
#ifdef DEBUG_MEM
    if (!mBuffer.isNull()) kDebug() << "Freeing" << mBuffer.size() << "bytes for" << mName;
#endif
}


void KScanOption::slotWidgetChange(const QString &t)
{
    set(t.toUtf8());
    emit guiChange(this);
}


void KScanOption::slotWidgetChange(int i)
{
    set(i);
    emit guiChange(this);
}


void KScanOption::slotWidgetChange()
{
    set(1);
    emit guiChange(this);
}


/* called on a widget change, if a widget was created.
 */
void KScanOption::redrawWidget()
{
    if (!isValid() || !isReadable() || mControl==NULL || getBuffer()==NULL) return;

    KScanControl::ControlType type = mControl->type();
    if (type==KScanControl::Number)			// numeric control
    {
        int i = 0;
        if (get(&i)) mControl->setValue(i);
    }
    else if (type==KScanControl::Text)			// text control
    {
        mControl->setText(get());
    }
}


/* Get and update the current setting from the scanner */
void KScanOption::reload()
{
    if (mControl!=NULL)
    {
        if (isGroup())
        {
            mControl->setEnabled(true);			// always enabled
            return;					// no more to do
        }

        if (!isActive())
        {
#ifdef DEBUG_RELOAD
            kDebug() << "not active" << mName;
#endif
            mControl->setEnabled(false);
        }
        else if (!isSoftwareSettable())
        {
#ifdef DEBUG_RELOAD
            kDebug() << "not settable" << mName;
#endif
            mControl->setEnabled(false);
        }
        else mControl->setEnabled(true);
    }

    if (!isReadable())
    {
#ifdef DEBUG_RELOAD
        kDebug() << "not readable" << mName;
#endif
        return;
    }

    if (mBuffer.isNull())				// first get mem if needed
    {
        kDebug() << "need to allocate now";
        allocForDesc();					// allocate the buffer now
    }

    if (!isActive()) return;

    if (mDesc->size>mBuffer.size())
    {
        kDebug() << "buffer too small for" << mName << "type" << mDesc->type
                 << "size" << mBuffer.size() << "need" << mDesc->size;
        allocForDesc();					// grow the buffer
    }

    SANE_Status sane_stat = sane_control_option(KScanDevice::instance()->scannerHandle(),
                                                mIndex,
                                                SANE_ACTION_GET_VALUE,
                                                mBuffer.data(), NULL);
    if (sane_stat!=SANE_STATUS_GOOD)
    {
        kDebug() << "Error: Can't get value for" << mName << "status" << sane_strstatus( sane_stat );
        return;
    }

    mBufferClean = false;
}


bool KScanOption::isAutoSettable() const
{
    return (mDesc!=NULL && (mDesc->cap & SANE_CAP_AUTOMATIC));
}


bool KScanOption::isCommonOption() const
{
    return (mDesc!=NULL && !(mDesc->cap & SANE_CAP_ADVANCED));
}


bool KScanOption::isActive() const
{
    return (mDesc!=NULL && SANE_OPTION_IS_ACTIVE(mDesc->cap));
}


bool KScanOption::isSoftwareSettable() const
{
    return (mDesc!=NULL && SANE_OPTION_IS_SETTABLE(mDesc->cap));
}


KScanOption::WidgetType KScanOption::type() const
{
    KScanOption::WidgetType ret = KScanOption::Invalid;
	
    if (isValid())
    {
        switch (mDesc->type)
        {	
case SANE_TYPE_BOOL:
            ret = KScanOption::Bool;
            break;

case SANE_TYPE_INT:
case SANE_TYPE_FIXED:
            if (mDesc->constraint_type == SANE_CONSTRAINT_RANGE)
	    {
                if (mDesc->unit==SANE_UNIT_DPI) ret = KScanOption::Resolution;
                else
                {
                    /* FIXME ! Dies scheint nicht wirklich so zu sein */
                    /* in other words: This does not really seem to be the case */
                    if (mDesc->size==sizeof(SANE_Word)) ret = KScanOption::Range;
                    else ret = KScanOption::GammaTable;
                }
            }
	    else if(mDesc->constraint_type==SANE_CONSTRAINT_NONE)
	    {
                ret = KScanOption::SingleValue;
	    }
	    else if (mDesc->constraint_type==SANE_CONSTRAINT_WORD_LIST)
	    {
		ret = KScanOption::StringList;
	    }
	    else
	    {
                ret = KScanOption::Invalid;
	    }
	    break;

case SANE_TYPE_STRING:
            if (QString::compare(mDesc->name,"filename")==0) ret = KScanOption::File;
            else if (mDesc->constraint_type==SANE_CONSTRAINT_STRING_LIST) ret = KScanOption::StringList;
	    else ret = KScanOption::String;
	    break;

case SANE_TYPE_BUTTON:
            ret = KScanOption::Button;
            break;

case SANE_TYPE_GROUP:
	    ret = KScanOption::Group;
	    break;

default:    kDebug() << "unsupported SANE type" << mDesc->type;
	    ret = KScanOption::Invalid;
	    break;
        }
    }

#ifdef DEBUG_GETSET
    kDebug() << "for SANE type" << (mDesc ? mDesc->type : -1) << "returning" << ret;
#endif
    return (ret);
}


bool KScanOption::set(int val)
{
    if (!isValid() || mBuffer.isNull()) return (false);
#ifdef DEBUG_GETSET
    kDebug() << "Setting" << mName << "to" << val;
#endif

    int word_size;
    QVector<SANE_Word> qa;
    SANE_Word sw;

    switch (mDesc->type)
    {
case SANE_TYPE_BUTTON:					// Activate a button
case SANE_TYPE_BOOL:					// Assign a Boolean value
        sw = (val ? SANE_TRUE : SANE_FALSE);
        mBuffer = QByteArray(((const char *) &sw),sizeof(SANE_Word));
        break;

case SANE_TYPE_INT:					// Fill the whole buffer with that value
	word_size = mDesc->size/sizeof(SANE_Word);
	qa.resize(word_size);
        sw = static_cast<SANE_Word>(val);
	qa.fill(sw);
        mBuffer = QByteArray(((const char *) qa.data()),mDesc->size);
	break;

case SANE_TYPE_FIXED:					// Fill the whole buffer with that value
        word_size = mDesc->size/sizeof(SANE_Word);
        qa.resize(word_size);
        sw = SANE_FIX(static_cast<double>(val));
        qa.fill(sw);
        mBuffer = QByteArray(((const char *) qa.data()),mDesc->size);
        break;

default:
	kDebug() << "Can't set" << mName << "with this type";
        return (false);
        break;
    }

    mBufferClean = false;
#ifdef APPLY_IN_SITU
    applyVal();
#endif
    return (true);
}



bool KScanOption::set(double val)
{
    if (!isValid() || mBuffer.isNull()) return (false);
#ifdef DEBUG_GETSET
    kDebug() << "Setting" << mName << "to" << val;
#endif

    int word_size;
    QVector<SANE_Word> qa;
    SANE_Word sw;

    switch (mDesc->type)
    {
case SANE_TYPE_BOOL:					// Assign a Boolean value
        sw = (val>0 ? SANE_TRUE : SANE_FALSE);
        mBuffer = QByteArray(((const char *) &sw),sizeof(SANE_Word));
        break;

case SANE_TYPE_INT:					// Fill the whole buffer with that value
        word_size = mDesc->size/sizeof(SANE_Word);
        qa.resize(word_size);
        sw = static_cast<SANE_Word>(val);
        qa.fill(sw);
        mBuffer = QByteArray(((const char *) qa.data()),mDesc->size);
        break;

case SANE_TYPE_FIXED:					// Fill the whole buffer with that value
        word_size = mDesc->size/sizeof(SANE_Word);
        qa.resize(word_size);
        sw = SANE_FIX(val);
        qa.fill(sw);
        mBuffer = QByteArray(((const char *) qa.data()),mDesc->size);
        break;

default:
	kDebug() << "Can't set" << mName << "with this type";
        return (false);
   }

    mBufferClean = false;
#ifdef APPLY_IN_SITU
    applyVal();
#endif
    return (true);
}


bool KScanOption::set(const int *val, int size)
{
    if (!isValid() || mBuffer.isNull()) return (false);
    if (val==NULL) return (false);
#ifdef DEBUG_GETSET
    kDebug() << "Setting" << mName << "to" << size;
#endif

    int offset = 0;
    int word_size = mDesc->size/sizeof(SANE_Word);
    QVector<SANE_Word> qa(1+word_size);		/* add 1 in case offset is needed */

#if 0
    if( mDesc->constraint_type == SANE_CONSTRAINT_WORD_LIST )
    {
	/* That means that the first entry must contain the size */
	kDebug() << "Size" << size << "word_size" << word_size << "mDescr-size"<< mDesc->size;
	qa[0] = (SANE_Word) 1+size;
	kDebug() << "set length field to" << qa[0];
	offset = 1;
    }
#endif

    switch (mDesc->type)
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
	kDebug() << "Can't set" << mName << "with this type";
        return (false);
        break;
    }

    int copybyte = mDesc->size;
    if (offset) copybyte += sizeof(SANE_Word);

    //kdDebug(29000) << "Copying " << copybyte << " byte to options buffer" << endl;
    mBuffer = QByteArray(((const char *) qa.data()),copybyte);
    mBufferClean = false;
#ifdef APPLY_IN_SITU
    applyVal();
#endif
    return (true);
}


bool KScanOption::set(const QByteArray &c_string)
{
    if (!isValid() || mBuffer.isNull()) return (false);
#ifdef DEBUG_GETSET
    kDebug() << "Setting" << mName << "to" << c_string;
#endif

    int val;
    int origSize;

    /* Check if it is a gammatable. If it is, convert to KGammaTable and call
     *  the approbiate set method.
     */
    QRegExp re( "\\d+,\\d+,\\d+" );
    re.setMinimal(true);
    QString s(c_string);
    if (s.contains(re))
    {
        QStringList relist = s.split(",");

        int brig = (relist[0]).toInt();
        int contr = (relist[1]).toInt();
        int gamm = (relist[2]).toInt();

        KGammaTable gt( brig, contr, gamm );
        kDebug() << "Setting GammaTable with int vals" << brig << contr << gamm;
        return (set(&gt));
    }

    /* On String-type the buffer gets malloced in Constructor */
    switch (mDesc->type)
    {
case SANE_TYPE_STRING:
        origSize = mBuffer.size();
        mBuffer = QByteArray(c_string.data(),(c_string.length()+1));
        mBuffer.resize(origSize);			// restore original size
        break;

case SANE_TYPE_INT:
case SANE_TYPE_FIXED:
        bool ok;
        val = c_string.toInt(&ok);
        if (ok) set(&val,1);
        else
        {
            kDebug() << "Conversion of string value" << c_string << "failed!";
            return (false);
        }
        break;

case SANE_TYPE_BOOL:
        val = (c_string=="true") ? 1 : 0;
        set(val);
        break;

default:
	kDebug() << "Can't set" << mName << "with this type";
        return (false);
        break;
    }

    mBufferClean = false;
#ifdef APPLY_IN_SITU
    applyVal();
#endif
    return (true);
}


bool KScanOption::set(KGammaTable *gt)
{
    if (!isValid() || mBuffer.isNull()) return (false);
#ifdef DEBUG_GETSET
    kDebug() << "Setting gamma table for" << mName;
#endif

    int size = gt->tableSize();
    SANE_Word *run = gt->getTable();

    int word_size = mDesc->size/sizeof(SANE_Word);
    QVector<SANE_Word> qa(word_size);

    switch (mDesc->type)
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
	kDebug() << "Can't set" << mName << "with this type";
        return (false);
    }

    /* Remember raw values */
    gamma      = gt->getGamma();
    brightness = gt->getBrightness();
    contrast   = gt->getContrast();

    mBuffer = QByteArray(((const char *) qa.data()),mDesc->size);
    mBufferClean = false;
#ifdef APPLY_IN_SITU
    applyVal();
#endif
    return (true);
}


bool KScanOption::get(int *val) const
{
    if (!isValid() || mBuffer.isNull()) return (false);

    SANE_Word sane_word;
    double d;
			
    switch (mDesc->type)
    {
case SANE_TYPE_BOOL:					/* Buffer has a SANE_Word */
        sane_word = *((SANE_Word *) mBuffer.data());
        *val = (sane_word==SANE_TRUE ? 1 : 0);
        break;

case SANE_TYPE_INT:					/* reading just the first is OK */
        sane_word = *((SANE_Word *) mBuffer.data());
        *val = sane_word;
        break;

case SANE_TYPE_FIXED:					/* reading just the first is OK */
        d = SANE_UNFIX(*((SANE_Word *) mBuffer.data()));
        *val = static_cast<int>(d);
        break;

default:
	kDebug() << "Can't get" << mName << "as this type";
        return (false);
    }

#ifdef DEBUG_GETSET
    kDebug() << "Returning" << mName << "as" << *val;
#endif
    return (true);
}


QByteArray KScanOption::get() const
{
    if (!isValid() || mBuffer.isNull()) return (PARAM_ERROR);

    QByteArray retstr;
    SANE_Word sane_word;

    /* Handle gamma-table correctly */
    if (type()==KScanOption::GammaTable)
    {
        retstr = QString("%1,%2,%3").arg(gamma).arg(brightness).arg(contrast).toLocal8Bit();
    }
    else
    {
        switch (mDesc->type)
        {
case SANE_TYPE_BOOL:
            sane_word = *((SANE_Word *) mBuffer.data());
            retstr = (sane_word==SANE_TRUE ? "true" : "false");
            break;

case SANE_TYPE_STRING:
            retstr = (const char*) mBuffer.data();
            break;

case SANE_TYPE_INT:
            sane_word = *((SANE_Word *) mBuffer.data());
            retstr.setNum(sane_word);
            break;

case SANE_TYPE_FIXED:
            sane_word = (SANE_Word) SANE_UNFIX(*((SANE_Word *) mBuffer.data()));
            retstr.setNum(sane_word);
            break;
	
default:
            kDebug() << "Can't get" << mName << "as this type";
            retstr = "unknown";
            break;
        }
    }

#ifdef DEBUG_GETSET
    kDebug() << "Returning" << mName << "as" << retstr;
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



QList<QByteArray> KScanOption::getList() const
{
    const char **sstring = NULL;
    QList<QByteArray> strList;
    if (mDesc==NULL) return (strList);

    if (mDesc->constraint_type==SANE_CONSTRAINT_STRING_LIST)
    {
        sstring = (const char **)mDesc->constraint.string_list;

        while (*sstring!=NULL)
        {
            strList.append(*sstring);
            sstring++;
        }
   }
   else if (mDesc->constraint_type==SANE_CONSTRAINT_WORD_LIST)
   {
       const SANE_Int *sint = mDesc->constraint.word_list;
       int amount_vals = *sint;
       sint++;
       QString s;

       for (int i = 0; i<amount_vals; i++)
       {
           if (mDesc->type==SANE_TYPE_FIXED) s.sprintf("%f",SANE_UNFIX(*sint));
           else s.sprintf("%d",*sint);
           sint++;
           strList.append(s.toLocal8Bit());
       }
   }
   else if (mDesc->constraint_type==SANE_CONSTRAINT_RANGE && type()==KScanOption::Resolution)
   {
       double min,max;
       int imin,imax;
       getRange( &min, &max);
       imin = static_cast<int>(min);
       imax = static_cast<int>(max);

       for (const int *ip = resList; *ip!=0; ++ip)
       {
           if (*ip<imin) continue;
           if (*ip>imax) continue;
           strList.append(QString::number(*ip).toAscii());
       }
   }

   return (strList);
}


bool KScanOption::getRange(double *minp, double *maxp, double *quantp) const
{
    if (mDesc==NULL) return (false);

    double min = 0.0;
    double max = 0.0;
    double quant = -1;

    if (mDesc->constraint_type==SANE_CONSTRAINT_RANGE)
    {
        const SANE_Range *r = mDesc->constraint.range;
        if (mDesc->type==SANE_TYPE_FIXED)
        {
            min = SANE_UNFIX(r->min);
            max = SANE_UNFIX(r->max);
            quant = SANE_UNFIX(r->quant);
        }
        else
        {
            min = r->min;
            max = r->max;
            quant = r->quant;
        }
    }
    else if (mDesc->constraint_type==SANE_CONSTRAINT_WORD_LIST)
    {
        // Originally done in KScanOption::getRangeFromList()
        const SANE_Int *wl = mDesc->constraint.word_list;
        const int num = wl[0];

        double value;
        for (int i = 1; i<=num; ++i)
        {
            if (mDesc->type==SANE_TYPE_FIXED) value = SANE_UNFIX(wl[i]);
            else value = wl[i];
            if (i==1 || value<min) min = value;
            if (i==1 || value>max) max = value;
	}

        if (num>=2) quant = (max-min)/(num-1);		// synthesise from total range
    }
    else
    {
        kDebug() << "Not a range type" << mDesc->name;
        return (false);
    }

    *minp = min;
    *maxp = max;
    if (quantp!=NULL) *quantp = quant;

    return (true);
}


KScanControl *KScanOption::createWidget(QWidget *parent, const QString &descr)
{
    if (!isValid())
    {
        kDebug() << "Option is not valid!";
	return (NULL);
    }

    delete mControl; mControl = NULL;			// dispose of the old control
 	
    mText = descr;
    if (mText.isEmpty() && mDesc!=NULL) mText = QString::fromLocal8Bit(mDesc->title);

    kDebug() << "type" << type() << "text" << mText;

    KScanControl *w = NULL;
    switch (type())
    {
case KScanOption::Bool:
	w = createToggleButton(parent, mText);		// toggle button
	break;

case KScanOption::SingleValue:
	w = createNumberEntry(parent, mText);		// numeric entry
	break;

case KScanOption::Range:
	w = createSlider(parent, mText);		// slider and spinbox
	break;

case KScanOption::Resolution:
	w = createComboBox(parent, mText);		// special resolution combo
	break;

case KScanOption::GammaTable:
	kDebug() << "GammaTable not implemented here";
	break;						// no widget for this

case KScanOption::StringList:	 		
	w = createComboBox(parent, mText);		// string list combo
	break;

case KScanOption::String:
	w = createStringEntry(parent, mText);		// free text entry
	break;

case KScanOption::File:
	w = createFileField(parent, mText);		// file name requester
        break;

case KScanOption::Group:
	w = createGroupSeparator(parent, mText);	// group separator
        break;

case KScanOption::Button:
	w = createActionButton(parent, mText);		// button to do action
        break;

default:
	kDebug() << "unknown control type " << type();
	break;
    }
 	
    if (w!=NULL)
    {
        mControl = w;

        switch (w->type())
        {
case KScanControl::Number:				// numeric control
            connect(w, SIGNAL(settingChanged(int)), SLOT(slotWidgetChange(int)));
            break;

case KScanControl::Text:				// text control
            connect(w, SIGNAL(settingChanged(const QString &)), SLOT(slotWidgetChange(const QString &)));
            break;

case KScanControl::Button:				// push button
            connect(w, SIGNAL(returnPressed()), SLOT(slotWidgetChange()));
            break;

case KScanControl::Group:				// group separator
            break;					// nothing to do here
        }

	if (mDesc!=NULL)				// set tool tip
        {
            QString tt = QString::fromLocal8Bit(mDesc->desc);
            if (!tt.isEmpty()) w->setToolTip(tt);
        }

        // No accelerators for advanced options, so as not to soak up
        // too many of the available accelerators for controls that are
        // rarely going to be used.  See also getLabel().
        if (!isCommonOption()) KAcceleratorManager::setNoAccel(w);
    }
 	
    reload();						// check if active, enabled etc.
    if (w!=NULL) redrawWidget();
    return (w);
}


inline KScanControl *KScanOption::createToggleButton(QWidget *parent, const QString &text)
{
    return (new KScanCheckbox(parent, text));
}


inline KScanControl *KScanOption::createComboBox(QWidget *parent, const QString &text)
{
    QList<QByteArray> list = getList();
    return (new KScanCombo( parent, text, list));
}


inline KScanControl *KScanOption::createStringEntry(QWidget *parent, const QString &text)
{
    return (new KScanStringEntry(parent, text));
}


inline KScanControl *KScanOption::createNumberEntry(QWidget *parent, const QString &text)
{
    return (new KScanNumberEntry(parent, text));
}


inline KScanControl *KScanOption::createSlider(QWidget *parent, const QString &text)
{
    double min, max;
    getRange(&min, &max);
    return (new KScanSlider(parent, text, min, max, true));
}


inline KScanControl *KScanOption::createFileField(QWidget *parent, const QString &text)
{
    return (new KScanFileRequester(parent, text));
}


inline KScanControl *KScanOption::createGroupSeparator(QWidget *parent, const QString &text)
{
    return (new KScanGroup(parent, text));
}


inline KScanControl *KScanOption::createActionButton(QWidget *parent, const QString &text)
{
    return (new KScanPushButton(parent, text));
}


QLabel *KScanOption::getLabel(QWidget *parent, bool alwaysBuddy) const
{
    if (mControl==NULL) return (NULL);
    KSqueezedTextLabel *l = new KSqueezedTextLabel(mControl->label(), parent);
    if (isCommonOption() || alwaysBuddy) l->setBuddy(mControl->focusProxy());
    return (l);
}


QLabel *KScanOption::getUnit(QWidget *parent) const
{
    if (mControl==NULL) return (NULL);

    QString s;
    switch (mDesc->unit)
    {
case SANE_UNIT_NONE:						break;
case SANE_UNIT_PIXEL:		s = i18n("pixels");		break;
case SANE_UNIT_BIT:		s = i18n("bits");		break;
case SANE_UNIT_MM:		s = i18n("mm");			break;
case SANE_UNIT_DPI:		s = i18n("dpi");		break;
case SANE_UNIT_PERCENT:		s = i18n("%");			break;
case SANE_UNIT_MICROSECOND:	s = i18n("\302\265sec");	break;
default:							break;
    }

    if (s.isEmpty()) return (NULL);			// no unit label
    QLabel *l = new QLabel(s, parent);
    return (l);
}


void KScanOption::allocForDesc()
{
    if (mDesc==NULL) return;

    switch (mDesc->type)
    {
case SANE_TYPE_INT:
case SANE_TYPE_FIXED:
case SANE_TYPE_STRING:		
        allocBuffer(mDesc->size);
        break;

case SANE_TYPE_BOOL:
        allocBuffer(sizeof(SANE_Word));
        break;

default:
        if (mDesc->size>0) allocBuffer(mDesc->size);
        break;
    }
}


void KScanOption::allocBuffer(long size)
{
    if (size<1) return;

#ifdef DEBUG_MEM
    kDebug() << "Allocating" << size << "bytes for" << name;
#endif

    mBuffer.resize(size);				// set buffer size
    if (mBuffer.isNull())				// check allocation worked???
    {
        kWarning() << "Fatal: Allocating" << size << "bytes for" << mName << "failed!";
        return;
    }

    mBuffer.fill(0);					// clear allocated buffer
}


#ifdef APPLY_IN_SITU
//  This seems to be done by KScanDevice::apply() instead
bool KScanOption::applyVal()
{
    if (mIndex<=0 || mBuffer.isNull()) return (false);

    SANE_Status stat = sane_control_option(KScanDevice::gScannerHandle,
                                           mIndex,
                                           SANE_ACTION_SET_VALUE,
                                           mBuffer.data(), NULL);
    if (stat!=SANE_STATUS_GOOD)
    {
        kDebug() << "Error applying" << mName << "status" << sane_strstatus(stat);
        return (false);
    }

    kDebug() << "Applied" << mName;
    return (true);
}	
#endif




