SMS Door Bell
=============

Introduction
------------
The SMS Door Bell is a simple prototype that uses the Texas Instruments
CC3200 LaunchPad and the Telstra SMS API to send a SMS Message by pressing
a button

A video of the prototype being used can be found here - 

The SMS Door Bell demostrates the use of a small embedded device that utilises REST APIs 
over a secure TLS connection. Each button press performs the following steps

1. Obtains the current time from a SNTP server (this is required by TLS encyrption)
1. Obtain the OAuth 2.0 Authentication token from the Telstra API using HTTP GET over TLS
1. Send an SMS message using the Telstra SMS API using HTTP POST over TLS

These procedures are common in most REST API based solutions, and hence the code can be
repurposed to be used in many different applications.

All code is written in C and could be used as starting point for a real world mass produced 
hardware device that could run from a battery.

Prerequisites
------------

The following is required to implement this prototype.

* Telstra SMS API - https://dev.telstra.com/content/sms-api-0
* CC3200 Wi-Fi LaunchPad - http://www.ti.com/ww/en/launchpad/launchpads-connected-cc3200-launchxl.html#tabs
* Code Composure Studio v6.1 - http://processors.wiki.ti.com/index.php/Category:Code_Composer_Studio_v6
* UniFlash for the CC3200/CC3100 - http://processors.wiki.ti.com/index.php/CCS_UniFlash_-_CC3100/CC3200_Edition
* CC3200 SDK v1.1 - http://www.ti.com/tool/cc3200sdk

**Note 1:** That the Telstra SMS API will only work with Australian mobile phones numbers.

**Note 2:** You will need to update TI-RTOS for SimpleLink from within CCS using CCS App Center

Setup Steps
-----------

**Install Software**

You will need to have Code Composure Studio (CCS) v6 or later installed and 
TI-RTOS for SimpleLink will also need to be installed. This can be done via 
App Center, which can be accessed from within CCS via the View menu (CCS App
Center)

If you have difficulties, you can find additional details on setting up the 
project to work with TI-RTOS at this webpage: http://processors.wiki.ti.com/index.php/TI-RTOS_CC3200Wireless

You will also need to install CC3200 SDK, it is assumed that you have the 
CC3200 SDK installed in the default location on the c: drive. The version that 
is used is CC3200SDK_1.1.0. 

You will also need to install UniFlash. 

**Edit Source Code With Code Composure Studio**

Make the following changes to the Defines in the smsdoorbell.c file, so that
the application will use your credientials and mobile number.

* define POST_DATA   "{\"to\":\"0448922942\", \"body\":\"Knock, knock!\"}" //Change to your preferred mobile number and message
* define APP_KEY 	"yourapikey" // Your Telstra Consumer Key
* define APP_SECRET	"yourapisecret" // Your  Telstra Consumer Secret

**Install Root CA with UniFlash**

So that you can connect securely over TLS, the CC3200 will need to have the Root CA for the
Telstra API services flashed to its serial flash.

1. Use the certificate authority file "TelstraDevCert.cer" which is located in the Resources folder of th repository
1. Start UniFlash for the CC3200/CC3100
1. In Uniflash, click the "add file" option.
1. Name the file to /cert/tel.crt
1. In the Url field browser to the location of were you have stored TelstraDevCert.cer
1. Select the Erase and Update check boxes
1. Select the top of the tree (CC31x Flash Setup Control) and then press program.

For additional assistance see the UniFlash Quick Start Guide.

Note: You can download your own Certificate from a browser or from the Certificate 
Manager in Windows. Telstra APIs use QuoVadis Root CA 2.

**Update HTTP Web Client Library**

The HTTP Web Client is used to perform HTTP GET and POST requests, the current version that
comes with CC3200 SDK 1.1.0 needs to be updated with the ones found in the Resources folder
of this repository. All you need to do is copy over the webclient.a library found in the 
Resouces folder for HTTPClientFullLib and HTTPClientMinLib Folders with the ones located in 
C:\ti\CC3200SDK_1.1.0\cc3200-sdk\netapps\http\client\ccs

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

Additional Information
--------------------------------

As this is a starting point for a product, there are additional features which can
be easily implemented, for example the code is ready to receive data, for example 
a configurable phone number or message. This can be done by sending to UDP port 4000
by using a UDP tool or an app, the port number is configurable via the simplelinklibrary.h
file. 

If you would like to extend features such as being able to set SMS number and message, 
using an iOS APP then you could easily update the startproject_ios app - https://github.com/remixed123/startproject_ios

Potential Updates
-----------------

The current code is a starting point for a developer, there could be additional features added to make it more flexible, here are a few.

* Add multiple buttons for different people or different messages
* Develop a mobile app or website where you can configure the SMS phone number to be used or to change the message sent
* Add a LCD screen and have the ability for the device to display a message that was in response to the SMS
* Run from a battery and add power management to ensure battery lasts a long time
* Improve performance for apllication that requirer quicker messages



