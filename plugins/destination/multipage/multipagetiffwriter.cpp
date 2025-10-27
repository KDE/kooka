/************************************************************************
 *									*
 *  This file is part of Kooka, a scanning/OCR application using	*
 *  Qt <http://www.qt.io> and KDE Frameworks <http://www.kde.org>.	*
 *									*
 *  Copyright (C) 2025      Jonathan Marten <jjm@keelhaul.me.uk>	*
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

#include "multipagetiffwriter.h"

#include <qtemporaryfile.h>
#include <qstandardpaths.h>
#include <qprocess.h>

#include <kmessagebox.h>
#include <klocalizedstring.h>

#include "scanimage.h"
#include "destination_logging.h"


#ifndef TIFFCP_COMMAND
#define TIFFCP_COMMAND		"tiffcp"
#endif


MultipageTiffWriter::MultipageTiffWriter()
    : AbstractMultipageWriter()
{
    qCDebug(DESTINATION_LOG);
}


void MultipageTiffWriter::setPageSize(const QPageSize &pageSize)
{
    // Ignored, size taken from each scanned image
}


void MultipageTiffWriter::setBaseImage(const QImage *img)
{
    if (img==nullptr) mBaseSize = QSize();		// unset the reference image
    else
    {
        mBaseSize = img->size();
        mBaseResX = DPM_TO_DPI(img->dotsPerMeterX());
        mBaseResY = DPM_TO_DPI(img->dotsPerMeterY());
        qCDebug(DESTINATION_LOG) << "size (pix)" << mBaseSize << "dpi X" << mBaseResX << "dpi Y" << mBaseResY;
    }
}


void MultipageTiffWriter::setOutputFile(const QString &fileName)
{
    mFileName = fileName;
}


bool MultipageTiffWriter::startPrint()
{
    if (mBaseSize.isNull()) return (false);		// no reference image

    if (mTiffcpCommand.isEmpty())			// command not yet found
    {
        mTiffcpCommand = QStandardPaths::findExecutable(TIFFCP_COMMAND);
        if (mTiffcpCommand.isEmpty())			// that didn't find the command
        {
            KMessageBox::error(nullptr, xi18nc("@info",
                                               "Cannot locate the <command section=\"1\">tiffcp</command> command,<nl/>"
                                               "ensure that it is available on <envvar>PATH</envvar>.<nl/><nl/>"
                                               "The command is part of the <application>libtiff</application> package,<nl/>"
                                               "but it may need to be installed separately."),
                               i18n("Cannot find TIFF command"));

            return (false);				// user can try again next time
        }

        qCDebug(DESTINATION_LOG) << "found command" << mTiffcpCommand;
    }

    // The 'mFileName' will be a QTemporaryFile which has been open()'ed
    // and then immediately close()'d by the caller.  So we can assume that
    // the file already exists, is writeable, and is currently zero size.
    // tiffcp(1) handles appending to an empty file correctly, so there is
    // nothing else to do to that file here.
    return (true);
}


bool MultipageTiffWriter::printImage(const QImage *img)
{
    qCDebug(DESTINATION_LOG) << "format" << img->format();

    // The QTemporaryFile created here is the current scanned page.
    // It is immediately appended to the 'mFileName' output file and
    // then discarded, so there is no need to keep the temporary file
    // any longer or to leave autoRemove() disabled.
    const QString ext = mFileName.mid(mFileName.lastIndexOf('.'));
    const QString dir = mFileName.left(mFileName.lastIndexOf('/'));
    QTemporaryFile tempPage(dir+"/pageXXXXXX"+ext);	// reuse the same template
    if (!tempPage.open())
    {
fail:   KMessageBox::error(nullptr,
                           xi18nc("@info", "Cannot save page to file <filename>%1</filename><nl/><message>%2</message>",
                                  tempPage.fileName(), tempPage.errorString()),
                           i18n("Cannot Save File"));
        return (false);
    }

    tempPage.close();					// only want the file name

    qDebug() << "saving TIFF to" << tempPage.fileName();
    if (!img->save(tempPage.fileName(), "TIFF")) goto fail;

    // No image format or conversion options are given to tiffcp(1),
    // so each page of the resulting file will have the same format as
    // the corresponding scanned page.
    const QStringList args = { "-a", tempPage.fileName(), mFileName };
    qDebug() << "executing command" << mTiffcpCommand << "args" << args;
    int s = QProcess::execute(mTiffcpCommand, args);
    if (s!=0)						// problem with tiffcp(1) command
    {
        KMessageBox::error(nullptr,
                           xi18nc("@info", "Cannot append page to file <filename>%1</filename>, status %2<nl/>(see standard error for any message)",
                                  mFileName, QString::number(s)),
                            i18n("Cannot Append Page"));
        return (false);
    }

    return (true);
}


void MultipageTiffWriter::endPrint()
{
}
