[English](./README.md) | Deutsch | [中文](./README_CN.md)

# RTL8720-Sniffer
Anwesenheitserkennung mit [RTL8720DN](https://www.amebaiot.com/en/amebad-bw16-arduino-getting-started) 2.4G/5G Dual Band WLAN Module

## Features
* keine aktive Verbindung und keine Passwörter erforderlich
* passiver Scan
* scan für ausgewählte Kanäle
* scan für Kanäle, die von einem Accesspoint verwendet werden
* scan-dauer `scanTimePerChannel` einstellbar
* automatische Wiederholung von Scans

## Verwendete Arduiono-Bibliotheken
* [Ameba Arduino SDK](https://github.com/ambiot/ambd_arduino)
* [SimpleCLI](https://github.com/SpacehuhnTech/SimpleCLI)
* [RTC](https://github.com/ambiot/ambd_arduino/blob/94b2bae9114552276e61581620aa5e3645e4de36/Arduino_package/hardware/libraries/RTC/examples/RTC/RTC.ino)

## Hardware
* [RTL8720DN](https://www.amebaiot.com/en/amebad-bw16-arduino-getting-started) (BW16)

## Kommandozeile
Die folgenden Befehle können über USB-Serial (115200 Baud) ausgeführt werden:
* `filter [mac]` -> setze die [MAC-Addresse](https://kb.wisc.edu/helpdesk/page.php?id=79258) von dem gesuchten Gerät 

* `station [-v]` -> zeigt alle verfügbaren Accesspoints im 2.4G/5G Spektrum

* `scan [from] [to] [-v] [-r]` -> suche nach MAC-Addressen auf [festgelegten Kanälen](https://en.wikipedia.org/wiki/List_of_WLAN_channels)
  - Channel 1..13    für 2.4 GHz (802.11b/g/n/ax)
  - Channel 32..173  für 5 GHz (802.11a/h/j/n/ac/ax)
  - Wiederholung des Befehls mit `-r`
  - Anzeige aller gefundenen MAC-Addressen mit `-v`
  
* `time [h] [m] [s]` -> interne Uhr einstellen

## Wofür MAC-Adressen?
Für alle WLAN-Verbindungen zwischen Sender und Empfänger werden sogenannte [MAC-Adressen](https://www.elektronik-kompendium.de/sites/net/1406201.htm) verwendet.
Jedes Gerät verwendet eine eigene MAC und sendet diese zusammen mit den verschlüsselten Datenpaketen.
Der Empfänger prüft die Adressen aller Pakete und filtert alle für ihn bestimmten Daten aus den Funksignalen heraus.
Erst danach werden die Nutzdaten vom Empfänger entschlüsselt und alle anderen Pakete mt unbekannten Adressen werden ignoriert.

## Tracking
Da die MAC-Adressen von allen in der Nähe befindlichen Geräten lesbar ist, könnte jemand diese aufzeichnen, und den Aufenthalt von Personen nachvollziehen.
Aus diesem Grund verwenden alle modernen Geräte, wie z.B. Smartphones [zufällige MAC-Adressen](https://www.extremenetworks.com/extreme-networks-blog/wi-fi-mac-randomization-privacy-and-collateral-damage/) und ändern diese in regelmäßigen Abständen.

## Schwachstelle
Beim Verbinden zwischen Sender und Empfänger wird stets die gleiche MAC-Adresse verwendet, um eine Identifizierung zu ermöglichen.
Wenn sich z.B. ein Smartphone zum ersten Mal mit einem öffentlichen WLAN verbindet, wird neben dem Namen des Accesspoints und den Zugangsdaten auch die zu dem Zeitpunkt zufällig verwendete MAC-Adresse gespeichert. Wenn der Nutzer mit dem Handy später wiederkommt und sich verbindet, meldet sich das Gerät mit der gespeicherten MAC an.
Auf diese Weise lässt sich der Benutzer wiederfinden und entsprechend tracken, ohne die verschlüsselte Kommunikation zwischen Sender und Empfänger anzutasten.

## Anwesenheitserkennung
RTL8720-Sniffer kann die oben genannte Schwachstelle nutzen, um die Anwesenheit von Personen im Haushalt zu erkennen und z.B. Licht oder Heizung automatisch eischalten.
Zunächst lässt sich mit dem Befehl `station` der Kanal feststellen, auf dem der heimische WLAN-Accesspoint sendet. 
Über den Befehl `scan` können die Adressen aller Geräte ausfindig gemacht werden, welche auf dem Kanal der Station kommunizieren. 
Hat man die Adresse des eigenen Handys gefunden, kann man diese mit dem Befehl `filter` speichern und einen automatischen Schaltvorgang auslösen, sobald eine Kommunikation registriert wird. In diesem Fall leuchtet die Board-LED grün auf.
