/* This file is part of the KDE Project
   Copyright (C) 2002 Klaas Freitag <freitag@suse.de>  

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

/**
\mainpage The KScan Library.

   The KScan Library provides an interface for the SANE-lib (see http://www.sane-project.org for more information) for KDE2 applications.

\section intro Introduction

The KScan Library furnishes each KDE2 application with an object which can connect to a scanner set up by SANE, as well as read out and manage the scanner's parameters. The difficulty with this is that SANE scanners do not have a uniform set of  options. The scanners support various scan options. An interface for establishing contact to the scanners has to be set up  dynamically after the decision is made as to which available device in the system should be used (dynamic GUI).

\section usage How to use libkscan

Libkscan provides a dialog for you to include scan functionality to your application. This includes on the main page of the dialog
   - The scanner setting parts with dynamically generated controls of scan parameters and the buttons to start scanning
   - A preview window to select the scan area
   - An option tab to edit basically scan options.
The scan dialog works completely as KPart and allows the user to scan a preview, select the interesting part on the preview scan, and than start the final scan. Your application will be notified by a signal finalImage that informs you that a new image is available from the scanner.





\section objectOverview Abstract KScan Objects

The KScan Library defines the following classes which are responsible for managing the scanner's parameters:

- KGammaTable\n
This is a base class which implements a gamma table and carries out the calculations etc. internally.

- KScanOption\n
The object KScanOption implements exactly one scanning option, such as the scanning resolution. A scanner supports several options which are, in part, independent of one another. This means that if option A is modified, option B could be modified along with it. The KScanLib supports the handling of these dependencies.

- KScanDevice\n
The object KScanDevice maps the scanners available in the system. This object assists the detection of reachable devices and the options they support.
\n
Once a device is decided upon and this has been opened, the KScanDevice will represent the scanning device. By way of this class,  the hardware will actually utilize the options (KScanOption). Furthermore, scanned image data is supplied by KScanDevice.

- KScanOptSet\n
The object KScanOptSet represents a container for the KScanOption options, apart from a specific device. Up until now, this was made possible by saving several options during the course of the program, e.g. previously configured scanning parameters if a  preview scan were to be carried out so that this can be restored following the preview scan.\n
Furthermore, option sets can be saved to disk.

\section helpers Helper Classes

There are some helper widgets which simplify the dynamic setup of the scanning interface. These objects provide simple combinations of base widgets to make their usage easier.

Itemized, these are:

- KScanEntry\n(defined in kscanslider.h)\n
Provides an entry field preceded by text.

- KScanSlider \n(defined in kscanslider.h)\n
Slider preceded by text. Range values are transferred with the data type double.

- KScanCombo \n(defined in kscanslider.h)\n
Combobox widget which precedes description text and can represent icons. 

- DispGamma \n(defined in dispgamma.h)\n
Widget for displaying gamma tables.

\section guiElements Interface Objects

The KScan Library offers some ready-made objects which can be used as a sort of pre-fabricated dialog in order to integrate the scanning functionality into an application. This results in the availability of scanning functionality in all applications over the same interface.

Currently, there are the following interface elements:

- DeviceSelector\n
is a class which represents a dialog for scanner selection.

- GammaDialog\n
is a dialog where a gamma table can be edited. To represent the gamma tables, the gamma dialog uses KGammaTable internally. This class is returned in such as way that it can set the relevant scanner options directly, once the dialog has been completed.

- MassScanDialog\n
is a mass scanning dialog which informs the user about how the scanning is progressing when the automatic document feeder is used. \e very \e beta \e !

- ScanSourceDialog\n
Small dialog which enables scanning source selection, e.g. Flatbed, automatic document feeder...

- ScanParams\n
The ScanParams class is the actual core of the
 KScan Library in terms of interface layout. The ScanParams class provides a ready-to-use interface for the selected scanner.\n
\n
 The scanner device is analyzed in this class and dynamically generates an interface, according to the device's properties, containing the most important operational elements. These are currently

 <ul>
    <li> Setting options for scanning resolution
    <li> Scanning source selection
    <li> Scanning mode
    <li> Brightness and contrast settings
    <li> Gamma table
    <li> Preview scanning
 </ul>
 


\author Klaas Freitag <freitag@suse.de>

*/

