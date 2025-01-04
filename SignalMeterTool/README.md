# Signal Meter Tool
| <img src="../assets/RAK-Whirls.png" alt="RAKWireless"> |     <img src="./assets/01-main-screen.png" alt="Signal Meter" size=50% > | <img src="../assets/rakstar.jpg" alt="RAKstar" > |    
| :-: | :-: | :-: |

Signal Meter Tool is an application to setup a RAK10706 without the hassle of sending AT commands from a Serial Terminal Application.    
#### ⚠️ IMPORTANT ⚠️
#### _**This is not an official RAKwireless application!**_      
It was written in my free time and has no official support from RAKwireless.    
It is not fully tested and still has bugs.     
It is tested on Windows 11, on a Linux Ubuntu 24.04 based VM (automatic reconnection doesn't work in the VM) and should work on macOS ????????? (not tested by me).    
It has some experimental support to be used as Meshtastic Signal Tester, but this feature is work in progress.

# Installation 

## Windows
For Windows 11 an application installer is in the [SignalMeterTool/Windows](./Windows/) folder.     
Virus scanner will most likely complain about it, as it is not a signed Windows application.

## Linux
For Ubuntu 24.04 download the content of the [SignalMeterTool/Linux](./Linux) folder.     

#### ⚠️ IMPORTANT ⚠️
It might be required to add your user to the _**`dialout`**_ group to get access to the USB ports of the Linux Machine.    
Another requirement might be to give the user access to the folder where the application was saved.
```shell
sudo usermod -a -G dialout $USER

sudo chown -R $USER ./SignalMeterTool
```

## macOS
For macOS download the content of the [SignalMeterTool/macOS](./macOS) folder.     

#### ⚠️ IMPORTANT ⚠️
I have no machine with macOS and could not test the installation procedure or the application itself

----

# Usage
After installation, start the application. The UI is separated into 6 parts.    
1. Serial Ports, Connect/Disconnect button, Load Device Settings, Update Device Settings, option to restart the device and to see current device settings in the log output 
2. LoRa P2P and LoRaWAN settings
3. Test mode settings
4. Buttons to read or delete log files _**(only available if the device is equipped with an SD card)**_
5. Functions to setup the RTC and the timezone _**(only available if the device is equipped with a RTC module)**_    

In addition there are 3 parts for debugging.     

6. Log output of the communication with the device. It can be used as well to send manual AT commands.        
7. Switching the tool function from device setup mode to debug mode, where the device will start its test mode and the log output will show information.    

<center><img src="./assets/02-ui-parts.png" alt="UI"></center>

## Connect to the device

Check the available devices in the **Available Ports** list box. To find the correct port where the RAK10706 is connected, start the application first, then plug in the device into the USB port.    
The list box will be automatically updated with the new device, which will be the RAK10706.     

<center><img src="./assets/06-port-selection.png" alt="Port selection"></center>

Once you have selected the port, push **Connect** to establish a connection to the device.     
     
Once connected, Use the **Load Device Settings** button to read the current settings from the RAK10706. This can take a while and you will see a red **Busy** wheel next to the **Connect** button.     

#### ⚠️ Potential errors during connection ⚠️
1. No port or an invalid port was selected
<center><img src="./assets/12-error-no-port-selected.png" size=50% alt="Port error"></center>     

In this case, select a port from the list box.     

#### ⚠️ Potential errors while loading the device settings ⚠️

1. Device is not a RAK10706 device
<center><img src="./assets/14-error-not-rak17006.png" size=30% alt="Not a RAK10706"></center>     

This error will show when Load Device Settings is clicked, but the connected device is not a RAK10706.    This application does only work with the RAK10706 Signal Meter for LoRa!

2. A communication error occured    
<center><img src="./assets/13-error-not-rui3-device.png" size=30% alt="Communication error"></center>     

The device does not support RUI3 AT commands or a communication error occured during fetching the device settings. In this case, disconnect the device and restart the application. Make sure the device you are connecting to is a RAK10706 Signal Meter for LoRa.

----

## Load device settings

After connecting the device, the _**Load Device Settings**_ button will load the current settings from the device. 

<center><img src="./assets/17-load-settings.png" size=30% alt="Load device settings"></center>     

#### ⚠️ Occasionally the application can hang while reading the current settings ⚠️    
In this case, disconnect the device and restart the application.    
The reason for this bug is still under investigation.

## Application UI after successful connection

After a successful sync with the device, the UI will change, depending on the current settings of the device and the available modules.     

### LoRaWAN test modes
If the device is in a test mode that is using LoRaWAN (LinkCheck or Field Tester modes), the UI will show the LoRaWAN credentials and LoRaWAN settings.    
Independent of the Join Mode, the following settings can be set:

1. Region → Select the LoRaWAN region that device will be used with.
2. DR → Set the data rate that will be used for the testing.
3. TX Power → Set the TX Power that will be used for the testing.
4. ADR → Enable or disable the automatic data rate adaption.     
⚠️ _**During testing, it is recommended to disable ADR to get usable test results**_
5. JoinMode → Change the join mode between OTAA and ABP. _**The change will only be applied after the device settings are updated and the device rebooted!**_
6. Automatic Network Join → The device will automatically join the network after power-up.    
⚠️ _**It is essential to enable the automatic network join on the Signal Meter before using it**_

Depending on the LoRaWAN Join mode, the device credentials will be different:

----

#### LoRaWAN in join mode OTAA

In OTAA mode, the UI will show the DevEUI, AppEUI (JoinEUI) and the AppKey:

<center><img src="./assets/03-lorawan-otaa-settings.png" size=30% alt="LoRaWAN OTAA settings"></center>     

The credentials can be copied from the application and used to register the device in a LoRaWAN server. Or, if preferred, the credentials can be set to values that are provided by a LoRaWAN server.    

----

#### LoRaWAN in join mode ABP

In ABP mode, the UI will show the DevEUI, AppEUI (JoinEUI) and the AppKey:

<center><img src="./assets/04-lorawan-abp-settings.png" size=30% alt="LoRaWAN ABP settings"></center>     

The credentials can be copied from the application and used to register the device in a LoRaWAN server. Or, if preferred, the credentials can be set to values that are provided by a LoRaWAN server.    

----

### LoRa P2P test mode
If the device is in LoRa P2P test mode, the UI will show the LoRa P2P (Phy) settings. These settings must be identical to the LoRa P2P network that other LoRa nodes in the test are using:

<center><img src="./assets/05-lora-p2p-settings.png" size=30% alt="LoRaWAN P2P settings"></center>     

#### ⚠️ Information
The device setup supports only LoRa mode, it is not possible to setup the device to FSK mode. If this is required, it has to be done with AT commands.     

#### ⚠️ Experimental Meshtastic support
In the right side of the P2P test mode, settings for the Meshtastic network can be found.    
After selecting the Meshtastic Region, the Modem Preset and the Frequency Slot, the _**Apply Settings**_ button will set the P2P parameters for these Meshtastic settings.    

_The RAK10706 can only listen to Meshtastic data packets. It cannot decode the content. But the basic information received can show if the RAK10706 is in coverage range of the Meshtastic network and the signal quality of the received packets._    
It is _**NOT**_ possible to send packets to Meshtastic nodes.    
To test the range of a specific Meshtastic device, the Meshnode DUT ID can be setup, then the RAK10706 will only show information about received packets from this specific Meshtastic Node.    
<center><img src="./assets/18-meshtastic-output.png" size=30% alt="LoRaWAN P2P settings"></center>

#### ⚠️ Experimental Meshtastic support

----

### Send Interval, Test Mode and Location

This section of the UI allows to change the send interval, select the test mode and enable or disable the location tracking mode.    

<center><img src="./assets/07-test-settings.png" size=30% alt="Test settings"></center> 

----

#### Send Interval
The send interval defines the time between sending test packets. The value is set in seconds. _**Values below 10 seconds are not recommended for test modes that are using LoRaWAN. If set too short, the downlink packets from the LoRaWAN server might not be received.**_

----

#### Test Mode selection
Four different test modes are available on the RAK10706 Signal Meter. Three of them are working in LoRaWAN mode, one is for LoRa P2P.    
For differences between the test modes, check the [RAK10706 Quick Start Guide's](https://docs.rakwireless.com/product-categories/wisnode/rak10706/quickstart) and [differences between the RAK10706 Signal Meter and the RAK10701 Field Tester](https://docs.rakwireless.com/product-categories/wisnode/rak10706/quickstart#difference-between-the-rak10706-signal-meter-and-the-rak10701-field-tester) section.

#### ⚠️ Important
Changing the test mode, will immediately reboot the device and set it to the new selected test mode.

----

#### Location
Depending on the test scenario (indoor or outdoor) it is possible to enable or disable the location module of the Signal Meter. 
An overview of different test scenarios are in the [RAK10706 Quick Start Guide's](https://docs.rakwireless.com/product-categories/wisnode/rak10706/quickstart#typical-test-scenarios) _**Typical Test Scenarios**_ section.

#### ⚠️ Important
Testing with the location tracking disabled does increase the life-time of the battery and is recommended if not absolute required.

----

### Load and Update Device Settings

<center><img src="./assets/16-update-reload.png" size=30% alt="Log files folder"></center> 

----

#### Load

The _**Load Device Settings**_ button will load the current device settings and update the UI of the application.    

#### ⚠️ Important
Most changes in the settings are sent immediately to the device. Reloading the settings will not recover the original settings of the device.    

----

#### Update Device

Once device settings are changed, the changes can be synced to the Signal Meter using the _**Update Device**_ button.

#### ⚠️ Important
Most changes in the settings are sent immediately to the device. The exception are the LoRaWAN credentials for both OTAA and ABP mode.    

----

### Reading and Deleting Log Files

#### ⚠️ Important
These functions are only enabled if the RAK10706 SignalMeter is equipped with an SD card.

<center><img src="./assets/08-logs-controls.png" size=30% alt="Log file controls"></center> 

----

#### Reading Logs

#### ⚠️ Important
_**Downloading the log files over the USB connection can take a very long time, depending on the size and number of log files stored on the SD card. It is recommended to remove the SD card from the device instead and copy the log files directly on a computer**_

Using the _**Read Logs**_ button will start the transfer of all log files from the SD card to the computer over the USB connection.     
The log files are in .CSV (comma separated values) format and can be read for further processing from most spreadsheet applications.    
The log files are saved in the folder shown below the button in the _**Folder Path:**_ section. Each reading of the log files will create a new sub folder in this path with the date and time the readings are started.

Log files top folder:

<center><img src="./assets/14-log-files-folder.png" size=30% alt="Log files folder"></center>     

Log files:

<center><img src="./assets/15-log-files-sub-folder.png" size=30% alt="Log files"></center>     

#### ⚠️ Important
After reading the log files is finished, the device will be rebooting!

----

#### Delete Logs

Using the _**Delete Logs**_ button will erase all saved log files from the SD card. After using this button, the log files can not be recovered any more.

#### ⚠️ Important
After deleting the log files is finished, the device will be rebooting!

----

#### Device Restart and Device Status

The _**Device Restart**_ button will force a reboot of the device, if desired.    

The _** Device Status**_ will get the current device settings and display them in the log file. It does not change the settings in the UI!    

<center><img src="./assets/18-device-control.png" size=30% alt="Log files"></center> 

----

### Setting the RTC and the Time Zone

<center><img src="./assets/09-rtc-setup.png" size=30% alt="Log file controls"></center> 

----

#### Setting the Time Zone

The device will synchronize its internal time with the LoRaWAN server time it is connected to. However the LoRaWAN server is sending the time in GMT+0 format. The device time is used in the log files, therefor it is important to set the correct Time Zone in which the device is used. Otherwise the date and time entries in the log files will be in GMT+0 format.

#### ⚠️ Important
_**It is essential to set the time zone the device will be used in to have the correct date and time in the log files**_

----

#### Setting the RTC module

#### ⚠️ Important
This function is only enabled if the RAK10706 SignalMeter is equipped with an RTC module.

The RTC module in the RAK10706 Signal Meter has a small back-up battery that can ensure that the time is kept accurate if the device is switched of for 1-2 weeks. But if the Signal Meter is not used over a longer time, the RTC will loose its time. The _**Set**_ button allows to synchronize the RTC time with the time of the connected computer.

----

### Communication Debug

The log window on the right side will show the communication between the SignalMeterTool and the device. It is helpful only for debug purposes.    

In addition it has a input window, where AT commands can be sent to the device for additional changes, that are not supported by the tool.    

The _**Clear**_ button deletes all lines from the log window.    

Using the _**Mode**_ selector, the tool can be switched between _Setup_ and _Debug_ mode. While in _**Setup**_ mode, the device does not perform range testing. Switching to _**Debug**_ mode, enables the test sequence on the device and it will show some debug information in the log window.. This can be helpful to find problems with the SignalMeterTool application.    

<center><img src="./assets/10-communication-log.png" size=25% alt="Communication Logs"></center> 

----

### About the Application
This application was designed using [Xojo](https://xojo.com/), a cross-platform App Development Platform.
the project for this application can be found in the [SignalMeterTool/Project](./Project) folder.

This application is licensed under MIT.

<center><img src="./assets/mwx.png" size=25% alt="Xojo"></center> 

----
----

# LoRa® is a registered trademark or service mark of Semtech Corporation or its affiliates. 


# LoRaWAN® is a licensed mark.

----
----
