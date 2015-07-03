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

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/Memory.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>

/* Example/Board Header files */
#include "Board.h"

///* SimpleLink Wi-Fi Host Driver Header files */
//#include <simplelink.h>
#include <osi.h>

#include "smsdoorbell.h"

/* Spawn Task Priority */
int SPAWN_TASK_PRI = 3;

/*
 *  ======== main ========
 */
int main(void)
{
    /* Call board init functions */
    Board_initGeneral();
    Board_initGPIO();

    /* Turn on user LED */
    GPIO_write(Board_LED0, Board_LED_ON);

    System_printf("Starting the SMS Door Bell example\nSystem provider is set"
                  " to SysMin. Halt the target to view any SysMin contents in"
                  " ROV.\n\n");
    /* SysMin will only print to the console when you call flush or exit */
    System_flush();

    /* Turn off All LEDs. It will be used as a connection indicator */
    GPIO_write(Board_LED0, Board_LED_ON); //Red
    //GPIO_write(Board_LED1, Board_LED_ON); //Orange
    //GPIO_write(Board_LED2, Board_LED_ON); //Green

    /* install Button callback */
    GPIO_setCallback(Board_BUTTON0, gpioButtonFxn0);

    /* Enable interrupts */
    GPIO_enableInt(Board_BUTTON0);

    /*
     *  If more than one input pin is available for your device, interrupts
     *  will be enabled on Board_BUTTON1.
     */
//    if (Board_BUTTON0 != Board_BUTTON1) {
//        /* install Button callback */
//        GPIO_setCallback(Board_BUTTON1, gpioButtonFxn1);
//        GPIO_enableInt(Board_BUTTON1);
//    }

    /*
     * The SimpleLink Host Driver requires a mechanism to allow functions to
     * execute in temporary context.  The SpawnTask is created to handle such
     * situations.  This task will remain blocked until the host driver
     * posts a function.  If the SpawnTask priority is higher than other tasks,
     * it will immediately execute the function and return to a blocked state.
     * Otherwise, it will remain ready until it is scheduled.
     */
    VStartSimpleLinkSpawnTask(SPAWN_TASK_PRI);

    /* Start BIOS */
    BIOS_start();

    return (0);
}
