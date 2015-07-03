//*****************************************************************************
//
// Application Name        - smsdoorbell
// Application Version     - 1.0.0
// Application Modify Date - 2nd of July 2015
// Application Developer   - Glenn Vassallo
// Application Contact	   - contact@swiftsoftware.com.au
// Application Repository  - https://github.com/remixed123/smsdoorbell
//
// Application Overview    - This example connects to a access point,
//                           it then allows the sending of an sms by
//                           pressing the SW2 button. It is using the
//                           Telstra APIs, and will only work with Australian
//                           mobile numbers. The code could be used as a
//                           starting point for any REST API application
//
// Application Details     - https://github.com/remixed123/smsdoorbell/readme.md
//
// Further Details         - If you would like to chat about your next CC3200 project
//                           then feel free contact us at contact@swiftsoftware.com.au
//
//*****************************************************************************

/* Standard Header files */
//#include <math.h>
//#include <string.h>
//#include <stdio.h>
//#include <stdint.h>
#include "smsdoorbell.h"

#include <stdbool.h>
//#include <stdlib.h>
//#include <time.h>

/* StartProject Library Headers */
#include "simplelinklibrary.h"

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/Memory.h>
#include <xdc/runtime/System.h>

/* SYS/BIOS Headers */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
//#include <ti/sysbios/knl/Clock.h>
//#include <ti/sysbios/knl/Swi.h>
//#include <ti/sysbios/gates/GateHwi.h>
//#include <ti/sysbios/knl/Mailbox.h>
//#include <ti/sysbios/knl/Semaphore.h>

/* Peripheral Headers */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/WiFi.h>
//#include <ti/drivers/SPI.h>
//#include "driverlib/ssi.h"
//#include "driverlib/sysctl.h"

/* SimpleLink Wi-Fi Host Driver Header files */
#include <simplelink.h>
#include <netapp.h>
#include <netcfg.h>
#include <osi.h>
#include <fs.h>
#include <socket.h>
//#include <protocol.h>

/* common interface includes */
//#include "timer_if.h"
//#include "gpio_if.h"
//#include "button_if.h"
//#include "uart_if.h"
//#include "common.h"
//#include "pinmux.h"

// HTTP Client lib
#include <http/client/httpcli.h>
#include <http/client/ssock.h>
#include <http/client/common.h>

// JSON Parser
#include "jsmn.h"

#include "Board.h"

//****************************************************************************
//                          LOCAL DEFINES
//****************************************************************************
//#define OSI_STACK_SIZE                   (2048)
#define TASK_PRIORITY       (1)
#define SSID_LEN_MAX        32
#define BSSID_LEN_MAX       6
#define SUCCESS             0

//*****************************************************************************
//                         APPLICATION DEFINES
//*****************************************************************************

#define APPLICATION_VERSION "1.0.0"
#define APP_NAME            "Telstra SMS Doorbell"

//*****************************************************************************
//                            HTTP DEFINES
//*****************************************************************************
#define POST_REQUEST_URI 	"/v1/sms/messages"
#define POST_DATA           "{\"to\":\"0448913198\", \"body\":\"Knock, knock!\"}"

#define APP_KEY 			"yourapikey" // YourTelstra Consumer Key
#define APP_SECRET			"yourapisecret" // Your  Telstra Consumer Secret

#define HOST_NAME       	"api.telstra.com"
#define HOST_PORT           443

#define PROXY_IP       	    <proxy_ip>
#define PROXY_PORT          <proxy_port>

#define READ_SIZE           1450
#define MAX_BUFF_SIZE       1460

//*****************************************************************************
//                       SNTP CONFIGURATION DEFINES
//*****************************************************************************

#define TIME2013                3565987200u      /* 113 years + 28 days(leap) */
#define YEAR2013                2013
#define SEC_IN_MIN              60
#define SEC_IN_HOUR             3600
#define SEC_IN_DAY              86400

#define SERVER_RESPONSE_TIMEOUT 10
#define GMT_DIFF_TIME_HRS       10
#define GMT_DIFF_TIME_MINS      0

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
volatile unsigned long  g_ulStatus = 0;//SimpleLink Status
unsigned long  g_ulDestinationIP; // IP address of destination server
unsigned long  g_ulGatewayIP = 0; //Network Gateway IP address
unsigned char  g_ucConnectionSSID[SSID_LEN_MAX+1]; //Connection SSID
unsigned char  g_ucConnectionBSSID[BSSID_LEN_MAX]; //Connection BSSID
unsigned char g_buff[MAX_BUFF_SIZE+1];
long bytesReceived = 0; // variable to store the file size
int g_iSockID;
unsigned long g_ulElapsedSec;
short g_isGeneralVar;
unsigned long g_ulGeneralVar;
unsigned long g_ulGeneralVar1;
char g_acTimeStore[30];
char *g_pcCCPtr;
unsigned short g_uisCCLen;

SlSockAddr_t sAddr;
SlSockAddrIn_t sLocalAddr;

const char g_acSNTPserver[25] = "tic.ntp.telstra.net"; // Telstra SNTP Server

// Queue Structure
typedef struct
{
    //Queue_Elem _elem;
    P_OSI_SPAWN_ENTRY pEntry;
    void* pValue;
}tPushButtonMsg;

static unsigned char accessToken[28];
static unsigned char expiresIn[8];

int buttonPressed = 0;

//*****************************************************************************
//                  Globals for Date and Time
//*****************************************************************************
SlDateTime_t dateTime =  {0};

// Tuesday is the 1st day in 2013 - the relative year
const char g_acDaysOfWeek2013[7][3] = {{"Tue"},{"Wed"},{"Thu"},{"Fri"},{"Sat"},{"Sun"},{"Mon"}};
const char g_acMonthOfYear[12][3] = {{"Jan"},{"Feb"},{"Mar"},{"Apr"},{"May"},{"Jun"},{"Jul"},{"Aug"},{"Sep"},{"Oct"},{"Nov"},{"Dec"}};
const char g_acNumOfDaysPerMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const char g_acDigits[] = "0123456789";

//*****************************************************************************
// Globals used by sockets and simplelink
//*****************************************************************************
uint8_t buffer[UDPPACKETSIZE];
bool deviceConnected = false;
bool ipAcquired = false;
uint32_t currButton;
uint32_t prevButton;

char returnPacket[128];

typedef struct
{
    unsigned long  ipV4;
    unsigned long  ipV4Mask;
    unsigned long  ipV4Gateway;
    unsigned long  ipV4DnsServer;
}_NetCfgIpV4Args_t;


/* variable to be read by GUI Composer */
int count = 0;

//*****************************************************************************
//                         UTILITIES
//*****************************************************************************

//*****************************************************************************
//
//! itoa
//!
//!    @brief  Convert integer to ASCII in decimal base
//!
//!     @param  cNum is input integer number to convert
//!     @param  cString is output string
//!
//!     @return number of ASCII parameters
//!
//!
//
//*****************************************************************************
unsigned short itoa(short cNum, char *cString)
{
    char* ptr;
    short uTemp = cNum;
    unsigned short length;

    // value 0 is a special case
    if (cNum == 0)
    {
        length = 1;
        *cString = '0';

        return length;
    }

    // Find out the length of the number, in decimal base
    length = 0;
    while (uTemp > 0)
    {
        uTemp /= 10;
        length++;
    }

    // Do the actual formatting, right to left
    uTemp = cNum;
    ptr = cString + length;
    while (uTemp > 0)
    {
        --ptr;
        *ptr = g_acDigits[uTemp % 10];
        uTemp /= 10;
    }

    return length;
}

//*****************************************************************************
//
//! getHostIP
//!
//! \brief  This function obtains the server IP address using a DNS lookup
//!
//! \param[in]  pcHostName        The server hostname
//! \param[out] pDestinationIP    This parameter is filled with host IP address.
//!
//! \return On success, +ve value is returned. On error, -ve value is returned
//!
//
//*****************************************************************************
long getHostIP(char* pcHostName, unsigned long * pDestinationIP)
{
    long lStatus = 0;

    lStatus = sl_NetAppDnsGetHostByName((signed char *) pcHostName,
                                            strlen(pcHostName),
                                            pDestinationIP, SL_AF_INET);
//    ASSERT_ON_ERROR(lStatus);
//
    System_printf("\nGet Host IP succeeded.\n\rHost: %s IP: %d.%d.%d.%d \n",
                    pcHostName, SL_IPV4_BYTE(*pDestinationIP,3),
                    SL_IPV4_BYTE(*pDestinationIP,2),
                    SL_IPV4_BYTE(*pDestinationIP,1),
                    SL_IPV4_BYTE(*pDestinationIP,0));
    System_flush();

    return lStatus;
}

//*****************************************************************************
//
//! Gets the current time from the selected SNTP server
//!
//! \brief  This function obtains the NTP time from the server.
//!
//! \param  GmtDiffHr is the GMT Time Zone difference in hours
//! \param  GmtDiffMins is the GMT Time Zone difference in minutes
//!
//! \return 0 : success, -ve : failure
//!
//*****************************************************************************
long GetSNTPTime(unsigned char ucGmtDiffHr, unsigned char ucGmtDiffMins)
{

/*
                            NTP Packet Header:

       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9  0  1
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |LI | VN  |Mode |    Stratum    |     Poll      |   Precision    |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                          Root  Delay                           |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                       Root  Dispersion                         |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                     Reference Identifier                       |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                                |
      |                    Reference Timestamp (64)                    |
      |                                                                |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                                |
      |                    Originate Timestamp (64)                    |
      |                                                                |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                                |
      |                     Receive Timestamp (64)                     |
      |                                                                |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                                |
      |                     Transmit Timestamp (64)                    |
      |                                                                |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                 Key Identifier (optional) (32)                 |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                                |
      |                                                                |
      |                 Message Digest (optional) (128)                |
      |                                                                |
      |                                                                |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
    char cDataBuf[48];
    long lRetVal = 0;
    int iAddrSize;

    // Send a query to the NTP server to get the NTP time
    memset(cDataBuf, 0, sizeof(cDataBuf));
    cDataBuf[0] = '\x1b';

    sAddr.sa_family = AF_INET;
    // the source port
    sAddr.sa_data[0] = 0x00;
    sAddr.sa_data[1] = 0x7B;    // UDP port number for NTP is 123
    sAddr.sa_data[2] = (char)((g_ulDestinationIP>>24)&0xff);
    sAddr.sa_data[3] = (char)((g_ulDestinationIP>>16)&0xff);
    sAddr.sa_data[4] = (char)((g_ulDestinationIP>>8)&0xff);
    sAddr.sa_data[5] = (char)(g_ulDestinationIP&0xff);

    lRetVal = sl_SendTo(g_iSockID, cDataBuf, sizeof(cDataBuf), 0, &sAddr, sizeof(sAddr));
    if (lRetVal != sizeof(cDataBuf))
    {
    	System_printf("Could not send request to SNTP server: %i\n",lRetVal);
    	System_flush();
        return lRetVal;
    }

    // Wait to receive the NTP time from the server
    sLocalAddr.sin_family = SL_AF_INET;
    sLocalAddr.sin_port = 0;
    sLocalAddr.sin_addr.s_addr = 0;
    if(g_ulElapsedSec == 0)
    {
        lRetVal = sl_Bind(g_iSockID, (SlSockAddr_t *)&sLocalAddr, sizeof(SlSockAddrIn_t));
        if(lRetVal < 0)
        {
        	System_printf("Could not bind to SNTP server: %i\n",lRetVal);
        	System_flush();
            return lRetVal;
        }
    }

    iAddrSize = sizeof(SlSockAddrIn_t);

    lRetVal = sl_RecvFrom(g_iSockID, cDataBuf, sizeof(cDataBuf), 0, (SlSockAddr_t *)&sLocalAddr, (SlSocklen_t*)&iAddrSize);
    if(lRetVal < 0)
    {
    	System_printf("Did not receive valid response from SNTP server: %i\n",lRetVal);
    	System_flush();
        return lRetVal;
    }

    // Confirm that the MODE is 4 --> server
    if ((cDataBuf[0] & 0x7) != 4)    // expect only server response
    {
        //ASSERT_ON_ERROR(SERVER_GET_TIME_FAILED);  // MODE is not server, abort
    	return -1;
    }
    else
    {
        unsigned char iIndex;

        // Getting the data from the Transmit Timestamp (seconds) field
        // This is the time at which the reply departed the
        // server for the client
        g_ulElapsedSec = cDataBuf[40];
        g_ulElapsedSec <<= 8;
        g_ulElapsedSec += cDataBuf[41];
        g_ulElapsedSec <<= 8;
        g_ulElapsedSec += cDataBuf[42];
        g_ulElapsedSec <<= 8;
        g_ulElapsedSec += cDataBuf[43];

        // seconds are relative to 0h on 1 January 1900
        g_ulElapsedSec -= TIME2013;

        // in order to correct the timezone
        g_ulElapsedSec += (ucGmtDiffHr * SEC_IN_HOUR);
        g_ulElapsedSec += (ucGmtDiffMins * SEC_IN_MIN);

        g_pcCCPtr = &g_acTimeStore[0];

        // day, number of days since beginning of 2013
        g_isGeneralVar = g_ulElapsedSec/SEC_IN_DAY;
        memcpy(g_pcCCPtr, g_acDaysOfWeek2013[g_isGeneralVar%7], 3);
        g_pcCCPtr += 3;
        *g_pcCCPtr++ = '\x20';

        // month
        g_isGeneralVar %= 365;
        for (iIndex = 0; iIndex < 12; iIndex++)
        {
            g_isGeneralVar -= g_acNumOfDaysPerMonth[iIndex];
            if (g_isGeneralVar < 0)
                    break;
        }
        if(iIndex == 12)
        {
            iIndex = 0;
        }
        memcpy(g_pcCCPtr, g_acMonthOfYear[iIndex], 3);
        g_pcCCPtr += 3;
        *g_pcCCPtr++ = '\x20';

        // Set the Month Value
        dateTime.sl_tm_mon = iIndex + 1;

        // date
        // restore the day in current month
        g_isGeneralVar += g_acNumOfDaysPerMonth[iIndex];
        g_uisCCLen = itoa(g_isGeneralVar + 1, g_pcCCPtr);
        g_pcCCPtr += g_uisCCLen;
        *g_pcCCPtr++ = '\x20';

        // Set the Date
        dateTime.sl_tm_day = g_isGeneralVar + 1;

        // time
        g_ulGeneralVar = g_ulElapsedSec%SEC_IN_DAY;

        // number of seconds per hour
        g_ulGeneralVar1 = g_ulGeneralVar%SEC_IN_HOUR;

        // number of hours
        g_ulGeneralVar /= SEC_IN_HOUR;
        g_uisCCLen = itoa(g_ulGeneralVar, g_pcCCPtr);
        g_pcCCPtr += g_uisCCLen;
        *g_pcCCPtr++ = ':';

        // Set the hour
        dateTime.sl_tm_hour = g_ulGeneralVar;

        // number of minutes per hour
        g_ulGeneralVar = g_ulGeneralVar1/SEC_IN_MIN;

        // Set the minutes
        dateTime.sl_tm_min = g_ulGeneralVar;

        // number of seconds per minute
        g_ulGeneralVar1 %= SEC_IN_MIN;
        g_uisCCLen = itoa(g_ulGeneralVar, g_pcCCPtr);
        g_pcCCPtr += g_uisCCLen;
        *g_pcCCPtr++ = ':';
        g_uisCCLen = itoa(g_ulGeneralVar1, g_pcCCPtr);
        g_pcCCPtr += g_uisCCLen;
        *g_pcCCPtr++ = '\x20';

        //Set the seconds
        dateTime.sl_tm_sec = g_ulGeneralVar1;

        // year
        // number of days since beginning of 2013
        g_ulGeneralVar = g_ulElapsedSec/SEC_IN_DAY;
        g_ulGeneralVar /= 365;
        g_uisCCLen = itoa(YEAR2013 + g_ulGeneralVar, g_pcCCPtr);
        g_pcCCPtr += g_uisCCLen;

        *g_pcCCPtr++ = '\0';

        //Set the year
        dateTime.sl_tm_year = 2013 + g_ulGeneralVar;

        System_printf("Response from server: ");
        System_printf((char *)g_acSNTPserver);
        System_printf("\n\r");
        System_printf(g_acTimeStore);
        System_printf("\n\r");
        System_flush();

        //Set time of the device for certificate verification.
        lRetVal = setDeviceTimeDate();
        if(lRetVal < 0)
        {
        	System_printf("Unable to set time in the device. Error Number: %i\n",lRetVal);
        	System_flush();
            return lRetVal;
        }
    }

    return SUCCESS;
}

//*****************************************************************************
//
//! This function obtains the current time from a SNTP server if required due
//! to not having current time (when booting up) or periodically to update
//! the time
//!
//! \param None
//!
//! \return  0 on success else error code
//! \return  Error Number of failure
//
//*****************************************************************************
long GetCurrentTime()
{
    int iSocketDesc;
    long lRetVal = -1;

	// Create UDP socket
	iSocketDesc = sl_Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(iSocketDesc < 0)
	{
		System_printf("Could not create UDP socket. Error Number: %i\n",iSocketDesc);
		System_flush();
		close(iSocketDesc);
		return iSocketDesc;
	}
	g_iSockID = iSocketDesc;

	// Get the NTP server host IP address using the DNS lookup
	lRetVal = getHostIP((char*)g_acSNTPserver, &g_ulDestinationIP);
	if( lRetVal >= 0)
	{
		// Configure the recieve timeout
		struct SlTimeval_t timeVal;
		timeVal.tv_sec =  SERVER_RESPONSE_TIMEOUT;    // Seconds
		timeVal.tv_usec = 0;     // Microseconds. 10000 microseconds resolution
		lRetVal = sl_SetSockOpt(g_iSockID,SL_SOL_SOCKET,SL_SO_RCVTIMEO, (unsigned char*)&timeVal, sizeof(timeVal));
		if(lRetVal < 0)
		{
			System_printf("Could not configure socket option (receive timeout). Error Number: %i\n",lRetVal);
			System_flush();
			close(iSocketDesc);
			return lRetVal;
		}
	}
	else
	{
		System_printf("DNS lookup failed. Error Number: %i\n\r",lRetVal);
		System_flush();
		close(iSocketDesc);
		return lRetVal;
	}

	// Get current time from the SNTP server
	System_printf("Fetching Time From SNTP Server\n");
	System_flush();
	lRetVal = GetSNTPTime(GMT_DIFF_TIME_HRS, GMT_DIFF_TIME_MINS);
	if(lRetVal < 0)
	{
		System_printf("Server Get Time failed. Error Number: %i\n",lRetVal);
		System_flush();
		close(iSocketDesc);
		return lRetVal;
	}
	else
	{
		System_printf("Server Get Time Successful - RTC Updated\n\n");
		System_flush();
	}

	// Close the socket
	close(iSocketDesc);

	return SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// SimpleLink Functions
///////////////////////////////////////////////////////////////////////////////

//*****************************************************************************
//! SimpleLinkHttpServerCallback
//!
//! \brief This function handles callback for the HTTP server events
//!
//! \param[in]     	pServerEvent - Contains the relevant event information
//! \param[in]      pServerResponse - Should be filled by the user with the
//!					relevant response information
//!
//! \return 		None
//*****************************************************************************
void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pEvent, SlHttpServerResponse_t *pResponse)
{

    switch (pEvent->Event)
    {
        case SL_NETAPP_HTTPGETTOKENVALUE:
        {

        }
        break;

        case SL_NETAPP_HTTPPOSTTOKENVALUE:
        {

        }
        break;

        default:
        break;
    }
}

//****************************************************************************
//! HTTPServerTask
//!
//!	\brief OOB Application Main Task - Initializes SimpleLink Driver and
//!                                              Handles HTTP Requests
//! \param[in]              	pvParameters is the data passed to the Task
//!
//! \return	                	None
//
//****************************************************************************
//static void HTTPServerTask(void *pvParameters)
//{
//    //memset(g_ucSSID,'\0',AP_SSID_LEN_MAX);
//
//    //Read Device Mode Configuration
//    //ReadDeviceConfiguration();
//
//    //Connect to Network
//    //ConnectToNetwork();
//
//    //Handle Async Events
//    while(1)
//    {
//
//    }
//}

//*****************************************************************************
//! SimpleLinkSockEventHandler
//!
//! This function handles socket events indication
//!
//! \param[in]      pSock - Pointer to Socket Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
    //
    // This application doesn't work w/ socket - Events are not expected
    //
       switch( pSock->Event )
    {
        case SL_SOCKET_TX_FAILED_EVENT:
        	switch( pSock->socketAsyncEvent.SockTxFailData.status )
            {
                case SL_ECLOSE:
                    System_printf("[SOCK ERROR] - close socket (%d) operation "
                    "failed to transmit all queued packets\n",
                           pSock->socketAsyncEvent.SockAsyncData.sd);
                    System_flush();
                    break;
                default:
                    System_printf("[SOCK ERROR] - TX FAILED : socket %d , reason"
                        "(%d) \n\r",
                        pSock->socketAsyncEvent.SockAsyncData.sd,
                        pSock->socketAsyncEvent.SockTxFailData.status);
                    System_flush();
            }
            break;

        default:
            System_printf("[SOCK EVENT] - Unexpected Event [%x0x]\n",pSock->Event);
            System_flush();
    }
}

///*****************************************************************************
//! SimpleLinkWlanEventHandler
//!
//! SimpleLink Host Driver callback for handling WLAN connection or
//! disconnection events.
//******************************************************************************
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pArgs)
{
    switch (pArgs->Event) {
        case SL_WLAN_CONNECT_EVENT:
            deviceConnected = true;
            break;

        case SL_WLAN_DISCONNECT_EVENT:
            deviceConnected = false;
            break;

        default:
            break;
    }
}

///*****************************************************************************
//! SimpleLinkNetAppEventHandler
//!
//! SimpleLink Host Driver callback for asynchoronous IP address events.
//******************************************************************************
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pArgs)
{
    switch (pArgs->Event) {
        case SL_NETAPP_IPV4_ACQUIRED:
            ipAcquired = true;
            break;

        default:
            break;
    }
}

///*****************************************************************************
//! smartConfigFxn
//!
//! Starts the Smart Config process which allows the user to tell the CC3100
//! which AP to connect to, using a smart phone app. Downloads available here:
//! http://www.ti.com/tool/smartconfig
//******************************************************************************
void smartConfigFxn()
{
  uint8_t policyVal;

  /* Set auto connect policy */
  sl_WlanPolicySet(SL_POLICY_CONNECTION,
                   SL_CONNECTION_POLICY(1, NULL, NULL, NULL, NULL),
                   &policyVal,
                   sizeof(policyVal));

  /* Start SmartConfig using unsecured method. */
  sl_WlanSmartConfigStart(NULL, SMART_CONFIG_CIPHER_NONE, NULL, NULL,
                          NULL, NULL, NULL, NULL);
}

//*****************************************************************************
//
//! \brief Flush response body.
//!
//! \param[in]  httpClient - Pointer to HTTP Client instance
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
static int FlushHTTPResponse(HTTPCli_Handle httpClient)
{
    const char *ids[2] = {
                            HTTPCli_FIELD_NAME_CONNECTION, /* App will get connection header value. all others will skip by lib */
                            NULL
                         };
    char buf[128];
    int id;
    int len = 1;
    bool moreFlag = 0;
    char ** prevRespFilelds = NULL;


    /* Store previosly store array if any */
    prevRespFilelds = HTTPCli_setResponseFields(httpClient, ids);

    /* Read response headers */
    while ((id = HTTPCli_getResponseField(httpClient, buf, sizeof(buf), &moreFlag))
            != HTTPCli_FIELD_ID_END)
    {

        if(id == 0)
        {
            if(!strncmp(buf, "close", sizeof("close")))
            {
                System_printf("Connection terminated by server\n");
                System_flush();
            }
        }

    }

    /* Restore previosuly store array if any */
    HTTPCli_setResponseFields(httpClient, (const char **)prevRespFilelds);

    while(1)
    {
        /* Read response data/body */
        /* Note:
                moreFlag will be set to 1 by HTTPCli_readResponseBody() call, if more
                data is available Or in other words content length > length of buffer.
                The remaining data will be read in subsequent call to HTTPCli_readResponseBody().
                Please refer HTTP Client Libary API documenation @ref HTTPCli_readResponseBody
                for more information.
        */
        HTTPCli_readResponseBody(httpClient, buf, sizeof(buf) - 1, &moreFlag);
        //ASSERT_ON_ERROR(len);

        if ((len - 2) >= 0 && buf[len - 2] == '\r' && buf [len - 1] == '\n'){
            break;
        }

        if(!moreFlag)
        {
            /* There no more data. break the loop. */
            break;
        }
    }
    return 0;
}


//*****************************************************************************
//
//! \brief Handler for parsing JSON data
//!
//! \param[in]  ptr - Pointer to http response body data
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
int ParseJSONData(char *ptr)
{
	int i;
	long lRetVal = 0;
    int noOfToken;
    jsmn_parser parser;
    jsmntok_t   *tokenList;
	unsigned char keyString[200];

    /* Initialize JSON PArser */
    jsmn_init(&parser);

    /* Get number of JSON token in stream as we we dont know how many tokens need to pass */
    noOfToken = jsmn_parse(&parser, (const char *)ptr, strlen((const char *)ptr), NULL, 10);
    if(noOfToken <= 0)
    {
    	System_printf("Failed to initialize JSON parser\n");
    	System_flush();
    	return -1;

    }

    /* Allocate memory to store token */
    tokenList = (jsmntok_t *) malloc(noOfToken*sizeof(jsmntok_t));
    if(tokenList == NULL)
    {
    	System_printf("Failed to allocate memory\n");
    	System_flush();
        return -1;
    }

    /* Initialize JSON Parser again */
    jsmn_init(&parser);
    noOfToken = jsmn_parse(&parser, (const char *)ptr, strlen((const char *)ptr), tokenList, noOfToken);
    if(noOfToken < 0)
    {
    	System_printf("Failed to parse JSON tokens\n");
    	System_flush();
    	lRetVal = noOfToken;
    }
    else
    {
    	System_printf("Successfully parsed %ld JSON tokens\n", noOfToken);
    	System_flush();
    }

    /* Iterate and extract tokens */
	for (i = 1; tokenList[i].end != 0 && tokenList[i].end < tokenList[0].end; i++)
	{
		jsmntok_t key = tokenList[i];
		jsmntok_t keyValue = tokenList[i+1];

		unsigned int length = key.end - key.start;
		unsigned int lengthValue = keyValue.end - keyValue.start;

		memcpy(keyString, &ptr[key.start], length);
		keyString[length] = '\0';

		System_printf("%i\n", i);
		System_printf("Key: %s\n", keyString);
		System_printf("tokens[0].end: %i\n", tokenList[0].end);
		System_printf("tokens[%i].end: %i\n",i, tokenList[i].end);
		System_flush();


		if (tokenList[i].type == JSMN_STRING || tokenList[i].type == JSMN_PRIMITIVE)
		{
			if (strcmp((const char *)keyString, "access_token") == 0)
			{
				memcpy(accessToken, &ptr[keyValue.start], lengthValue);
				accessToken[lengthValue] = '\0';
			}

			if (strcmp((const char *)keyString, "expires_in") == 0)
			{
				memcpy(expiresIn, &ptr[keyValue.start], lengthValue);
				expiresIn[lengthValue] = '\0';
			}
			System_printf("%.*s\n", tokenList[i].end - tokenList[i].start, ptr + tokenList[i].start);
			System_flush();
		}
		else if (tokenList[i].type == JSMN_ARRAY)
		{
//			System_printf("[%d elems]\n", tokenList[i].size);
		}
		else if (tokenList[i].type == JSMN_OBJECT)
		{
//			System_printf("{%d elems}\n", tokenList[i].size);
		}
		else
		{
			//System_printf(tokens[i]);
		}

	}

	free(tokenList);

    return lRetVal;
}

//*****************************************************************************
//
//!    \brief This function read respose from server and dump on console
//!
//!    \param[in]      httpClient - HTTP Client object
//!
//!    \return         0 on success else -ve
//!
//!    \note
//!
//!    \warning
//!
//*****************************************************************************
static int readResponse(HTTPCli_Handle httpClient)
{
	long lRetVal = 0;
	int bytesRead = 0;
	int id = 0;
	unsigned long len = 0;
	int json = 0;
	char *dataBuffer=NULL;
	bool moreFlags = 1;
	const char *ids[4] = {
	                        HTTPCli_FIELD_NAME_CONTENT_LENGTH,
			                HTTPCli_FIELD_NAME_CONNECTION,
			                HTTPCli_FIELD_NAME_CONTENT_TYPE,
			                NULL
	                     };

	/* Read HTTP POST request status code */
	lRetVal = HTTPCli_getResponseStatus(httpClient);
	if(lRetVal > 0)
	{
		switch(lRetVal)
		{
		case 200:
		case 202:
		{
			System_printf("\nHTTP Status %d\n", lRetVal);
			System_flush();
			/*
                 Set response header fields to filter response headers. All
                  other than set by this call we be skipped by library.
			 */
			HTTPCli_setResponseFields(httpClient, (const char **)ids);

			/* Read filter response header and take appropriate action. */
			/* Note:
                    1. id will be same as index of fileds in filter array setted
                    in previous HTTPCli_setResponseFields() call.

                    2. moreFlags will be set to 1 by HTTPCli_getResponseField(), if  field
                    value could not be completely read. A subsequent call to
                    HTTPCli_getResponseField() will read remaining field value and will
                    return HTTPCli_FIELD_ID_DUMMY. Please refer HTTP Client Libary API
                    documenation @ref HTTPCli_getResponseField for more information.
			 */
			while((id = HTTPCli_getResponseField(httpClient, (char *)g_buff, sizeof(g_buff), &moreFlags))
					!= HTTPCli_FIELD_ID_END)
			{

				switch(id)
				{
				case 0: /* HTTPCli_FIELD_NAME_CONTENT_LENGTH */
				{
					len = strtoul((char *)g_buff, NULL, 0);
				}
				break;
				case 1: /* HTTPCli_FIELD_NAME_CONNECTION */
				{
				}
				break;
				case 2: /* HTTPCli_FIELD_NAME_CONTENT_TYPE */
				{
					if(strncmp((const char *)g_buff, "application/json",
							sizeof("application/json")))
					{
						json = 1;
					}
					else
					{
						/* Note:
                                Developers are advised to use appropriate
                                content handler. In this example all content
                                type other than json are treated as plain text.
						 */
						json = 0;
					}
					System_printf(HTTPCli_FIELD_NAME_CONTENT_TYPE);
					System_printf(" : ");
					System_printf("application/json\n");
					System_flush();
				}
				break;
				default:
				{
					System_printf("Wrong filter id\n");
					System_flush();
					lRetVal = -1;
					goto end;
				}
				}
			}
			bytesRead = 0;
			if(len > sizeof(g_buff))
			{
				dataBuffer = (char *) malloc(len);
				if(dataBuffer)
				{
					System_printf("Failed to allocate memory\n");
					System_flush();
					lRetVal = -1;
					goto end;
				}
			}
			else
			{
				dataBuffer = (char *)g_buff;
			}

			/* Read response data/body */
			/* Note:
                    moreFlag will be set to 1 by HTTPCli_readResponseBody() call, if more
		            data is available Or in other words content length > length of buffer.
		            The remaining data will be read in subsequent call to HTTPCli_readResponseBody().
		            Please refer HTTP Client Libary API documenation @ref HTTPCli_readResponseBody
		            for more information

			 */
			bytesRead = HTTPCli_readResponseBody(httpClient, (char *)dataBuffer, len, &moreFlags);
			if(bytesRead < 0)
			{
				System_printf("Failed to received response body\n");
				System_flush();
				lRetVal = bytesRead;
				goto end;
			}
			else if( bytesRead < len || moreFlags)
			{
				System_printf("Mismatch in content length and received data length\n");
				System_flush();
				goto end;
			}
			dataBuffer[bytesRead] = '\0';

			if(json)
			{
				/* Parse JSON data */
				lRetVal = ParseJSONData(dataBuffer);
				if(lRetVal < 0)
				{
					goto end;
				}
			}
			else
			{
				/* treating data as a plain text */
			}

		}
		break;

		case 404:
			System_printf("File not found. \n");
			System_flush();
			/* Handle response body as per requirement.
                  Note:
                    Developers are advised to take appopriate action for HTTP
                    return status code else flush the response body.
                    In this example we are flushing response body in default
                    case for all other than 200 HTTP Status code.
			 */
		default:
			/* Note:
              Need to flush received buffer explicitly as library will not do
              for next request.Apllication is responsible for reading all the
              data.
			 */
			FlushHTTPResponse(httpClient);
			break;
		}
	}
	else
	{
		System_printf("Failed to receive data from server.\n");
		System_flush();
		goto end;
	}

	lRetVal = 0;

end:
    if(len > sizeof(g_buff) && (dataBuffer != NULL))
	{
	    free(dataBuffer);
    }
    return lRetVal;
}

//*****************************************************************************
//
//!  \brief     Cleans up connection
//!
//! \param[in]  httpClient - Pointer to http client
//!
//! \return 0 on success
//*****************************************************************************
static int cleanUp(HTTPCli_Handle httpClient)
{
    HTTPCli_disconnect(httpClient);
    //HTTPCli_destruct(httpClient);

    return 0;
}


//*****************************************************************************
//
//! \brief HTTP POST Demonstration
//!
//! \param[in]  httpClient - Pointer to http client
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
static int HTTPPostMethod(HTTPCli_Handle httpClient)
{
	int i;
    bool moreFlags = 1;
    bool lastFlag = 1;
    char tmpBuf[4];
    long lRetVal = 0;

    static char bearerToken[200];

 	for (i = 0; i < 200; i++)
 		bearerToken[i] = 0x00;

    strcat(bearerToken, "Bearer ");
    strcat(bearerToken, (const char *)accessToken);
    strcat(bearerToken, "\0");

    HTTPCli_Field fields[4] = {
                                {HTTPCli_FIELD_NAME_HOST, HOST_NAME},
                                {HTTPCli_FIELD_NAME_CONTENT_TYPE, "application/json"},
                                {"Authorization", (const char *)bearerToken},
                                {NULL, NULL}
                            };


    /* Set request header fields to be sent for HTTP request. */
    HTTPCli_setRequestFields(httpClient, fields);

    /* Send POST method request. */
    /* Here we are setting moreFlags = 1 as there are some more header fields need to send
       other than setted in previous call HTTPCli_setRequestFields() at later stage.
       Please refer HTTP Library API documentaion @ref HTTPCli_sendRequest for more information.
    */
    moreFlags = 1;
    lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_POST, POST_REQUEST_URI, moreFlags);
    if(lRetVal < 0)
    {
    	System_printf("Failed to send HTTP POST request header. error(%d)\n", lRetVal);
    	System_flush();
        return lRetVal;
    }

    sprintf((char *)tmpBuf, "%d", (sizeof(POST_DATA)-1));

    /* Here we are setting lastFlag = 1 as it is last header field.
       Please refer HTTP Library API documentaion @ref HTTPCli_sendField for more information.
    */
    lastFlag = 1;
    lRetVal = HTTPCli_sendField(httpClient, HTTPCli_FIELD_NAME_CONTENT_LENGTH, (const char *)tmpBuf, lastFlag);
    if(lRetVal < 0)
    {
    	System_printf("Failed to send HTTP POST request header. error(%d)\n", lRetVal);
    	System_flush();
        return lRetVal;
    }


    /* Send POST data/body */
    lRetVal = HTTPCli_sendRequestBody(httpClient, POST_DATA, (sizeof(POST_DATA)-1));
    if(lRetVal < 0)
    {
    	System_printf("Failed to send HTTP POST request body. error(%d)\n", lRetVal);
    	System_flush();
        return lRetVal;
    }

    lRetVal = readResponse(httpClient);

    return lRetVal;
}


//*****************************************************************************
//
//! \brief HTTP GET Demonstration
//!
//! \param[in]  httpClient - Pointer to http client
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
static int HTTPGetMethod(HTTPCli_Handle httpClient)
{
	int i;
    long lRetVal = 0;
    HTTPCli_Field fields[4] = {
                                {HTTPCli_FIELD_NAME_HOST, HOST_NAME},
                                {HTTPCli_FIELD_NAME_ACCEPT, "*/*"},
                                {HTTPCli_FIELD_NAME_CONTENT_LENGTH, "0"},
                                {NULL, NULL}
                            };
    bool        moreFlags;

    /* Set request header fields to be send for HTTP request. */
    HTTPCli_setRequestFields(httpClient, fields);

    // Create GET Request URI
    static char getUri[150];

 	for (i = 0; i < 150; i++)
 		getUri[i] = 0x00;

    strcat(getUri, "/v1/oauth/token?client_id=");
    strcat(getUri, (const char *)APP_KEY);
    strcat(getUri, "&client_secret=");
    strcat(getUri, (const char *)APP_SECRET);
    strcat(getUri, "&grant_type=client_credentials&scope=SMS");
    strcat(getUri, "\0");

    /* Send GET method request. */
    /* Here we are setting moreFlags = 0 as there are no more header fields need to send
       at later stage. Please refer HTTP Library API documentaion @ HTTPCli_sendRequest
       for more information.
    */
    moreFlags = 0;
    lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_GET, getUri, moreFlags);
    if(lRetVal < 0)
    {
    	System_printf("Failed to send HTTP GET request. error(%d)\n", lRetVal);
    	System_flush();
        return lRetVal;
    }

    lRetVal = readResponse(httpClient);

    return lRetVal;
}

//*****************************************************************************
//
//! Function to connect to HTTP server
//!
//! \param  httpClient - Pointer to HTTP Client instance
//!
//! \return Error-code or SUCCESS
//!
//*****************************************************************************
#define SECURE 1
static int ConnectToHTTPServer(HTTPCli_Handle httpClient)
{
    long lRetVal = -1;
    struct sockaddr_in addr;

#ifdef USE_PROXY
    struct sockaddr_in paddr;
    paddr.sin_family = AF_INET;
    paddr.sin_port = htons(PROXY_PORT);
    paddr.sin_addr.s_addr = sl_Htonl(PROXY_IP);
    HTTPCli_setProxy((struct sockaddr *)&paddr);
#endif

    /* Resolve HOST NAME/IP */
    lRetVal = sl_NetAppDnsGetHostByName((signed char *)HOST_NAME,
                                          strlen((const char *)HOST_NAME),
                                          &g_ulDestinationIP,SL_AF_INET);
    if(lRetVal < 0)
    {
        //ASSERT_ON_ERROR(GET_HOST_IP_FAILED);
    }

#ifdef SECURE
#define SL_SSL_CA_CERT	"/cert/tel.crt"
    struct HTTPCli_SecureParams sparams;
    /* Set secure TLS connection  */
    /* Security parameters */
    sparams.method.secureMethod = SL_SO_SEC_METHOD_TLSV1_2;
    sparams.mask.secureMask = SL_SEC_MASK_TLS_RSA_WITH_AES_256_CBC_SHA; //SL_SEC_MASK_SSL_RSA_WITH_RC4_128_SHA;;
    strncpy(sparams.cafile, SL_SSL_CA_CERT, sizeof(SL_SSL_CA_CERT));
    sparams.privkey[0] = 0;
    sparams.cert[0] = 0;
    sparams.dhkey[0] = 0;
    HTTPCli_setSecureParams(&sparams);
#endif
    /* Set up the input parameters for HTTP Connection */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(HOST_PORT);
    addr.sin_addr.s_addr = sl_Htonl(g_ulDestinationIP);

    /* Testing HTTPCli open call: handle, address params only */
    HTTPCli_construct(httpClient);
#ifdef SECURE
    lRetVal = HTTPCli_connect(httpClient, (struct sockaddr *)&addr, HTTPCli_TYPE_TLS, NULL);
#else
    lRetVal = HTTPCli_connect(httpClient, (struct sockaddr *)&addr, 0, NULL);
#endif
    if (lRetVal < 0)
    {
        System_printf("Connection to server failed. error(%d)\n", lRetVal);
        System_flush();
    }
    else
    {
        System_printf("Connection to server created successfully\n");
        System_flush();
    }

    return 0;
}

//*****************************************************************************
//
//! Callback Function for Board_BUTTON0
//!
//! \param  void
//!
//! \return void
//!
//*****************************************************************************
void gpioButtonFxn0(void)
{
	buttonPressed = 1;
}

//*****************************************************************************
//
//! Function to orhestrate the sms sending process
//!
//! \param  void
//!
//! \return 0 on completion (success or fail)
//!
//*****************************************************************************
int sendSms()
{

    long lRetVal = -1;
    HTTPCli_Struct httpClient;

    System_printf("\n\r");
    System_printf("******** Button Pressed Start ********\n");
    System_flush();

    // Get the current time from an SNTP server
    lRetVal = -1;
    while (lRetVal < 0)
    {
    	lRetVal = GetCurrentTime();
		if(lRetVal < 0)
		{
			System_printf("Failed to get time from SNTP server. Retrying.\n");
			System_flush();
		}
    }

    // Connect securely to server
    lRetVal = ConnectToHTTPServer(&httpClient);
    if(lRetVal < 0)
    {
    	System_printf("ConnectHTTPServer Failed. error(%d)\n", lRetVal);
    	System_flush();
    }

    // Get the access token
    lRetVal = HTTPGetMethod(&httpClient);
    if(lRetVal < 0)
    {
    	System_printf("HTTP GET failed. error(%d)\n", lRetVal);
    	System_flush();
    }
    {
    	System_printf("\nHTTP GET Successful.\n");
    	System_flush();
    }

    // Send SMS
    lRetVal = HTTPPostMethod(&httpClient);
    if(lRetVal < 0)
    {
    	System_printf("\nHTTP POST failed. error(%d)\n", lRetVal);
    	System_flush();
    }
    else
    {
    	System_printf("\nHTTP POST Successful.\n");
    	System_flush();
    }

    cleanUp(&httpClient);

    System_printf("\n\r");
    System_printf("******** Button Pressed End ********\n");
    System_flush();

    return 0;
}

//*****************************************************************************
//! smsdoorbell
//!
//! This is the main program that establishes connectivity and then runs a program
//! loop wait to receive packets
//!
//! Task for this function is created statically. See the project's .cfg file.
//*****************************************************************************
Void smsdoorbell(UArg arg0, UArg arg1)
{
    unsigned char len = sizeof(SlNetCfgIpV4Args_t);
    unsigned char dhcpIsOn = 1;
    SlNetCfgIpV4Args_t ipV4 = {0};
    int               nbytes;
    int               status;
    int				  iretVal;
    int               selectRes;
    int               slSocket;
    fd_set            readSet;
    timeval           timeout;
    sockaddr_in       localAddr;
    sockaddr_in       client_addr;
    socklen_t         addrlen = sizeof(client_addr);
//    int			      bytesSent;
    int				  configValue;

    ULong       currButton;
    ULong		connectionButton;
    ULong       prevButton = 0;

    /* Turn Red and Orange LED OFF. It will be used as a connection indicator */
    GPIO_write(Board_LED0, Board_LED_OFF); //Red
    //GPIO_write(Board_LED1, Board_LED_OFF); //Orange
    //GPIO_write(Board_LED2, Board_LED_ON); //Green

    /*
     * Host driver starts the network processor.
     *
     * sl_Start returns the network processor operating mode:
     *      ROLE_STA (0x00): configured as a station
     *      ROLE_AP  (0x02): configured as an access point
     */

    configValue = sl_Start(NULL, NULL, NULL);
    if (configValue < 0) {
        System_abort("Could not initialize WiFi");
    }

    //Set device name. Maximum length of 33 characters
    iretVal = setDeviceName();
    if (iretVal < 0)
    {
    	System_printf("Failed to set Device Name\n");
    	System_flush();
    }

    // Set the AP domain name
    iretVal = setApDomainName();
    if (iretVal < 0)
    {
    	System_printf("Failed to set AP Domain Name\n");
    	System_flush();
    }

    // Holding down button SW3 while booting will switch between Station or AccessPoint Modes
    // It also sets the AP mode as open security and gives it the standard AP name smsdoorbellAP
    connectionButton = GPIO_read(Board_BUTTON1);
    if(connectionButton != 0)
    {

		if (configValue == ROLE_STA) {

			/* Change mode to access point */
			configValue = sl_WlanSetMode(ROLE_AP);

			// Set SSID name for AP mode
			iretVal = setSsidName();
    	    if (iretVal < 0)
    	    {
    	    	System_printf("Failed to set SSID Name\n");
    	    	System_flush();
    	    }

			//unsigned char  str[33] = "StartProjectAP";
			//unsigned short  length = strlen((const char *)str);
			//sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_SSID, length, str);

			// Set security type for AP mode
			//Security options are:
			//Open security: SL_SEC_TYPE_OPEN
			//WEP security:  SL_SEC_TYPE_WEP
			//WPA security:  SL_SEC_TYPE_WPA
			unsigned char  val = SL_SEC_TYPE_OPEN;
			sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_SECURITY_TYPE, 1, (unsigned char *)&val);

			//Set Password for for AP mode (for WEP or for WPA) example:
			//Password - for WPA: 8 - 63 characters
			//           for WEP: 5 / 13 characters (ascii)
//			unsigned char  strpw[65] = "passWORD";
//			unsigned short  len = strlen((const char *)strpw);
//			memset(strpw, 0, 65);
//			memcpy(strpw, "passWORD", len);
//			sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_PASSWORD, len, (unsigned char *)strpw);

			/* Restart the network processor */
			configValue = sl_Stop(0);
			configValue = sl_Start(NULL, NULL, NULL);
		}
		else if  (configValue == ROLE_AP) {
			/* Change mode to wireless station */
			configValue = sl_WlanSetMode(ROLE_STA);

			/* Restart the network processor */
			configValue = sl_Stop(0);
			configValue = sl_Start(NULL, NULL, NULL);
		}
    }

    // We are in Station Mode, so need to connect to Wifi Router/Access Point
    if (configValue == ROLE_STA)
    {
        /* Turn Red and Orange LED OFF. It will be used as a connection indicator */
        GPIO_write(Board_LED0, Board_LED_OFF); //Red
        //GPIO_write(Board_LED1, Board_LED_OFF); //Orange
        //GPIO_write(Board_LED2, Board_LED_ON); //Green

		/*
		 * Wait for the WiFi to connect to an AP. If a profile for the AP in
		 * use has not been stored yet, press Board_BUTTON0 to start SmartConfig.
		 */
		while ((deviceConnected != true) || (ipAcquired != true)) {
			/*
			 *  Start SmartConfig if a button is pressed. This could be done with
			 *  GPIO interrupts, but for simplicity polling is used to check the
			 *  button.
			 */

			currButton = GPIO_read(Board_BUTTON1);
			if((currButton == 0) && (prevButton != 0))
			{
				smartConfigFxn();
			}
			prevButton = currButton;
			Task_sleep(50);
		}
    }

    // Set the color of LED to indicate which mode we are in
    if (configValue == ROLE_STA)
    {
    	System_printf("Device is in Station Mode\n");
    	System_flush();

        // Turn Green LED on to indicate that device is connected in Station Mode
        GPIO_write(Board_LED0, Board_LED_OFF); //Red
        //GPIO_write(Board_LED1, Board_LED_OFF); //Orange
        //GPIO_write(Board_LED2, Board_LED_ON); //Green

        // Get the IP Address details.
        sl_NetCfgGet(SL_IPV4_STA_P2P_CL_GET_INFO,&dhcpIsOn,&len,(_u8 *)&ipV4);
    }
    else
    {
    	System_printf("Device is in Access Point Mode\n");
    	System_flush();

        // Turn Green LED off to indicate that device is in AP mode
        GPIO_write(Board_LED0, Board_LED_OFF); //Red
        //GPIO_write(Board_LED1, Board_LED_OFF); //Orange
        //GPIO_write(Board_LED2, Board_LED_OFF); //Green

        // Get the IP Adress details
        sl_NetCfgGet(SL_IPV4_AP_P2P_GO_GET_INFO,&dhcpIsOn,&len,(_u8 *)&ipV4);
    }

    // Display the IP Address details
    System_printf("DHCP is %s | IP %d.%d.%d.%d | MASK %d.%d.%d.%d | GW %d.%d.%d.%d | DNS %d.%d.%d.%d\n",
            (dhcpIsOn > 0) ? "ON" : "OFF",
            SL_IPV4_BYTE(ipV4.ipV4,3),SL_IPV4_BYTE(ipV4.ipV4,2),SL_IPV4_BYTE(ipV4.ipV4,1),SL_IPV4_BYTE(ipV4.ipV4,0),
            SL_IPV4_BYTE(ipV4.ipV4Mask,3),SL_IPV4_BYTE(ipV4.ipV4Mask,2),SL_IPV4_BYTE(ipV4.ipV4Mask,1),SL_IPV4_BYTE(ipV4.ipV4Mask,0),
            SL_IPV4_BYTE(ipV4.ipV4Gateway,3),SL_IPV4_BYTE(ipV4.ipV4Gateway,2),SL_IPV4_BYTE(ipV4.ipV4Gateway,1),SL_IPV4_BYTE(ipV4.ipV4Gateway,0),
            SL_IPV4_BYTE(ipV4.ipV4DnsServer,3),SL_IPV4_BYTE(ipV4.ipV4DnsServer,2),SL_IPV4_BYTE(ipV4.ipV4DnsServer,1),SL_IPV4_BYTE(ipV4.ipV4DnsServer,0));
    System_flush();

    GPIO_write(Board_LED0, Board_LED_ON); //Red

    /* Register mDNS */
    iretVal = registerMdnsService();
    if (iretVal < 0)
    {
    	System_printf("mDNS Failed to Register\n");
    	System_flush();
    }
    else
    {
    	System_printf("mDNS Service %s on Port %d Successfully Registered\n", MDNS_SERVICE, UDPPORT);
    	System_flush();
    }

    /* Create a UDP socket */
    slSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (slSocket < 0)
    {
        System_printf("Error: socket not created.");
        Task_exit();
    }

    memset((char *)&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = htonl(0);
    localAddr.sin_port = htons(UDPPORT);

    status = bind(slSocket, (const sockaddr *)&localAddr, sizeof(localAddr));
    if (status < 0)
    {
        System_printf("Error: bind failed.");
        close(slSocket);
        Task_exit();
    }

    /* Loop in case remote application terminates the connection */
    while (1)
    {
        FD_ZERO(&readSet);
        FD_SET(slSocket, &readSet);

        /* Set up 1.5sec timeout for select() - May want to reduce to 1/2sec */
        //memset(&timeout, 0, sizeof(timeval));
        timeout.tv_sec = 1; //TIMEOUT;
        timeout.tv_usec = 500000;

        /* Wait for the reply. If timeout, assume UDP packet dropped */
        selectRes = select(slSocket + 1, &readSet, NULL, NULL, &timeout);

        if ((selectRes > 0) && (selectRes != -1)) // We have received a packet
        {
            if(FD_ISSET(slSocket, &readSet)) // We have received a packet on the UDP port (set in simplelinklibraty.h)
            {
                nbytes = recvfrom(slSocket, buffer, UDPPACKETSIZE, 0,(sockaddr *)&client_addr, &addrlen);
                if (nbytes > 0)
                {
                	if (((uint8_t*)buffer)[0] == 0xFF) // We have a change phone number control packet
                	{
                		// Change the mobile number for the sms
                	}

                	// Send UDP Response to Client - Remove to send response
//                	char statusPacket[40] = "";
//                	strcat(statusPacket, "Packet Received: ");
//                	strcat(statusPacket, (const char *)buffer);
//
//                	const char sp[40];
//                	int spLen = strlen(statusPacket);
//                	memcpy((void *)sp, (const char *)statusPacket, spLen);
//
//                	bytesSent = sendto(slSocket, sp, spLen, 0, (sockaddr *)&client_addr, sizeof(client_addr));
                }
            }
        }
        else if (buttonPressed == 1) // Button has been pressed, sens sms
        {
        	sendSms();
        	buttonPressed = 0;
        }
        else if (selectRes == -1)
        {
            System_printf("Closing socket 0x%x.\n", slSocket);
            close(slSocket);
            Task_exit();
        }
    }
}
