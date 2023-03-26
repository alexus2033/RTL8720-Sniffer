English | [Deutsch](./README_DE.md) | [中文](./README_CN.md)

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

* `station [-v]` -> list all availabe wifi-stations in 2.4G/5G Spectrum

* `scan [from] [to] [-v] [-r]` -> look for MAC-Addresses on [specified channels](https://en.wikipedia.org/wiki/List_of_WLAN_channels)
  - Channel 1..13    for 2.4 GHz (802.11b/g/n/ax)
  - Channel 32..173  for 5 GHz (802.11a/h/j/n/ac/ax)
  - Repeat this command using `-r`
  - Display all found MAC-Addresses using `-v`
  
* `time [h] [m] [s]` -> set the internal clock

## What are MAC addresses used for?
So-called [MAC addresses](https://kb.wisc.edu/helpdesk/page.php?id=79258) are used for all wireless connections between transmitter and receiver.
Each device uses its own MAC and sends this along with the encrypted data packets.
The receiver checks the addresses of all packets and filters out all data intended for it from the radio signals.
After this, data are decrypted by the recipient and all other packets with unknown addresses are ignored.

## Tracking
Since the MAC addresses are readable by all nearby devices, someone could record them and track people's locations.
This is why all modern devices, such as smartphones, use [random MAC addresses](https://www.extremenetworks.com/extreme-networks-blog/wi-fi-mac-randomization-privacy-and-collateral-damage/ ) and change them at regular intervals.

## Vulnerability
When connecting the transmitter and receiver, the same MAC address is always used to enable identification.
If, for example, a smartphone connects to a public WLAN for the first time, the randomly generated MAC address is saved in addition to the name of the access point and the access data. If the user comes back later with the cell phone and connects, the device logs in with the stored MAC.
In this way, the user can be found and tracked accordingly without touching the encrypted communication between sender and recipient.

## Presence detection
RTL8720-Sniffer can use the vulnerability mentioned above to detect the presence of people in the household and e.g. switch on lights or heating automatically.
First, the channel on which the home WLAN access point is transmitting can be determined with the `station` command.
The addresses of all devices communicating on the channel can be found using the `scan` command.
Once you have found the address of your own mobile phone, you can save it with the `filter` command and trigger a switching process as soon as communication is registered.
In this case, the board LED lights up green.
