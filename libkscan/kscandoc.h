/*
  A dummy source file for documenting the library.
  Copied from dcop
*/

/**
\mainpage Die KScan Bibliothek.

   Die kscan Bibliothek bietet ein Interface zur SANE-lib (siehe http://www.mostang.com
zur weiteren Information) für KDE2 Programme.

\section intro Einführung

Die KScan Bibliothek bietet jedem KDE2 Programm ein Objekt an, dass zu einem über
SANE angeschlossenen Scanner Verbindung aufbauen, dessen Parameter auslesen und
verwalten kann. Die Problematik dabei ist, dass SANE Scanner nicht über einen
einheitlichen Satz von Optionen verfügen. Die Scanner unterstützen verschiedene
Scanoptionen, und eine Oberfläche zum Ansprechen des Scanners muss dynamisch
aufgebaut werden, nachdem die Entscheidung gefallen ist, welches im System
vorhandene Gerät verwendet werden soll.

\section objectOverview Abstrakte KScan Objekte

Die KScan Bibliothek definiert folgende Klassen, die sich um die Verwaltung
der Parameter, die ein Scanner bietet, kümmern:

- KGammaTable\n
Dies ist eine Basisklasse, die einen Gammatable implementiert und dessen Berechnung
etc. intern durchführt.

- KScanOption\n
Das Objekt KScanOption implementiert genau eine Scanoption, wie z. B. die Scanauflösung.
Ein Scanner unterstützt verschiedene Optionen, die zum Teil abhängig voneinander sind.
Das bedeutet, dass sich im Falle einer Änderung von Option A die Option B mitändern kann.
Die KScanLib unterstützt die Behandlung dieser Abhängigkeiten.

- KScanDevice\n
Das Objekt KScanDevice bildet die vorhandenen Scanner im System ab. Über dieses Objekt
kann ermittelt werden, welche Geräte erreichbar sind und welche Optionen sie unterstützen.
\n
Wenn sich für ein Gerät entschieden und dieses geöffnet wurde, repräsentiert das KScanDevice
das Scangerät.

- KScanOptSet\n
Das Objekt KScanOptSet stellt einen Container für Optionen KScanOption dar, unabhängig
von einem bestimmten Gerät. Bisher ermöglicht es das Sichern von mehreren Optionen während der
Programausführung, z. B. die vorher eingestellten Scanparameter, wenn ein Previewscan
durchgeführt werden soll, um diese nach dem Previewscan wieder zu restaurieren.

\section helpers Hilfsklassen

Es existieren einige Hilfswidgets, die den dynamischen Aufbau der Scan-Oberfläche
vereinfachen. Diese Objekte bieten einfache Kombinationen von Grundwidgets, die einfacher
verwendet werden können.

Im einzelnen sind dies:

- KScanEntry\n(definiert in kscanslider.h)\n
bietet ein Eingabefeld mit vorangestelltem Text.

- KScanSlider \n(definiert in kscanslider.h)\n
Slider mit vorangestelltem Text. Bereichswerte werden mit Datentyp double übergeben.

- KScanCombo \n(definiert in kscanslider.h)\n
Combobox-Widget, das einen beschreibenden Text voranstellt und Icons darstellen kann. 

- DispGamma \n(definiert in dispgamma.h)\n
Widget zur Anzeige von Gammatables.

\section guiElements Oberflächenobjekte

Die KScanbibliothek bietet einige fertige Objekte, die in Programmen im Stile eines
vorgefertigten Dialoges verwendet werden können, um Scanfunktionalität in ein Programm
zu integrieren. Das führt dazu, dass die Scanfunktionalität in allen Programmen durch
die selbe Oberfläche angeboten wird.

Bis jetzt existieren folgende Oberflächenelemente:

- DeviceSelector\n
ist eine Klasse, die einen Dialog zur Auswahl eines Scanners darstellt.

- GammaDialog\n
ist ein Dialog, in dem ein Gammatable editiert werden kann. Der Gammadialog
verwendet KGammaTable intern zur Darstellung der Gammatables. Diesse Klasse
wird bei Beendigung des Dialoges zurückgeliefert, sodass die entsprechenden
Optionen des Scanners damit direkt gesetzt werden können.

- MassScanDialog\n
ist ein Massen-Scan-Dialog, der Benutzer über den Fortschritt des Scanvorganges
bei Benutzung des Automatischen Dokumenteinzuges informiert. \e very \e beta \e !

- ScanSourceDialog\n
einfacher Dialog, der die Auswahl der Scanquelle ermöglicht, z. B. Flachbett,
Durchlichteinheit etc.

- ScanParams\n
Die ScanParams-Klasse ist das eigentliche Herzstück der
 KScan-Bibliothek aus Sicht der Oberflächengestaltung.
 Die Klasse ScanParams stellt eine Ready-To-Use Oberfläche für den ausgewählten
 Scanner zur Verfügung.\n
\n
 In der Klasse wird das Scannerdevice analysiert und entsprechend
 seiner Eigenschaften dynamisch eine Oberfläche generiert, die die
 wichtigsten Bedienelemente enthält. Das sind bisher

 <ul>
    <li> Einstellungsmöglichkeiten für die Scanauflösung.
    <li> Scanquelle auswählen
    <li> Scanmodus
    <li> Helligkeit und Kontrasteinstellungen
    <li> Gammatable
    <li> Previewsannen
 </ul>
 


\author Klaas Freitag <freitag@suse.de>

*/

