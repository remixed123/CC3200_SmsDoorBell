SMS Door Bell - Introduction
============================
The SMS Door Bell is a simple prototype that uses the Texas Instruments
CC3200 LaunchPad and the Telstra SMS API to send a SMS Message by pressing
a button

A video of the prototype being used can be found here - 

The SMS Door Bell shows how to use a small embedded device to use REST APIs 
over a secure TLS connection. Each button press performs the following steps
1. Obtains the current time from a SNTP server (this is required by TLS encyrption)
1. Obtain the OAuth 2.0 Authentication token using HTTP GET over TLS
1. Send an SMS message using the SMS API using HTTP POST over TLS

These procedures are common in most REST API based solutions, and hence the code can be
repurposed to be used in many different applications.

Prerequisites
------------

The following is required to implement this prototype.

* Telstra SMS API - https://dev.telstra.com/content/sms-api-0
* CC3200 Wi-Fi LaunchPad - http://www.ti.com/ww/en/launchpad/launchpads-connected-cc3200-launchxl.html#tabs
* Code Composure Studio v6.1 - http://processors.wiki.ti.com/index.php/Category:Code_Composer_Studio_v6
* UniFlash for the CC3200/CC3100 - http://processors.wiki.ti.com/index.php/CCS_UniFlash_-_CC3100/CC3200_Edition
* CC3200 SDK v1.1 - http://www.ti.com/tool/cc3200sdk

Note 1: That the Telstra SMS API will only work with Australian mobile phones numbers.
Note 2: You will need to update TI-RTOS for SimpleLink from within CCS using CCS App Center

Example Summary
---------------

The project controls the LEDs on the CC3200 LaunchPad depending on UDP Packet 
received. See details on how to use below.

The program loads the startproject Task statically. This can be changed via the
cfg script tab of the startproject.cfg

 

Setup Steps
-----------

You will need to have Code Composure Studio (CCS) v6 or later installed and 
TI-RTOS for SimpleLink will also need to be installed. This can be done via 
App Center, which can be accessed from within CCS via the View menu (CCS App
Center)

It is assumed that you have the CC3200 SDK installed in the default location on
the c: drive. The version that is used is CC3200SDK_1.1.0. if you are using a
later version of the SDK, then you will need to update references to the project.

If you have difficulties, you can find additional details on setting up the 
project to work with TI-RTOS at this webpage:
http://processors.wiki.ti.com/index.php/TI-RTOS_CC3200Wireless

Prototype Usage
---------------
When you run first run the example, you will need to provision the device with
a Wi-Fi Router. You can connect to the Wi-Fi network using either Station or 
Access Point modes. By default, it is set up to use SmartConfig in Station Mode 
(you will need to download the SimpleLink App from Apple App Store or Google 
Play). If you wish you can also set it up in Access Point mode, by holding down
Button (SW3) when starting the LaunchPad (or you can edit the code appropriately)

Once the device has successfully attached to a Wi-Fi Network, the Red LED will
turn on. You can then press Button (SW2) to send an SMS to the mobile number you 
configured in the source code.

Peripherals Exercised
---------------------
* Board_LED0 - CC3200_LP_LED_D7 - Red LED
* Board_BUTTON0 - CC3200_LP_SW2 - Button SW2
* Board_BUTTON1 - CC3200_LP_SW3 - Button SW3
* WIFI - Wi-Fi is used in this example

For an good introduction to the CC3200 LaunchPad - https://www.youtube.com/watch?v=KEERSpj3Gks

SimpleLink Library
-------------------

There is a library included (simplelinklibrary.c) which contains many useful functions 
that are required when developing applications with the CC3200. These include mDNS, 
device names, MAC address, date and time and more.

The startproject contains examples for most of the library functions, these can be removed
if not needed.

Additional Information
--------------------------------

As this is a starting point for a product, there are additional features which can
be easily implemented, for example the code is ready to receive data, for example 
a configurable phone number or message. This can be done by sending to UDP port 4000
by using a UDP tool or an app, the port number is configurable via the simplelinklibrary.h
file. 

If you would like to extend features such as being able to set SMS number and message, 
using an iOS APP then you could easily update the startproject_ios app - 
https://github.com/remixed123/startproject_ios


