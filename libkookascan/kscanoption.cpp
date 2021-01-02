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

#include <qwidget.h>
#include <qobject.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qregexp.h>

#include <klocalizedstring.h>
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
#include "libkookascan_logging.h"


//  Debugging options
#undef DEBUG_MEM
#undef DEBUG_GETSET
#define DEBUG_APPLY
#undef DEBUG_RELOAD


//  This defines the possible resolutions that will be shown by the combo.
//  Only resolutions from this list falling within the scanner's allowed range
//  will be included.
static const int resList[] = { 50, 75, 100, 150, 200, 300, 600, 900, 1200, 1800, 2400, 4800, 9600, 0 };


KScanOption::KScanOption(const QByteArray &name, KScanDevice *scandev)
{
    mScanDevice = scandev;

    if (!initOption(name))
    {
        qCWarning(LIBKOOKASCAN_LOG) << "initOption for" << name << "failed!";
        return;
    }

    if (!mIsReadable) return;				// no value to read
    if (mBuffer.isNull()) return;			// no buffer for value

    // read initial value from the scanner
    SANE_Status sanestat = sane_control_option(mScanDevice->scannerHandle(),
                                                mIndex,
                                                SANE_ACTION_GET_VALUE,
                                                mBuffer.data(), nullptr);
    if (sanestat==SANE_STATUS_GOOD) mBufferClean = false;
}


static bool shouldBePriorityOption(const QByteArray &name)
{
    return (name=="source");
}


bool KScanOption::initOption(const QByteArray &name)
{
    mDesc = nullptr;
    mControl = nullptr;
    mIsGroup = false;
    mIsReadable = true;
    mIsPriority = shouldBePriorityOption(name);
    mWidgetType = KScanOption::Invalid;

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
        qCWarning(LIBKOOKASCAN_LOG) << "no option descriptor for" << mName;
        return (false);
    }

    mDesc = sane_get_option_descriptor(mScanDevice->scannerHandle(), mIndex);
    if (mDesc==nullptr) return (false);

    mBuffer.resize(0);
    mBufferClean = true;
    mApplied = false;

    if (mDesc->type==SANE_TYPE_GROUP) mIsGroup = true;
    if (mIsGroup || mDesc->type==SANE_TYPE_BUTTON) mIsReadable = false;
    if (!(mDesc->cap & SANE_CAP_SOFT_DETECT)) mIsReadable = false;

    mGammaTable = nullptr;				// for recording gamma values

    mWidgetType = resolveWidgetType();			// work out the type of widget
    allocForDesc();					// allocate initial buffer
    return (true);
}


KScanOption::~KScanOption()
{
#ifdef DEBUG_MEM
    if (!mBuffer.isNull()) qCDebug(LIBKOOKASCAN_LOG) << "Freeing" << mBuffer.size() << "bytes for" << mName;
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


void KScanOption::updateList()
{
    KScanCombo *combo = qobject_cast<KScanCombo *>(mControl);
    if (combo==nullptr) return;

    QList<QByteArray> list = getList();
    combo->setList(list);
}


/* called on a widget change, if a widget was created.
 */
void KScanOption::redrawWidget()
{
    if (!isValid() || !isReadable() || mControl==nullptr || mBuffer.isNull()) return;

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
    if (mControl!=nullptr)
    {
        if (isGroup())
        {
            mControl->setEnabled(true);			// always enabled
            return;					// no more to do
        }

        if (!isActive())
        {
#ifdef DEBUG_RELOAD
            qCDebug(LIBKOOKASCAN_LOG) << "not active" << mName;
#endif
            mControl->setEnabled(false);
        }
        else if (!isSoftwareSettable())
        {
#ifdef DEBUG_RELOAD
            qCDebug(LIBKOOKASCAN_LOG) << "not settable" << mName;
#endif
            mControl->setEnabled(false);
        }
        else mControl->setEnabled(true);
    }

    if (!isReadable())
    {
#ifdef DEBUG_RELOAD
        qCDebug(LIBKOOKASCAN_LOG) << "not readable" << mName;
#endif
        return;
    }

    if (mBuffer.isNull())				// first get mem if needed
    {
        qCDebug(LIBKOOKASCAN_LOG) << "need to allocate now";
        allocForDesc();					// allocate the buffer now
    }

    if (!isActive()) return;

    if (mDesc->size>mBuffer.size())
    {
        qCWarning(LIBKOOKASCAN_LOG) << "buffer too small for" << mName << "type" << mDesc->type
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
    if (desc==nullptr) return;				// should never happen

    SANE_Status sanestat = sane_control_option(mScanDevice->scannerHandle(),
                                                mIndex,
                                                SANE_ACTION_GET_VALUE,
                                                mBuffer.data(), nullptr);
    if (sanestat!=SANE_STATUS_GOOD)
    {
        qCWarning(LIBKOOKASCAN_LOG) << "Can't get value for" << mName << "status" << sane_strstatus(sanestat);
        return;
    }

    updateList();					// if range changed, update GUI

#ifdef DEBUG_RELOAD
    qCDebug(LIBKOOKASCAN_LOG) << "reloaded" << mName;
#endif
    mBufferClean = false;
}


bool KScanOption::apply()
{
    int sane_result = 0;
    bool reload = false;
#ifdef DEBUG_APPLY
    QString debug = QString("option '%1'").arg(mName.constData());
#endif // DEBUG_APPLY

    SANE_Status sanestat = SANE_STATUS_GOOD;

    // See comment in reload() above
    const SANE_Option_Descriptor *desc = sane_get_option_descriptor(mScanDevice->scannerHandle(), mIndex);
    if (desc==nullptr) return (false);			// should never happen

    if (mName==SANE_NAME_PREVIEW || mName==SANE_NAME_SCAN_MODE)
    {
        sanestat = sane_control_option(mScanDevice->scannerHandle(), mIndex,
                                       SANE_ACTION_SET_AUTO, nullptr,
                                       &sane_result);
        /* No return here, please! Carsten, does it still work than for you? */
    }

    if (!isInitialised() || mBuffer.isNull())
    {
#ifdef DEBUG_APPLY
        debug += " nobuffer";
#endif // DEBUG_APPLY

        if (!isAutoSettable()) goto ret;

#ifdef DEBUG_APPLY
        debug += " auto";
#endif // DEBUG_APPLY
        sanestat = sane_control_option(mScanDevice->scannerHandle(), mIndex,
                                       SANE_ACTION_SET_AUTO, nullptr,
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
        qCWarning(LIBKOOKASCAN_LOG) << "apply" << mName << "failed, SANE status" << sane_strstatus(sanestat);
        return (false);
    }

#ifdef DEBUG_APPLY
    debug += QString(" -> '%1'").arg(get().constData());
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
    qCDebug(LIBKOOKASCAN_LOG) << qPrintable(debug);			// no quotes, please
#endif // DEBUG_APPLY
    return (reload);
}


// The name of the option is checked here to detect options which are
// a resolution or a gamma table, and therefore are to be treated
// specially.  This should hopefully be more reliable then the earlier
// heuristics.
KScanOption::WidgetType KScanOption::resolveWidgetType() const
{
    if (!isValid()) return (KScanOption::Invalid);

    KScanOption::WidgetType ret;
    switch (mDesc->type)
    {
case SANE_TYPE_BOOL:
        ret = KScanOption::Bool;
        break;

case SANE_TYPE_INT:
case SANE_TYPE_FIXED:
        if (qstrcmp(mDesc->name, SANE_NAME_SCAN_RESOLUTION)==0 ||
            qstrcmp(mDesc->name, SANE_NAME_SCAN_X_RESOLUTION)==0 ||
            qstrcmp(mDesc->name, SANE_NAME_SCAN_Y_RESOLUTION)==0)
        {
            ret = KScanOption::Resolution;
            if (mDesc->unit!=SANE_UNIT_DPI) qCWarning(LIBKOOKASCAN_LOG) << "expected" << mName << "unit" << mDesc->unit << "to be DPI";
        }
        else if (qstrcmp(mDesc->name, SANE_NAME_GAMMA_VECTOR)==0 ||
                 qstrcmp(mDesc->name, SANE_NAME_GAMMA_VECTOR_R)==0 ||
                 qstrcmp(mDesc->name, SANE_NAME_GAMMA_VECTOR_G)==0 ||
                 qstrcmp(mDesc->name, SANE_NAME_GAMMA_VECTOR_B)==0)
        {
            ret = KScanOption::GammaTable;
            if (mDesc->size!=sizeof(SANE_Byte)) qCDebug(LIBKOOKASCAN_LOG) << "expected" << mName << "size" << mDesc->size << "to be BYTE";
        }
        else if (mDesc->constraint_type==SANE_CONSTRAINT_RANGE) ret = KScanOption::Range;
        else if (mDesc->constraint_type==SANE_CONSTRAINT_WORD_LIST) ret = KScanOption::StringList;
        else if (mDesc->constraint_type==SANE_CONSTRAINT_NONE) ret = KScanOption::SingleValue;
        else ret = KScanOption::Invalid;
        break;

case SANE_TYPE_STRING:
        if (qstrcmp(mDesc->name, SANE_NAME_FILE)==0) ret = KScanOption::File;
        else if (mDesc->constraint_type==SANE_CONSTRAINT_STRING_LIST) ret = KScanOption::StringList;
        else ret = KScanOption::String;
        break;

case SANE_TYPE_BUTTON:
        ret = KScanOption::Button;
        break;

case SANE_TYPE_GROUP:
        ret = KScanOption::Group;
        break;

default:
        qCDebug(LIBKOOKASCAN_LOG) << "unsupported SANE type" << mDesc->type;
        ret = KScanOption::Invalid;
        break;
    }

#ifdef DEBUG_GETSET
    qCDebug(LIBKOOKASCAN_LOG) << "for SANE type" << mDesc->type << "returning" << ret;
#endif
    return (ret);
}


bool KScanOption::set(int val)
{
    if (!isValid() || mBuffer.isNull()) return (false);
#ifdef DEBUG_GETSET
    qCDebug(LIBKOOKASCAN_LOG) << "Setting" << mName << "to" << val;
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
        qCDebug(LIBKOOKASCAN_LOG) << "Can't set" << mName << "with type" << mDesc->type;
        return (false);
    }

    mBufferClean = false;
    return (true);
}


bool KScanOption::set(double val)
{
    if (!isValid() || mBuffer.isNull()) return (false);
#ifdef DEBUG_GETSET
    qCDebug(LIBKOOKASCAN_LOG) << "Setting" << mName << "to" << val;
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
        qCDebug(LIBKOOKASCAN_LOG) << "Can't set" << mName << "with type" << mDesc->type;
        return (false);
   }

    mBufferClean = false;
    return (true);
}


bool KScanOption::set(const int *val, int size)
{
    if (!isValid() || mBuffer.isNull()) return (false);
    if (val==nullptr) return (false);
#ifdef DEBUG_GETSET
    qCDebug(LIBKOOKASCAN_LOG) << "Setting" << mName << "of size" << size;
#endif

    int offset = 0;
    int word_size = mDesc->size/sizeof(SANE_Word);
    QVector<SANE_Word> qa(1+word_size);		/* add 1 in case offset is needed */

    switch (mDesc->type)
    {
case SANE_TYPE_FIXED:					// must have already used SANE_FIX()
case SANE_TYPE_INT:
        for (int i = 0; i<word_size; i++)
        {
            if (i<size) qa[offset+i] = (SANE_Word) *(val++);
            else qa[offset+i] = (SANE_Word) *val;
        }
        break;

default:
        qCDebug(LIBKOOKASCAN_LOG) << "Can't set" << mName << "with type" << mDesc->type;
        return (false);
    }

    int copybyte = mDesc->size;
    if (offset) copybyte += sizeof(SANE_Word);

    //qCDebug(LIBKOOKASCAN_LOG) << "Copying" << copybyte << "bytes to options buffer";
    mBuffer = QByteArray(((const char *) qa.data()), copybyte);
    mBufferClean = false;
    return (true);
}


bool KScanOption::set(const QByteArray &buf)
{
    if (!isValid() || mBuffer.isNull()) return (false);
#ifdef DEBUG_GETSET
    qCDebug(LIBKOOKASCAN_LOG) << "Setting" << mName << "to" << buf;
#endif

    int val;
    int origSize;
    bool ok;

    // Check whether the string value looks like a gamma table specification.
    // If it is, then convert it to a gamma table and set that.
    KGammaTable gt;
    if (gt.setFromString(buf))
    {
#ifdef DEBUG_GETSET
        qCDebug(LIBKOOKASCAN_LOG) << "is a gamma table";
#endif
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
        val = buf.toInt(&ok);
        if (ok) set(&val, 1);
        else return (false);
        break;

case SANE_TYPE_FIXED:
        val = SANE_FIX(buf.toDouble(&ok));
        if (ok) set(&val, 1);
        else return (false);
        break;

case SANE_TYPE_BOOL:
        set(buf=="true");
        break;

default:
        qCDebug(LIBKOOKASCAN_LOG) << "Can't set" << mName << "with type" << mDesc->type;
        return (false);
    }

    mBufferClean = false;
    return (true);
}


// The parameter here must be 'const'.
// Otherwise, a call of set() with a 'const KGammaTable *' argument appears
// to be silently resolved to a call of set(int) without warning.
bool KScanOption::set(const KGammaTable *gt)
{
    if (!isValid() || mBuffer.isNull()) return (false);

    // Remember the set values
    if (mGammaTable!=nullptr) delete mGammaTable;
    mGammaTable = new KGammaTable(*gt);

    int size = mDesc->size/sizeof(SANE_Word);		// size of scanner gamma table
#ifdef DEBUG_GETSET
    qCDebug(LIBKOOKASCAN_LOG) << "Setting gamma table for" << mName << "size" << size << "to" << gt->toString();
#endif
    const int *run = mGammaTable->getTable(size);	// get table of that size
    QVector<SANE_Word> qa(size);			// converted to SANE values

    switch (mDesc->type)
    {
case SANE_TYPE_INT:
        for (int i = 0; i<size; ++i) qa[i] = static_cast<SANE_Word>(run[i]);
        break;

case SANE_TYPE_FIXED:
        for (int i = 0; i<size; ++i) qa[i] = SANE_FIX(static_cast<double>(run[i]));
        break;

default:
        //qCDebug(LIBKOOKASCAN_LOG) << "Can't set" << mName << "with type" << mDesc->type;
        return (false);
    }

    mBuffer = QByteArray(((const char *) (qa.constData())), mDesc->size);
    mBufferClean = false;
    return (true);
}


bool KScanOption::get(int *val) const
{
    if (!isValid() || mBuffer.isNull()) return (false);

    switch (mDesc->type)
    {
case SANE_TYPE_BOOL:					/* Buffer has a SANE_Word */
        *val = (*((SANE_Word *) mBuffer.constData()))==SANE_TRUE ? 1 : 0;
        break;

case SANE_TYPE_INT:					/* reading just the first is OK */
        *val = *((SANE_Word *) mBuffer.constData());
        break;

case SANE_TYPE_FIXED:					/* reading just the first is OK */
        *val = static_cast<int>(SANE_UNFIX(*((SANE_Word *) mBuffer.constData())));
        break;

default:
        //qCDebug(LIBKOOKASCAN_LOG) << "Can't get" << mName << "as type" << mDesc->type;
        return (false);
    }

#ifdef DEBUG_GETSET
    qCDebug(LIBKOOKASCAN_LOG) << "Returning" << mName << "=" << *val;
#endif
    return (true);
}


QByteArray KScanOption::get() const
{
    if (!isValid() || mBuffer.isNull()) return ("");

    QByteArray retstr;

    /* Handle gamma-table correctly */
    if (mWidgetType==KScanOption::GammaTable)
    {
        if (mGammaTable!=nullptr) retstr = mGammaTable->toString().toLocal8Bit();
    }
    else
    {							// first SANE_Word of buffer
        SANE_Word sane_word = *((SANE_Word *) mBuffer.constData());
        switch (mDesc->type)
        {
case SANE_TYPE_BOOL:
            retstr = (sane_word==SANE_TRUE ? "true" : "false");
            break;

case SANE_TYPE_STRING:
            retstr = mBuffer;
            break;

case SANE_TYPE_INT:
            retstr.setNum(sane_word);
            break;

case SANE_TYPE_FIXED:
            retstr.setNum(SANE_UNFIX(sane_word), 'f');
            // Using 'f' format always has the specified number of digits (default 6)
            // after the decimal point.  We never want exponential format here so must
            // use 'f', but do not want trailing zeros or a decimal point.
            while (retstr.endsWith('0')) retstr.chop(1);
            if (retstr.endsWith('.')) retstr.chop(1);
            break;

default:    //qCDebug(LIBKOOKASCAN_LOG) << "Can't get" << mName << "as type" << mDesc->type;
            retstr = "?";
            break;
        }
    }

#ifdef DEBUG_GETSET
    qCDebug(LIBKOOKASCAN_LOG) << "Returning" << mName << "=" << retstr;
#endif
    return (retstr);
}


bool KScanOption::get(KGammaTable *gt) const
{
    if (mGammaTable==nullptr) return (false);		// has not been set

    gt->setAll(mGammaTable->getGamma(), mGammaTable->getBrightness(), mGammaTable->getContrast());
#ifdef DEBUG_GETSET
    qCDebug(LIBKOOKASCAN_LOG) << "Returning" << mName << "=" << gt->toString();
#endif
    return (true);
}


QList<QByteArray> KScanOption::getList() const
{
    const char **sstring = nullptr;
    QList<QByteArray> strList;
    if (mDesc==nullptr) return (strList);

    if (mDesc->constraint_type==SANE_CONSTRAINT_STRING_LIST)
    {
        sstring = (const char **)mDesc->constraint.string_list;

        while (*sstring!=nullptr)
        {
            strList.append(*sstring);
            sstring++;
        }
   }
   else if (mDesc->constraint_type==SANE_CONSTRAINT_WORD_LIST)
   {
       const SANE_Int *sint = mDesc->constraint.word_list;
       const int amount_vals = sint[0];
       QString s;

       for (int i = 1; i<=amount_vals; i++)
       {
           if (mDesc->type==SANE_TYPE_FIXED) s = QString::number(SANE_UNFIX(sint[i]), 'f');
           else s = QString::number(sint[i]);

           strList.append(s.toLocal8Bit());
       }
   }
   else if (mDesc->constraint_type==SANE_CONSTRAINT_RANGE && mWidgetType==KScanOption::Resolution)
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
           strList.append(QString::number(*ip).toLocal8Bit());
       }
   }

   return (strList);
}


bool KScanOption::getRange(double *minp, double *maxp, double *quantp) const
{
    if (mDesc==nullptr) return (false);

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
        qCDebug(LIBKOOKASCAN_LOG) << "Not a range type" << mDesc->name;
        return (false);
    }

    *minp = min;
    *maxp = max;
    if (quantp!=nullptr) *quantp = quant;

    return (true);
}


KScanControl *KScanOption::createWidget(QWidget *parent)
{
    if (!isValid())
    {
        qCWarning(LIBKOOKASCAN_LOG) << "Option is not valid";
	return (nullptr);
    }

    delete mControl; mControl = nullptr;		// dispose of the old control
 	
    if (mDesc!=nullptr) mText = i18n(mDesc->title);

    qCDebug(LIBKOOKASCAN_LOG) << "type" << mWidgetType << "name" << mName;

    KScanControl *w = nullptr;

    switch (mWidgetType)
    {
case KScanOption::Bool:					// toggle button
	w = new KScanCheckbox(parent, mText);
	break;

case KScanOption::SingleValue:				// numeric entry
        // This will have been deduced by the SANE parameter having a type
        // of SANE_TYPE_INT or SANE_TYPE_FIXED and a constraint of SANE_CONSTRAINT_NONE.
        // In theory a SANE_TYPE_FIXED value should allow floating point user input
        // and values, as noted for KScanOption::Range below.  No problem caused by
        // only allowing integer values has been reported with any existing scanner.
	w = new KScanNumberEntry(parent, mText);
	break;

case KScanOption::Range:				// slider and spinbox
	{
            KScanSlider *kss = new KScanSlider(parent, mText, true);
            w = kss;

            // This is the only option type that has an option to reset
            // the value to the default.  SANE does not specify what the
            // default value is, so it has to be guessed.  If the allowable
            // range includes the value 0 then the default is set to that,
            // otherwise the minimum value is used.
            //
            // Note that in theory the constrained range of a SANE value may
            // not have endpoints that are integers and it may not even include
            // any integer;  for example, the range could be 12.02 - 12.08
            // or even more precisely specified.  However, since the GUI
            // controls used (QSlider and QSpinBox) only work in integers then
            // the range is rounded to integer values.  No problem caused by
            // doing this has been reported with any existing scanner.

            double min, max, quant;
            getRange(&min, &max, &quant);

            int stdValue = 0;
            if (stdValue>max || stdValue<min) stdValue = qRound(min);
            kss->setRange(qRound(min), qRound(max), qRound(quant), stdValue);
        }
	break;

case KScanOption::Resolution:				// special resolution combo
case KScanOption::StringList:	 			// string list combo
	w = new KScanCombo(parent, mText);
	break;

case KScanOption::GammaTable:				// no widget for this
	qCDebug(LIBKOOKASCAN_LOG) << "GammaTable not implemented here";
	break;

case KScanOption::String:				// free text entry
	w = new KScanStringEntry(parent, mText);
	break;

case KScanOption::File:					// file name requester
	w = new KScanFileRequester(parent, mText);
        break;

case KScanOption::Group:				// group separator
	w = new KScanGroup(parent, mText);
        break;

case KScanOption::Button:				// action button
	w = new KScanPushButton(parent, mText);
        break;

default:
	qCWarning(LIBKOOKASCAN_LOG) << "unknown control type " << mWidgetType;
	break;
    }
 	
    if (w!=nullptr)
    {
        mControl = w;
        updateList();					// set list for combo box

        switch (w->type())
        {
case KScanControl::Number:				// numeric control
            connect(w, QOverload<int>::of(&KScanControl::settingChanged), this, QOverload<int>::of(&KScanOption::slotWidgetChange));
            break;

case KScanControl::Text:				// text control
            connect(w, QOverload<const QString &>::of(&KScanControl::settingChanged), this, QOverload<const QString &>::of(&KScanOption::slotWidgetChange));
            break;

case KScanControl::Button:				// push button
            connect(w, &KScanControl::returnPressed, this, QOverload<>::of(&KScanOption::slotWidgetChange));
            break;

case KScanControl::Group:				// group separator
            break;					// nothing to do here
        }

	if (mDesc!=nullptr)				// set tool tip
        {
            if (qstrlen(mDesc->desc)>0)			// if there is a SANE description
            {
                QString tt = i18n(mDesc->desc);
                // KDE tooltips do not normally end with a full stop, unless
                // they are multi-sentence.  But the SANE descriptions often do,
                // so trim it off for consistency.  Is this a good thing to do
                // in non-western languages?
                if (tt.endsWith('.') && tt.count(". ")==0) tt.chop(1);
                // Force the format to be rich text so that it will be word wrapped
                // at a sensible width, see documentation for QToolTip.
                w->setToolTip("<qt>"+tt);
            }
        }

        // No accelerators for advanced options, so as not to soak up
        // too many of the available accelerators for controls that are
        // rarely going to be used.  See also getLabel().
        if (!isCommonOption()) KAcceleratorManager::setNoAccel(w);
    }
 	
    reload();						// check if active, enabled etc.
    if (w!=nullptr) redrawWidget();
    return (w);
}


QLabel *KScanOption::getLabel(QWidget *parent, bool alwaysBuddy) const
{
    if (mControl==nullptr) return (nullptr);
    KSqueezedTextLabel *l = new KSqueezedTextLabel(mControl->label(), parent);
    if (isCommonOption() || alwaysBuddy) l->setBuddy(mControl->focusProxy());
    return (l);
}


QLabel *KScanOption::getUnit(QWidget *parent) const
{
    if (mControl==nullptr) return (nullptr);

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

    if (s.isEmpty()) return (nullptr);			// no unit label
    QLabel *l = new QLabel(s, parent);
    return (l);
}


void KScanOption::allocForDesc()
{
    if (mDesc==nullptr) return;

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
    qCDebug(LIBKOOKASCAN_LOG) << "Allocating" << size << "bytes for" << name;
#endif
    mBuffer.resize(size);				// set buffer size
    if (mBuffer.isNull())				// check allocation worked???
    {
        qCWarning(LIBKOOKASCAN_LOG) << "Allocating" << size << "bytes for" << mName << "failed!";
        return;
    }

    mBuffer.fill(0);					// clear allocated buffer
}
