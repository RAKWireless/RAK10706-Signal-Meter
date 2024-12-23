# RAK10706 Signal Meter
| <img src="./assets/RAK-Whirls.png" alt="RAKWireless"> |     <img src="./assets/rak10706-view-1.png" alt="Signal Meter" size=50% > | <img src="./assets/rakstar.jpg" alt="RAKstar" > |    
| :-: | :-: | :-: |

----

#### ⚠️ IMPORTANT ⚠️        
This firmware requires at least RUI3 V4.1.1 or newer.

#### ⚠️ ATTENTION ⚠️        
Requires the following Arduino libraries.

| Library                      | Get from Library Manager                       |
| ---                          | ---                                            |
| ArduinoJSON version 6.x.x    | http://librarymanager/All#ArduinoJson          |
| SD                           | http://librarymanager/All#SD                   |
| Melopero RV3028              | http://librarymanager/All#RV3028               |
| Sparkfun LIS3DH              | http://librarymanager/All#LIS3DH               |
| Sparkfun u-Blox GNSS library | http://librarymanager/All#SparkFun_u-blox_GNSS |
| nRF OLED display             | http://librarymanager/All#nRF52_OLED           |
| CayenneLPP                   | http://librarymanager/All#CayenneLPP           |

----

# Content
- [Overview](#overview)
- [Typical test scenarios](#typical-test-scenarios)
- [Custom AT commands](#custom-at-commands)
- [Hardware](#hardware)
- [Setup with built-in UI](#setup-with-built-in-ui)
- [Setup with AT commands](#setup-with-at-commands)
  - [LoRa P2P](#lora-p2p-setup)
  - [LoRaWAN LinkCheck](#lorawan-linkcheck-setup)
  - [LoRaWAN FieldTester (requires backend server)](#lorawan-field-tester-setup)
- [Usage](#usage)
  - [LoRa P2P](#lora-p2p)
  - [LoRaWAN LinkCheck](#lorawan-linkcheck)
  - [LoRaWAN FieldTester (requires backend server)](#lorawan-field-tester)
- [Log files (If SD card is present)](#log-files-if-sd-card-is-present)
  - [AT command for log files](#at-commands-for-log-files)
  - [Linkcheck mode log format](#linkcheck-mode-log-format)
  - [FieldTester mode log format](#field-tester-mode-log-format)
  - [P2P mode log format](#p2p-mode-log-format)
- [Enclosure](#enclosure)
- [Firmware](#firmware)

----

# Overview

The **RAK10706 Signal Meter for LoRa** is a basic signal meter. It works in both LoRa P2P and LoRaWAN mode. It uses an OLED display and a single-button controlled UI for settings changes. In addition, it can use an SD card to log the test results in CSV files. It is powered by a rechargeable battery and can be charged via USB Type-C interface.    
It uses an OLED display and a one-button controlled UI for settings changes. In addition, it can use an SD card to save the test results in .CSV files.   
All settings can be done as well over USB with RUI3 AT commands.    
It is a very easy to use device that can help to check LoRa and LoRaWAN coverage. It does not claim to be a super precise instrument, it is just an affordable small instrument to check signal coverage.    

<center><img src="./assets/rak10706-view-1.png" width="50%" alt="Device">&nbsp&nbsp&nbsp&nbsp<img src="./assets/rak10706-antenna.png" height="50%" width="35%" alt="Device internal"></center>

#### ⚠️ INFO ⚠️        
One of the advantages of this simple tester is that it does not require any backend installations on the LoRaWAN server (like Helium, TTN and Chirpstack) if used in LinkCheck Packet mode, but should work with any LoRaWAN server like AWS or Actility.     
Only the FieldTester Mode requires a backend server.    

## LoRa P2P mode
If used in LoRa P2P, it is listening on the selected LoRa P2P settings and sending packets on the same settings:
- Frequency
- Bandwidth
- Spreading Factor
- Coding Rate
- Preample Length  

If a packet is received on above settings, it will display the information on the OLED screen. To use it for a specific P2P scenario, it will require adaption in the code, like recognizing packets, send out specific test packets to other LoRa P2P nodes.

## LoRaWAN mode
If used in LoRaWAN mode, the device is connecting to a LoRaWAN server and sends out frequently a data packet. 
It requires setup of the devices with its LoRaWAN credentials and register on a LoRaWAN server with
- DevEUI
- AppEUI
- AppKey
- OTAA join mode
- LoRaWAN region

### LoRaWAN Linkcheck test mode     
It uses LinkCheck to collect information about the connection to the gateway(s).    
With LinkCheck, the LoRaWAN server will report the number of gateways and the demodulation margin (calculated on the LoRaWAN server). The demodulation margin can give you information about the received signal quality (The higher the margin, the better the signal quality).     
Extract from the _**LoRaWAN 1.0.3 Specification**_:
<center><img src="./assets/lorawan-linkcheck.png" alt="LinkCheck"></center>

### LoRaWAN FieldTester test mode    
In addition, it supports the RAK10701 FieldTester protocol. The advantage of the FieldTester protocol is that it provides more information about the test, including distances to the gateways. The disadvantage is that this protocol requires a backend server to process the information and send it back to the device.

# Typical test scenarios
In all scenarios, tests can be performed in two ways:
- automatic sending in a specified interval. The interval can be set either through the built-in UI or with an AT command.
- forced sending. 3 times pushing the button enforces sending out a single packet in the pre-defined settings.
- forced sending with DR sweep (only in LoRaWAN test modes). 4 times pushing the button enforces multiple packets to be sent out. Sending starts with the lowest possible data rate and increases the data rate with each packet until the highest possible data rate has been reached.    

## Outdoor testing
In this scenario the location tracking should be enabled to add the tester location to the test results.    
The log files will contain the location of the tester at the time the test was performed. If no location fix could be acquired, the location will be set to Lat 0, Long 0.    

## Indoor testing
In this scenario the location tracking should be disabled, as the GNSS chip cannot aqcuire a valid location inside of buildings.    
The log files will not contain the location of the tester.

## LinkCheck testing
If it is not possible to connect a backend server to the LoRaWAN server to process the data of the received packets, the LinkCheck method should be used. It can deliver the basic information of the connection quality and if the tester is in the coverage range of gateways.

## FieldTester mode
If a backend server is connected to the LoRaWAN server that can process the data of the received packets, the FieldTester mode gives more details in the results of the test. 

## LoRa P2P testing
This is a very basic test that only shows whether the device is in range of another LoRa P2P device that is sending out packets.    

# Custom AT commands     
This examples includes multiple custom AT commands:     
- **`ATC+SENDINT`** to set the send interval time or heart beat time. The device will send a payload with this interval. The time is set in seconds, e.g. **`AT+SENDINT=600`** sets the send interval to 600 seconds or 10 minutes.    
- **`ATC+MODE`** to set the test mode. 0 using LPWAN LinkCheck, 1 using LoRa P2P, 2 using FieldTester protocol.
- **`ATC+STATUS`** to get some status information from the device.    
- **`ATC+PCKG`** to setup a custom payload that is used in the uplink packets.
- **`ATC+LOGS`** to retrieve or erase saved log files from the SD card (if SD card is present). See [AT command for log files](#at-commands-for-log-files)
- **`ATC+RTC`** to set or get time of RTC. Set format = [yyyy:mm:dd:hh:MM] (discard leading zeros!)

[Back to top](#content)

----

# Hardware
The device is built with a custom WisBlock Base Board:
- [WisBlock Base Board RAK19026 (WisMesh Base Board)](https://store.rakwireless.com/products/wismesh-baseboard-rak19026) ([Datasheet](https://docs.rakwireless.com/Product-Categories/Meshtastic/WisMesh-Base/Overview/))

To extend lifetime of the device, the battery can be disconnected by a simple slider switch. This helps to avoid discharging the battery while the device is not in use. The device can be charged, even with the button in off position!

The RAK19026 Base Board features as well a user configurable button, in this case it is used to control the UI, enable/disable the display and reset the device.

[Back to top](#content)

----

# Setup with built-in UI

Most of the parameters (not all) can be changed with the built-in UI.     
The UI has several levels, the navigation between the levels and selection of items is done with the single user button of the device.    

## Generic function of the button if the Settings UI is not active:

### Single click
==> no function

### Double click
==> enter the Settings UI (stops the testing mode, no more test packets are sent and received packets are ignored)

### 3 clicks
==> Force a downlink packet to be sent

### 4 clicks
==> Force multiple downlink packets with DR sweep. Sending starts with the lowest possible data rate and increases the data rate with each packet until the highest possible data rate has been reached.

### 5 clicks
==> no function (to avoid accidental reset of device)

### 6 clicks
==> Reset the device

### 7 clicks
==> Enter Bootloader Mode

### Long Press
==> switch off / on the display for power savings

## Function of the button if the Settings UI is active

The button function in the Settings UI changes, depending on the settings level. In general, a single click goes up one level in the settings.     
For other items, the number in front of the item indicates the number of clicks required to activate the level.    
If a level has selectable items, the selected items is marked with _**`(X)`**_ instead of its number.    
If a level has an item that can be toggled on/off, the status is shown after the item name.    

Overview of all settings levels and button functions:

| Level | Sub Level 1 | Sub Level 2 | Comment |
| ----- | ---------- | ---------- |------- |
| Top level<br><img src="./assets/ui-top.png"> | | | Device might reset on leaving the settings if test mode has changed. |
| | Device Info <br><img src="./assets/ui-top-info.png"> | | Current test settings |
| | Device Settings <br><img src="./assets/ui-dev-setting-top.png"> | | General settings<br> Location and Display Saver are on/off toggle items<br><br>- Location on works only in FieldTester Mode and keeps the GNSS module powered up for faster location acquisition (faster battery drain)<br><br>- Display Saver on switches off the display after 1 minute. The display can be turned on with a single button click. |
| | | Send Interval <br><img src="./assets/ui-dev-setting-interval.png"> | Change send interval in 10 second steps<br>(2) 10 seconds more<br>(3) 10 seconds less |
| | Mode <br><img src="./assets/ui-mode-top.png"> | | Exclusive selection of one mode by number of clicks |
| | LoRa Settings (LoRaWAN test modes) <br><img src="./assets/ui-lorawan-top.png"> | | UI depends on selected test mode.<br> In LinkCheck, Confirmed Packet and FieldTester Mode, it shows LoRaWAN specific settings. |
| | | ADR on/off<br><img src="./assets/ui-lorawan-adr.png"> | Switch ADR on or off |
| | | DR selection<br><img src="./assets/ui-lorawan-dr.png"> | Change DR setting<br> (2) next higher DR<br>(3) next lower DR |
| | | TX Power selection<br><img src="./assets/ui-lorawan-tx.png"> | Change TX power setting<br> (2) next higher TX power<br>(3) next lower TX power |
| | | LoRaWAN region selection<br><img src="./assets/ui-lorawan-region.png"> | Change LoRaWAN region setting<br> (2) next region<br>(3) previous region |
| | LoRa Settings (LoRa P2P test mode) <br><img src="./assets/ui-lora-top.png"> | | UI depends on selected test mode.<br> In LoRa P2P Mode, it shows LoRa P2P specific settings. |
| | | Frequency change<br><img src="./assets/ui-lora-freq.png"> | Change send/receive frequency<br>(2) 0.1MHz up<br>(3) 0.1MHz down<br>For larger changes, it is recommended to use the AT commands. |
| | | SF change<br><img src="./assets/ui-lora-sf.png"> | Change Spreading Factor<br>(2) next SF<br>(3) previous SF<br>For larger changes, it is recommended to use the AT commands. |
| | | BW change<br><img src="./assets/ui-lora-bw.png"> | Change Bandwidth<br>(2) next BW<br>(3) previous BW |
| | | CR change<br><img src="./assets/ui-lora-cr.png"> | Change Coding Rate<br>(2) next CR<br>(3) previous CR |
| | | TX power change<br><img src="./assets/ui-lora-tx.png"> | Change Transmission Power<br>(2) next TX level <br>(3) previous TX level |


----

# Setup with AT commands

## LoRa P2P Setup

To use the device in LoRa P2P mode it has to be set into this mode with     
```at
AT+NWM=0
```
The device might reboot after this command, if it was not already in P2P mode.    
Then the LoRa P2P parameters have to be setup. In this example, I am setting the device to 916100000 Hz frequency, 125kHz bandwidth, spreading factor 7, coding rate 4/5, preamble length 8 and TX power of 5dBm:

```at
AT+PRECV=0
AT+P2P=916000000:7:0:1:8:5
ATC+MODE=2
```

#### ⚠️ TIP ⚠️ 
If the credentials were set already (they are saved in the flash of the device), the switch to P2P testing can as well be done with
```at
ATC+MODE=2
```
The device might reboot after this command, if it was not already in LoRa P2P mode.    


#### ⚠️ TIP ⚠️        
The command _**`AT+PRECV=0`**_ is _**required**_ to stop the device from listening. While in RX mode, parameters cannot be changed.

To be able to receive packets from other devices, they have to be setup to exactly the same parameters.

[Back to top](#content)

----

## LoRaWAN LinkCheck Setup

To use the device in LoRaWAN mode it has to be set into this mode with     
```at
AT+NWM=1
```
The device might reboot after this command, if it was not already in LoRaWAN mode.    
Then the LoRaWAN parameters and credentials have to be setup. In this example, I am setting the device to AS923-3, OTAA join mode, unconfirmed packet mode, enable link check and then reset the device to perform a LoRaWAN JOIN sequence:

```at
AT+BAND=10
AT+NJM=1
AT+CFM=0
AT+LINKCHECK=2
AT+DEVEUI=AC1F09FFFE000000
AT+APPEUI=AC1F09FFFE000000
AT+APPKEY=AC1F09FFFE000000AC1F09FFFE000000
ATC+MODE=0
ATZ
```

#### ⚠️ TIP ⚠️ 
If the credentials were set already (they are saved in the flash of the device), the switch to LinkCheck testing can as well be done with
```at
ATC+MODE=0
```
The device might reboot after this command, if it was not already in LoRaWAN mode.    

#### ⚠️ IMPORTANT ⚠️        
The device has to be registered in a LoRaWAN server with these credentials and a gateway in range has to be connected to the LoRaWAN server. Otherwise the device cannot join and there are no tests possible!
If the device cannot join the network, it will show an error on the display:

<center><img src="./assets/lpw-join-failed.png" alt="LoRaWAN Join Failed"></center>

In this case double check all settings on the device and LoRaWAN server and check if a gateway is in range and connected to the LoRaWAN server.

[Back to top](#content)

----

## LoRaWAN FieldTester Setup

To use the device in LoRaWAN mode it has to be set into this mode with     
```at
AT+NWM=1
```
The device might reboot after this command, if it was not already in LoRaWAN mode.    
Then the LoRaWAN parameters and credentials have to be setup. In this example, I am setting the device to AS923-3, OTAA join mode, confirmed packet mode, disable link check and then reset the device to perform a LoRaWAN JOIN sequence:

```at
AT+BAND=10
AT+NJM=1
AT+CFM=1
AT+LINKCHECK=0
AT+DEVEUI=AC1F09FFFE000000
AT+APPEUI=AC1F09FFFE000000
AT+APPKEY=AC1F09FFFE000000AC1F09FFFE000000
ATC+MODE=3
ATZ
```

#### ⚠️ TIP ⚠️ 
If the credentials were set already (they are saved in the flash of the device), the switch to FieldTester testing can as well be done with
```at
ATC+MODE=3
```
The device might reboot after this command, if it was not already in LoRaWAN mode.    

#### ⚠️ IMPORTANT ⚠️        
In FieldTester Mode a backend server has to be setup as integration in the LoRaWAN server. Without this backend server, the FieldTester Mode does not work.    
More information about available backend solutions can be found in the [RAK10701 documentation](https://docs.rakwireless.com/Product-Categories/WisNode/RAK10701-P/Quickstart/#lorawan-network-servers-guide-for-rak10701-p-field-tester-pro)

#### ⚠️ IMPORTANT ⚠️        
The device has to be registered in a LoRaWAN server with these credentials and a gateway in range has to be connected to the LoRaWAN server. Otherwise the device cannot join and there are no tests possible!
If the device cannot join the network, it will show an error on the display:

<center><img src="./assets/lpw-join-failed.png" alt="LoRaWAN Join Failed"></center>

In this case double check all settings on the device and LoRaWAN server and check if a gateway is in range and connected to the LoRaWAN server.

[Back to top](#content)

----

# Usage
The principle usage for all modes is similar. After selecting the mode and setting the correct parameters and credentials, the device will send uplink packets in the selected send interval.    

#### ⚠️ IMPORTANT ⚠️        
When using FieldTester Mode, the device requires to have a valid location fix from it's builtin GNSS module. Otherwise it will not send any uplink packets.    

## LoRa P2P

If the setup of all devices is the same and a packet is received, the display will show the received LoRa P2P packets:

- P2P received packet number
- Frequency, spreading factor and bandwidth
- RSSI
- SNR

<center><img src="./assets/lora-p2p-rx.png" alt="LoRa P2P"></center>

[Back to top](#content)

----

## LoRaWAN LinkCheck

After the device has joined the network, it will send unconfirmed packets with LinkCheck request enabled to the LoRaWAN server. The LoRaWAN server will answer to the LinkCheck request. The display will show
- Linkcheck result
- Packet number and number of gateways
- DR of the received packet
- RSSI and SNR of the received packet
- Demodulation Margin from the LoRaWAN server

<center><img src="./assets/lpw-linkcheck-ok.png" alt="LoRaWAN ACK"></center>

If the device is out of the range of gateways (after it had joined before), it will show an error message if the LoRaWAN server did respond to the LinkCheck request:
- Linkcheck result
- Number of lost packets

<center><img src="./assets/lpw-linkcheck-nok.png" alt="LoRaWAN ACK"></center>

[Back to top](#content)

----

## LoRaWAN FieldTester

After the device has joined the network, it will send confirmed packets with location information to the LoRaWAN server. The LoRaWAN server will forward this information together with gateway information to the backend server. The backend server will create and send a downlink packet to the tester. The display will show
- RSSI and SNR level of the received downlink packet
- Number of gateways that received the packet
- Min and Max RSSI levels seen by the gateways
- Min and Max calculated distance between the tester and the gateways
- Location of the device

<center><img src="./assets/fieldtester-ok.png" alt="Fieldtester display"></center>

Before sending a uplink packet, the tester will try to acquire a location.    

<center><img src="./assets/fieldtester-get-location.png" alt="Fieldtester location acquisition"></center>

If a location fix can be acquired, it will send an uplink packet, then wait for the downlink packet from the backend server:

If no location fix can be acquired, an error will be displayed and no packet will be sent:

<center><img src="./assets/fieldtester-no-location.png" alt="Fieldtester location failure"></center>

#### ⚠️ IMPORTANT ⚠️        
In FieldTester Mode a backend server has to be setup as integration in the LoRaWAN server. Without this backend server, the FieldTester Mode does not work.    
More information about available backend solutions can be found in the [RAK10701 documentation](https://docs.rakwireless.com/Product-Categories/WisNode/RAK10701-P/Quickstart/#lorawan-network-servers-guide-for-rak10701-p-field-tester-pro)

[Back to top](#content)

----

# Log files (If SD card is present)

If a SD card is present, the results of the coverage tests are written in CSV format to the SD card.    
The files start from 0000-log.csv and on every restart a new file with an upcounting number is created.    

## AT commands for log files

_**`ATC+LOGS=?`**_ is used to retrieve the log files over the USB port. This makes it possible to read the log files without removing the SD card from the device.    

_**`ATC+LOGS=e`**_ is used to erase all log files from the SD card.    

----

## Linkcheck mode log format

When in Linkcheck mode for LoRaWAN, the log file has the following format:    

### If location is enabled

time;Mode;Gw;Lat;Lng;RX RSSI;RX SNR;Demod;TX DR;Lost

### If location is disabled

time;Mode;Gw;RX RSSI;RX SNR;Demod;TX DR;Lost

| time | Mode | Gw | Lat | Lng | RX RSSI | RX SNR | Demod | TX DR | Lost |
| ---  | ---  | --- | --- | --- | ---    | ---    | ---   | ---  | ---  |
| Time stamp (available if LNS has provided the time or if a RTC module is attached) | 0 for LinkCheck mode | Number of gateways | Latitude (if location is active and location fix) | Longitude (if location is active and location fix) | RSSI of downlink | SNR of downlink | demod value | TX datarate | number of lost packets  |
| 2024-10-07 14:35:20 | 0 | 1 | 14.521355 | 121.106880 | -91 | 8 | 29 | 3 |0  |

----

## FieldTester mode log format

When in FieldTester mode for LoRaWAN, the log file has the following format:    

time;Mode;Gw;Lat;Lng;min RSSI;max RSSI;RX RSSI;RX SNR;min Dist;max Dist; TX DR

| time | Mode | Gw | Lat | Lng | min RSSI | max RSSI | RX RSSI | RX SNR | min Dist | max Dist | TX DR |
| ---  | ---  | --- | --- | --- | ---     | ---      | ---     | ---    | ---      | ---      | ---   |
| Time stamp (available if LNS has provided the time or if a RTC module is attached) | 2 for FieldTester mode | Number of gateways | Latitude (can be 0.0 if no location fix, e.g. indoor testing) | Longitude (can be 0.0 if no location fix, e.g. indoor testing) | min RSSI seen by gateways | max RSSI seen by gateways | RSSI of downlink | SNR of downlink | min distance to gateway(s) | max distance to gateway(s) | TX datarate |
| 2024-10-07 14:39:00 | 2 | 1 | 14.521355 | 121.106880 | -50 | -50 | -59 | 7 | 250 | 250      | 5 |

----

## P2P mode log format

When in Linkcheck mode for LoRaWAN, the log file has the following format:    

### If location is enabled

time;Mode;Lat;Lng;RX RSSI;RX SNR

### If location is disabled

time;Mode;RX RSSI;RX SNR

| time | Mode | Lat | Lng | RX RSSI | RX SNR |
| ---  | ---  | --- | --- | ---     | ---    |
| Time stamp (available if LNS has provided the time or if a RTC module is attached) | 1 for LinkCheck mode | Latitude (if location is active and location fix) | Longitude (if location is active and location fix) | RSSI of downlink | SNR of downlink |
| 2024-10-07 14:51:21 | 1 | 14.521355 | 121.106880 | -38 | 12    |

----

# Enclosure

The enclosure is 3D printed and kept as simple as possible. Two main parts are needed. The bottom and the lid are sliding into each other to give a basic dust protection and are secured with four screws.    
In addition three smaller parts are used to give a dust protection to the user button, power switch and reset button. The LED's and the OLED screen can be protected by adding a thin transparent plastic foil.    
For the USB port and SD card slot rubber lids are used.    

<center><img src="./assets/enclosure-topview.png" alt="Enclosure Top View"></center>

The OLED display is part of the Base Board.    

The top and bottom part of the enclosure are overlapping to provide a simple sealing.

<center><img src="./assets/enclosure-overlap.png" alt="Enclosure Sealing"></center>    

The button of the RAK19026 is protected with additional 3D parts (green) against dust entry.

<center><img src="./assets/enclosure-switch-button.png" alt="Enclosure Switch and Button"></center>    

The enclosure provides enough space for a 3200mAh Li-Ion battery glued to the bottom.

<center><img src="./assets/enclosure-battery.png" alt="Enclosure Screws"></center>

#### ⚠️ TIP ⚠️        
The 3D files for the enclosure are available in this repository in the folder [enclosure](./enclosure) 

[Back to top](#content)

----

# Firmware

The firmware for this Signal Meter is available in this repository.

Callbacks are defined for all possible events, both LoRa P2P and LoRaWAN and trigger the display to change its content.

The **`setup()`**` function is checking in which mode the device is setup and initializes the required event callbacks.

The application is complete timer triggered and the **`loop()`** function is only used when the user button is used to check the number of clicks or to detect a long press of the button.

## LoRa P2P callbacks

```cpp
/**
 * @brief Receive callback for LoRa P2P mode
 *
 * @param data structure with RX packet information
 */
void recv_cb_p2p(rui_lora_p2p_recv_t data)
{}
```

## LoRaWAN callback

```cpp
/**
 * @brief Join network callback
 * 
 * @param status status of join request
 */
void join_cb_lpw(int32_t status)
{}

/**
 * @brief Receive callback for LoRaWAN mode
 *
 * @param data structure with RX packet information
 */
void recv_cb_lpw(SERVICE_LORA_RECEIVE_T *data)
{}

/**
 * @brief Send finished callback for LoRaWAN mode
 *
 * @param status
 */
void send_cb_lpw(int32_t status)
{}

/**
 * @brief Linkcheck callback
 * 
 * @param data structure with the result of the Linkcheck
 */
void linkcheck_cb_lpw(SERVICE_LORA_LINKCHECK_T *data)
{}
```

## LoRaWAN send
This function sends a short LoRaWAN packet in confirmed or unconfirmed mode, depending whether LinkCheck is enabled or not
```cpp
/**
 * @brief Send a LoRaWAN packet
 *
 * @param data unused
 */
void send_packet(void *data)
{
	Serial.println("Send packet");
	uint8_t payload[4] = {0x01, 0x02, 0x03, 0x04};

	api.lorawan.send(4, payload, 2, false);
	tx_active = true;
	}
}
```

## Display handler
The display handler callback includes a flag that tells what kind of display content should be displayed.
```cpp
/**
 * @brief Display handler
 *
 * @param reason 1 = RX packet display
 *               2 = TX failed display (only LPW mode)
 *               3 = Join failed (only LPW mode)
 *               4 = Linkcheck result display (only LPW LinkCheck mode)
 *               5 = Join success (only LPW mode)
 *               6 = FieldTester downlink packet
 */
void handle_display(void *reason)
{}

```
[Back to top](#content)

----

----
----

# LoRa® is a registered trademark or service mark of Semtech Corporation or its affiliates. 


# LoRaWAN® is a licensed mark.

----
----