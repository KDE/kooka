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

#ifndef KSCANOPTSET_H
#define KSCANOPTSET_H

#include "kookascan_export.h"

#include <qhash.h>
#include <qmap.h>
#include <qbytearray.h>

class KConfigGroup;
class KScanOption;

/**
 * @short A set of scanner parameters.
 *
 * Named scanner parameters can be added to the set, which stores
 * their names and values.  They can be enumerated and retrieved from
 * the set.
 *
 * A set can be saved to and restored from the global scanner
 * configuration file.  This can be used to save scanner options
 * between runs of an application, or to manage a repertoire of
 * saved scanner configurations.
 *
 * The saved sets available can be listed, and a saved set can be
 * deleted from the configuration file.
 *
 *  @author Klaas Freitag
 *  @author Jonathan Marten
 **/

class KOOKASCAN_EXPORT KScanOptSet : public QHash<QByteArray, QByteArray>
{

public:
    /**
     * Create a new option set container.
     *
     * @param setName name for the option set.  When saving to or loading
     * from a configuration file, the set name specified here is used as
     * the group name.
     **/
    explicit KScanOptSet(const QString &setName);

    /**
     * Destructor.
     **/
    ~KScanOptSet() = default;

    /**
     * Category values for naming save set groups.
     **/
    enum Category
    {
        Params,						///< scanner parameters
        Options,					///< other scanner options
        Preset,						///< saved user preset
    };

    /**
     * Save the current value of an option.
     *
     * @param opt The option whose value is to be saved
     * @return @c true if the option was successfully stored
     */
    bool backupOption(const KScanOption *opt);

    /**
     * Save the option set to the global scanner configuration file.
     *
     * @param category The storage category to save under.
     * @param scannerName The SANE device name of the scanner to which
     * this configuration applies.
     * @return @c true if the save was successful.
     *
     * @note This does not automatically read the current options from the
     * scanner before saving them to the configuration file, the values last
     * read by backupOption() are used.  Therefore, to ensure the saved
     * option set correctly reflects the current scanner parameters, the
     * following should be done:
     *
     * @code
     * KScanOptSet optSet(setName);
     * saneDevice->getCurrentOptions(&optSet);
     * optSet.setDescription(setDesc);
     * optSet.saveConfig(KScanOptSet::Params, saneDevice->scannerBackendName());
     * @endcode
     **/
    bool saveConfig(KScanOptSet::Category category, const QByteArray &scannerName) const;

    /**
     * Load an option set from the global scanner configuration file.
     *
     * @param category The storage category to read from.
     * @param scannerName The SANE device name of the scanner to which
     * this configuration is intended to apply.  If it does not match the
     * scanner name that this option set was saved for, a warning message
     * is output (but the load will succeed, as far as is possible, anyway).
     * If this is a null or empty string, no check is made.
     * @return @c true if the load was successful.
     *
     * @note The option values read are not automatically sent to the scanner.
     * Therefore, to ensure that the scanner uses the loaded values, the
     * following should be done:
     *
     * @code
     * KScanOptSet optSet(setName);
     * optSet.loadConfig(KScanOptSet::Params);
     * saneDevice->loadOptionSet(&optSet);
     * saneDevice->reloadAll();
     * @endcode
     *
     * @note The option set is not cleared before it is loaded from the
     * configuration file, so any preexisting options which are not present
     * in the file will retain their previous values.  If a clean loaded
     * set is required for a previously-used option set, then simply use
     * @c clear() on it before calling @c loadConfig().
     **/
    bool loadConfig(KScanOptSet::Category category, const QByteArray &scannerName = QByteArray());

    /**
     * Set a description for the option set.
     *
     * @param desc The new description
     **/
    void setDescription(const QString &desc)	{ mSetDescription = desc; }

    /**
     * Get the description of the option set.
     *
     * @return The option set description
     **/
    QString getDescription() const		{ return (mSetDescription); }

    /**
     * Set a new name for the option set.
     *
     * @param newName The new option set name
     **/
    void setSetName(const QString &newName);

    /**
     * Get the name of the option set.
     *
     * @return The option set name
     **/
    const QString &getSetName() const		{ return (mSetName); }

    /**
     * List all of the saved option set names available in the
     * configuration file.
     *
     * @param category The storage category to list.  The default is
     * the saved scan presets.
     * @return a list of the set names
     **/
    static QStringList listSavedSets(KScanOptSet::Category category = KScanOptSet::Preset);

    /**
     * Delete a saved option set from the configuration file.
     *
     * @param setName The name of the set to delete
     * @param category The set's storage category.  The default is
     * the saved scan presets.
     **/
    static void deleteSet(const QString &setName, KScanOptSet::Category category = KScanOptSet::Preset);

    /**
     * Access a group in the global scanner configuration file.
     *
     * @param groupName The group name
     * @param category The storage category
     * @return the group
     */
    static KConfigGroup configGroup(const QString &groupName, KScanOptSet::Category category);

private:
    QString mSetName;
    QString mSetDescription;
};

#endif                          // KSCANOPTSET_H
