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
#include <sane/saneopts.h>
}

#include "kgammatable.h"
#include "kscandevice.h"
#include "kscancontrols.h"
#include "kscanoptset.h"


//  Debugging options
#undef DEBUG_MEM
#undef DEBUG_GETSET
#define DEBUG_APPLY
#define DEBUG_RELOAD


//  This defines the possible resolutions that will be shown by the combo.
//  Only resolutions from this list falling within the scanner's allowed range
//  will be included.
static const int resList[] = { 50, 75, 100, 150, 200, 300, 600, 900, 1200, 1800, 2400, 4800, 9600, 0 };


KScanOption::KScanOption(const QByteArray &name, KScanDevice *scandev)
{
    mScanDevice = scandev;

    if (!initOption(name))
    {
        kDebug() << "initOption for" << name << "failed!";
        return;
    }

    if (!mIsReadable) return;				// no value to read
    if (mBuffer.isNull()) return;			// no buffer for value

    // read initial value from the scanner
    SANE_Status sanestat = sane_control_option(mScanDevice->scannerHandle(),
                                                mIndex,
                                                SANE_ACTION_GET_VALUE,
                                                mBuffer.data(), NULL);
    if (sanestat==SANE_STATUS_GOOD) mBufferClean = false;
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
    mIndex = mScanDevice->getOptionIndex(mName);
    if (mIndex<=0)
    {
        kDebug() << "no option descriptor for" << mName;
        return (false);
    }

    mDesc = sane_get_option_descriptor(mScanDevice->scannerHandle(), mIndex);
    if (mDesc==NULL) return (false);

    mBuffer.resize(0);
    mBufferClean = true;
    mApplied = false;

    if (mDesc->type==SANE_TYPE_GROUP) mIsGroup = true;
    if (mIsGroup || mDesc->type==SANE_TYPE_BUTTON) mIsReadable = false;
    if (!(mDesc->cap & SANE_CAP_SOFT_DETECT)) mIsReadable = false;

    /* Gamma-Table - initial values */
    gamma = 0; /* marks as unvalid */
    brightness = 0;
    contrast = 0;
    gamma = 100;
	
    allocForDesc();
    return (true);
}


KScanOption::~KScanOption()
{
#ifdef DEBUG_MEM
    if (!mBuffer.isNull()) kDebug() << "Freeing" << mBuffer.size() << "bytes for" << mName;
#endif
    // TODO: need to delete mControl here?
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
    if (!isValid() || !isReadable() || mControl==NULL || mBuffer.isNull()) return;

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

    // Starting with SANE 1.0.20, the 'test' device needs this dummy call of
    // sane_get_option_descriptor() before any use of sane_control_option(),
    // otherwise the latter fails with a SANE_STATUS_INVAL.  From the master
    // repository at http://anonscm.debian.org/gitweb/?p=sane/sane-backends.git:
    //
    //   author  m. allan noah <kitno455@gmail.com>
    //   Thu, 26 Jun 2008 13:14:23 +0000 (13:14 +0000)
    //   commit 8733651c4b07ac6ccbcee0d39eccca0c08057729
    //   test backend checks for options that have not been loaded before being controlled
    //
    // I'm hoping that this is not in general an expensive operation - it certainly
    // is not so for the 'test' device and a sample of others - so it should not be
    // necessary to conditionalise it for that device only.

    const SANE_Option_Descriptor *desc = sane_get_option_descriptor(mScanDevice->scannerHandle(), mIndex);
    if (desc==NULL)					// should never happen
    {
        kDebug() << "sane_get_option_descriptor returned NULL for" << mName;
        return;
    }

    SANE_Status sanestat = sane_control_option(mScanDevice->scannerHandle(),
                                                mIndex,
                                                SANE_ACTION_GET_VALUE,
                                                mBuffer.data(), NULL);
    if (sanestat!=SANE_STATUS_GOOD)
    {
        kDebug() << "Error: Can't get value for" << mName << "status" << sane_strstatus(sanestat);
        return;
    }

#ifdef DEBUG_RELOAD
    kDebug() << "reloaded" << mName;
#endif
    mBufferClean = false;
}


bool KScanOption::apply()
{
    int sane_result = 0;
    bool reload = false;
#ifdef DEBUG_APPLY
    QByteArray debug = QString("option '%1'").arg(mName.constData()).toLatin1();
#endif // DEBUG_APPLY

    SANE_Status sanestat = SANE_STATUS_GOOD;

    if (mName==SANE_NAME_PREVIEW || mName==SANE_NAME_SCAN_MODE)
    {
        sanestat = sane_control_option(mScanDevice->scannerHandle(), mIndex,
                                       SANE_ACTION_SET_AUTO, 0,
                                       &sane_result);
        /* No return here, please! Carsten, does it still work than for you? */
    }

    if (!isInitialised() || mBuffer.isNull())
    {
#ifdef DEBUG_APPLY
        debug += "nobuffer";
#endif // DEBUG_APPLY

        if (!isAutoSettable()) goto ret;

#ifdef DEBUG_APPLY
        debug += " auto";
#endif // DEBUG_APPLY
        sanestat = sane_control_option(mScanDevice->scannerHandle(), mIndex,
                                       SANE_ACTION_SET_AUTO, 0,
                                       &sane_result);
    }
    else
    {
        if (!isActive())
        {
#ifdef DEBUG_APPLY
            debug += " notactive";
#endif // DEBUG_APPLY
            goto ret;
        }
        else if (!isSoftwareSettable())
        {
#ifdef DEBUG_APPLY
            debug += " notsettable";
#endif // DEBUG_APPLY
            goto ret;
        }
        else
        {
            sanestat = sane_control_option(mScanDevice->scannerHandle(), mIndex,
                                           SANE_ACTION_SET_VALUE,
                                           mBuffer.data(),
                                           &sane_result);
        }
    }

    if (sanestat!=SANE_STATUS_GOOD)
    {
        kDebug() << "apply" << mName << "failed, SANE status" << sane_strstatus(sanestat);
        return (false);
    }

#ifdef DEBUG_APPLY
    debug += " applied";
#endif // DEBUG_APPLY

    if (sane_result & SANE_INFO_RELOAD_OPTIONS)
    {
#ifdef DEBUG_APPLY
        debug += " reload";
#endif // DEBUG_APPLY
        reload = true;
    }

#ifdef DEBUG_APPLY
    if (sane_result & SANE_INFO_INEXACT) debug += " inexact";
#endif // DEBUG_APPLY

    mApplied = true;
ret:
#ifdef DEBUG_APPLY
    kDebug() << debug.constData();			// no quotes, please
#endif // DEBUG_APPLY
    return (reload);
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


// Some heuristics are used here to determine whether the type of a numeric
// (SANE_TYPE_INT or SANE_TYPE_FIXED) option needs to be further refined.
// If the constraint is SANE_TYPE_RANGE and the unit is SANE_UNIT_DPI then
// it is assumed to be a resolution;  otherwise, if it is an array then
// it is assumed to be a gamma table.  The comment below suggests that
// this guess (for the gamma table) may not be reliable.
//
// TODO: May be better to hardwire the option names here, especially for the
// gamma table (and maybe also for resolution, it does give a false positive
// for the 'test' device option "int-constraint-array-constraint-range").
// ScanParams::slotApplyGamma() has the complete repertoire of gamma table
// names.

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
	    else if (mDesc->constraint_type==SANE_CONSTRAINT_WORD_LIST)
	    {
		ret = KScanOption::StringList;
	    }
	    else if(mDesc->constraint_type==SANE_CONSTRAINT_NONE)
	    {
                ret = KScanOption::SingleValue;
	    }
	    else
	    {
                ret = KScanOption::Invalid;
	    }
	    break;

case SANE_TYPE_STRING:
            if (QString::compare(mDesc->name, SANE_NAME_FILE)==0) ret = KScanOption::File;
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
    }

    mBufferClean = false;
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
    return (true);
}


bool KScanOption::set(const int *val, int size)
{
    if (!isValid() || mBuffer.isNull()) return (false);
    if (val==NULL) return (false);
#ifdef DEBUG_GETSET
    kDebug() << "Setting" << mName << "of size" << size;
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
    }

    int copybyte = mDesc->size;
    if (offset) copybyte += sizeof(SANE_Word);

    //kdDebug(29000) << "Copying " << copybyte << " byte to options buffer" << endl;
    mBuffer = QByteArray(((const char *) qa.data()),copybyte);
    mBufferClean = false;
    return (true);
}


bool KScanOption::set(const QByteArray &buf)
{
    if (!isValid() || mBuffer.isNull()) return (false);
#ifdef DEBUG_GETSET
    kDebug() << "Setting" << mName << "to" << buf;
#endif

    int val;
    int origSize;

    /* Check if it is a gammatable. If it is, convert to KGammaTable and call
     *  the appropriate set method.
     */
    QRegExp re( "\\d+,\\d+,\\d+" );
    re.setMinimal(true);
    QString s(buf);
    if (s.contains(re))
    {
        QStringList relist = s.split(",");

        int gamm = (relist[0]).toInt();
        int brig = (relist[1]).toInt();
        int contr = (relist[2]).toInt();

        KGammaTable gt( gamm, brig, contr);
        kDebug() << "Setting GammaTable with int vals" << gamm << brig << contr;
        return (set(&gt));
    }

    /* On String-type the buffer gets malloced in Constructor */
    switch (mDesc->type)
    {
case SANE_TYPE_STRING:
        origSize = mBuffer.size();
        mBuffer = QByteArray(buf.data(),(buf.length()+1));
        mBuffer.resize(origSize);			// restore original size
        break;

case SANE_TYPE_INT:
case SANE_TYPE_FIXED:
        bool ok;
        val = buf.toInt(&ok);
        if (ok) set(&val,1);
        else
        {
            kDebug() << "Conversion of string value" << buf << "failed!";
            return (false);
        }
        break;

case SANE_TYPE_BOOL:
        val = (buf=="true") ? 1 : 0;
        set(val);
        break;

default:
	kDebug() << "Can't set" << mName << "with this type";
        return (false);
    }

    mBufferClean = false;
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
    if (!isValid() || mBuffer.isNull()) return ("");

    QByteArray retstr;
    SANE_Word sane_word;

    /* Handle gamma-table correctly */
    if (type()==KScanOption::GammaTable)
    {
// TODO: centralise this formatting, and the reverse in set(const QByteArray &),
// in KGammaTable
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
            retstr = (const char *) mBuffer.data();
            break;

case SANE_TYPE_INT:
            sane_word = *((SANE_Word *) mBuffer.data());
            retstr.setNum(sane_word);
            break;

case SANE_TYPE_FIXED:
            sane_word = (SANE_Word) SANE_UNFIX(*((SANE_Word *) mBuffer.data()));
            retstr.setNum(sane_word);
            break;
	
default:    kDebug() << "Can't get" << mName << "as this type";
            retstr = "?";
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
    return (new KScanCombo(parent, text, list));
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
