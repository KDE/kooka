/*
  A dummy source file for documenting the library.
  Copied from dcop
*/

/**
\mainpage Die KScan Bibliothek.

   Die kscan Bibliothek bietet ein Interface zur SANE-lib (siehe http://www.mostang.com
zur weiteren Information) f�r KDE2 Programme.

\section intro Einf�hrung

Die KScan Bibliothek bietet jedem KDE2 Programm ein Objekt an, dass zu einem �ber
SANE angeschlossenen Scanner Verbindung aufbauen, dessen Parameter auslesen und
verwalten kann. Die Problematik dabei ist, dass SANE Scanner nicht �ber einen
einheitlichen Satz von Optionen verf�gen. Die Scanner unterst�tzen verschiedene
Scanoptionen, und eine Oberfl�che zum Ansprechen des Scanners muss dynamisch
aufgebaut werden, nachdem die Entscheidung gefallen ist, welches im System
vorhandene Ger�t verwendet werden soll.

\section objectOverview Abstrakte KScan Objekte

Die KScan Bibliothek definiert folgende Klassen, die sich um die Verwaltung
der Parameter, die ein Scanner bietet, k�mmern:

- KGammaTable\n
Dies ist eine Basisklasse, die einen Gammatable implementiert und dessen Berechnung
etc. intern durchf�hrt.

- KScanOption\n
Das Objekt KScanOption implementiert genau eine Scanoption, wie z. B. die Scanaufl�sung.
Ein Scanner unterst�tzt verschiedene Optionen, die zum Teil abh�ngig voneinander sind.
Das bedeutet, dass sich im Falle einer �nderung von Option A die Option B mit�ndern kann.
Die KScanLib unterst�tzt die Behandlung dieser Abh�ngigkeiten.

- KScanDevice\n
Das Objekt KScanDevice bildet die vorhandenen Scanner im System ab. �ber dieses Objekt
kann ermittelt werden, welche Ger�te erreichbar sind und welche Optionen sie unterst�tzen.
\n
Wenn sich f�r ein Ger�t entschieden und dieses ge�ffnet wurde, repr�sentiert das KScanDevice
das Scanger�t.

- KScanOptSet\n
Das Objekt KScanOptSet stellt einen Container f�r Optionen KScanOption dar, unabh�ngig
von einem bestimmten Ger�t. Bisher erm�glicht es das Sichern von mehreren Optionen w�hrend der
Programausf�hrung, z. B. die vorher eingestellten Scanparameter, wenn ein Previewscan
durchgef�hrt werden soll, um diese nach dem Previewscan wieder zu restaurieren.

\section helpers Hilfsklassen

Es existieren einige Hilfswidgets, die den dynamischen Aufbau der Scan-Oberfl�che
vereinfachen. Diese Objekte bieten einfache Kombinationen von Grundwidgets, die einfacher
verwendet werden k�nnen.

Im einzelnen sind dies:

- KScanEntry\n(definiert in kscanslider.h)\n
bietet ein Eingabefeld mit vorangestelltem Text.

- KScanSlider \n(definiert in kscanslider.h)\n
Slider mit vorangestelltem Text. Bereichswerte werden mit Datentyp double �bergeben.

- KScanCombo \n(definiert in kscanslider.h)\n
Combobox-Widget, das einen beschreibenden Text voranstellt und Icons darstellen kann. 

- DispGamma \n(definiert in dispgamma.h)\n
Widget zur Anzeige von Gammatables.

\section guiElements Oberfl�chenobjekte

Die KScanbibliothek bietet einige fertige Objekte, die in Programmen im Stile eines
vorgefertigten Dialoges verwendet werden k�nnen, um Scanfunktionalit�t in ein Programm
zu integrieren. Das f�hrt dazu, dass die Scanfunktionalit�t in allen Programmen durch
die selbe Oberfl�che angeboten wird.

Bis jetzt existieren folgende Oberfl�chenelemente:

- DeviceSelector\n
ist eine Klasse, die einen Dialog zur Auswahl eines Scanners darstellt.

- GammaDialog\n
ist ein Dialog, in dem ein Gammatable editiert werden kann. Der Gammadialog
verwendet KGammaTable intern zur Darstellung der Gammatables. Diesse Klasse
wird bei Beendigung des Dialoges zur�ckgeliefert, sodass die entsprechenden
Optionen des Scanners damit direkt gesetzt werden k�nnen.

- MassScanDialog\n
ist ein Massen-Scan-Dialog, der Benutzer �ber den Fortschritt des Scanvorganges
bei Benutzung des Automatischen Dokumenteinzuges informiert. \e very \e beta \e !

- ScanSourceDialog\n
einfacher Dialog, der die Auswahl der Scanquelle erm�glicht, z. B. Flachbett,
Durchlichteinheit etc.

- ScanParams\n
Die ScanParams-Klasse ist das eigentliche Herzst�ck der
 KScan-Bibliothek aus Sicht der Oberfl�chengestaltung.
 Die Klasse ScanParams stellt eine Ready-To-Use Oberfl�che f�r den ausgew�hlten
 Scanner zur Verf�gung.\n
\n
 In der Klasse wird das Scannerdevice analysiert und entsprechend
 seiner Eigenschaften dynamisch eine Oberfl�che generiert, die die
 wichtigsten Bedienelemente enth�lt. Das sind bisher

 <ul>
    <li> Einstellungsm�glichkeiten f�r die Scanaufl�sung.
    <li> Scanquelle ausw�hlen
    <li> Scanmodus
    <li> Helligkeit und Kontrasteinstellungen
    <li> Gammatable
    <li> Previewsannen
 </ul>
 


\author Klaas Freitag <freitag@suse.de>

*/

