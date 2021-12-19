# RTL8720-Sniffer
Presence detection using [RTL8720DN](https://www.amebaiot.com/en/amebad-bw16-arduino-getting-started) 2.4G/5G Dual Bands Wifi Module

## Features
* no connection needed, no passwords used
* passive scan
* scan channels used by stations
* scan a range of channels only
* modify the `scanTimePerChannel`
* auto-repeat scans

## Used Libraries
* [Ameba Arduino SDK](https://github.com/ambiot/ambd_arduino)
* [SimpleCLI](https://github.com/SpacehuhnTech/SimpleCLI)
* [RTC](https://github.com/ambiot/ambd_arduino/blob/94b2bae9114552276e61581620aa5e3645e4de36/Arduino_package/hardware/libraries/RTC/examples/RTC/RTC.ino)

## Hardware
* [RTL8720DN](https://www.amebaiot.com/en/amebad-bw16-arduino-getting-started) (BW16)

## Usage
The following Commands are availabe via USB-Serial (115200 Baud):
* `filter [mac]` -> set [MAC-Address](https://kb.wisc.edu/helpdesk/page.php?id=79258) of the searched device 

* `station` -> list all availabe wifi-stations in 2.4G/5G Spectrum

* `scan [from] [to] [-v] [-r]` -> look for MAC-Addresses on [specified channels](https://en.wikipedia.org/wiki/List_of_WLAN_channels)
  - Channel 1..13    for 2.4 GHz (802.11b/g/n/ax)
  - Channel 32..173  for 5 GHz (802.11a/h/j/n/ac/ax)
  - Repeat this command using `-r`
  - Display all found MAC-Addresses using `-v`
  
* `time [h] [m] [s]` -> set the internal clock


