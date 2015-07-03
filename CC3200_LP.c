/*
 * Copyright (c) 2015, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== CC3200_LP.c ========
 *  This file is responsible for setting up the board specific items for the
 *  CC3200_LP board.
 */

#include <stdint.h>
#include <stdbool.h>

#include <xdc/std.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/family/arm/m3/Hwi.h>

#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/prcm.h>
#include <driverlib/uart.h>
#include <driverlib/udma.h>

#include "CC3200_LP.h"
#include "pin_mux_config.h"

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC3200.h>

/*
 *  This define determines whether to use the UARTCC3200DMA driver
 *  or the UARTCC3200 (no DMA) driver.  Set to 1 to use the UARTCC3200DMA
 *  driver.
 */
#ifndef TI_DRIVERS_UART_DMA
#define TI_DRIVERS_UART_DMA 0
#endif

/*
 *  =============================== DMA ===============================
 */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_ALIGN(dmaControlTable, 1024)
#elif defined(__IAR_SYSTEMS_ICC__)
#pragma data_alignment=1024
#elif defined(__GNUC__)
__attribute__ ((aligned (1024)))
#endif
static tDMAControlTable dmaControlTable[64];
static bool dmaInitialized = false;

/* Hwi_Struct used in the initDMA Hwi_construct call */
static Hwi_Struct hwiStruct;

/*
 *  ======== dmaErrorHwi ========
 */
static Void dmaErrorHwi(UArg arg)
{
    System_printf("DMA error code: %d\n", MAP_uDMAErrorStatusGet());
    MAP_uDMAErrorStatusClear();
    System_abort("DMA error!!");
}

/*
 *  ======== CC3200_LP_initDMA ========
 */
void CC3200_LP_initDMA(void)
{
    Error_Block eb;
    Hwi_Params  hwiParams;

    if (!dmaInitialized) {
        Error_init(&eb);
        Hwi_Params_init(&hwiParams);
        Hwi_construct(&(hwiStruct), INT_UDMAERR, dmaErrorHwi,
                      &hwiParams, &eb);
        if (Error_check(&eb)) {
            System_abort("Couldn't create DMA error hwi");
        }

        MAP_PRCMPeripheralClkEnable(PRCM_UDMA, PRCM_RUN_MODE_CLK);
        MAP_PRCMPeripheralReset(PRCM_UDMA);
        MAP_uDMAEnable();
        MAP_uDMAControlBaseSet(dmaControlTable);

        dmaInitialized = true;
    }
}

/*
 *  =============================== General ===============================
 */
/*
 *  ======== CC3200_LP_initGeneral ========
 */
void CC3200_LP_initGeneral(void)
{
    PinMuxConfig();
    Power_init();
}

/*
 *  =============================== GPIO ===============================
 */
/* Place into subsections to allow the TI linker to remove items properly */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(GPIOCC3200_config, ".const:GPIOCC3200_config")
#endif

#include <ti/drivers/GPIO.h>
#include <ti/drivers/gpio/GPIOCC3200.h>

/*
 * Array of Pin configurations
 * NOTE: The order of the pin configurations must coincide with what was
 *       defined in CC3200_LP.h
 * NOTE: Pins not used for interrupts should be placed at the end of the
 *       array.  Callback entries can be omitted from callbacks array to
 *       reduce memory usage.
 */
GPIO_PinConfig gpioPinConfigs[] = {
    /* input pins with callbacks */
    /* CC3200_LP_SW2 */
    GPIOCC3200_GPIO_22 | GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_RISING,
    /* CC3200_LP_SW3 */
    GPIOCC3200_GPIO_13 | GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_RISING,

    /* output pins */
    /* CC3200_LP_LED_D7 */
    GPIOCC3200_GPIO_09 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
/*
 *  CC3200_LP_LED_D5 and CC3200_LP_LED_D6 are shared with the I2C and PWM
 *  peripherals. In order for those examples to work, these LEDs are taken out
 *  of gpioPinCOnfig[]
 */
    /* CC3200_LP_LED_D6 */
    //GPIOCC3200_GPIO_10 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
    /* CC3200_LP_LED_D5 */
    //GPIOCC3200_GPIO_11 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
};

/*
 * Array of callback function pointers
 * NOTE: The order of the pin configurations must coincide with what was
 *       defined in CC3200_LP.h
 * NOTE: Pins not used for interrupts can be omitted from callbacks array to
 *       reduce memory usage (if placed at end of gpioPinConfigs array).
 */
GPIO_CallbackFxn gpioCallbackFunctions[] = {
    /* CC3200_LP_SW2 */
    NULL,
    /* CC3200_LP_SW3 */
    NULL
};

/* The device-specific GPIO_config structure */
const GPIOCC3200_Config GPIOCC3200_config = {
    .pinConfigs = (GPIO_PinConfig *)gpioPinConfigs,
    .callbacks = (GPIO_CallbackFxn *)gpioCallbackFunctions,
    .numberOfPinConfigs = sizeof(gpioPinConfigs)/sizeof(GPIO_PinConfig),
    .numberOfCallbacks = sizeof(gpioCallbackFunctions)/sizeof(GPIO_CallbackFxn),
    .intPriority = ~0
};

/*
 *  ======== CC3200_LP_initGPIO ========
 */
void CC3200_LP_initGPIO(void)
{
    /* Initialize peripheral and pins */
    GPIO_init();
}

/*
 *  =============================== I2C ===============================
 */
/* Place into subsections to allow the TI linker to remove items properly */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(I2C_config, ".const:I2C_config")
#pragma DATA_SECTION(i2cCC3200HWAttrs, ".const:i2cCC3200HWAttrs")
#endif

#include <ti/drivers/I2C.h>
#include <ti/drivers/i2c/I2CCC3200.h>

I2CCC3200_Object i2cCC3200Objects[CC3200_LP_I2CCOUNT];

/* I2C configuration structure */
const I2CCC3200_HWAttrs i2cCC3200HWAttrs[CC3200_LP_I2CCOUNT] = {
    {I2CA0_BASE, INT_I2CA0, ~0 /* interrupt priority */}
};

const I2C_Config I2C_config[] = {
    {&I2CCC3200_fxnTable, &i2cCC3200Objects[0], &i2cCC3200HWAttrs[0]},
    {NULL, NULL, NULL}
};

/*
 *  ======== CC3200_initI2C ========
 */
void CC3200_LP_initI2C(void)
{
    I2C_init();
}

/*
 *  =============================== I2S ===============================
 */
/* Place into subsections to allow the TI linker to remove items properly */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(I2S_config, ".const:I2S_config")
#pragma DATA_SECTION(i2sCC3200HWAttrs, ".const:i2sCC3200HWAttrs")
#endif

#include <ti/drivers/I2S.h>
#include <ti/drivers/i2s/I2SCC3200DMA.h>

I2SCC3200DMA_Object i2sCC3200Objects[CC3200_LP_I2SCOUNT];

/* I2C configuration structure */
const I2SCC3200DMA_HWAttrs i2sCC3200HWAttrs[CC3200_LP_I2SCOUNT] = {
    {I2S_BASE, INT_I2S,
     ~0,  /* I2S interrupt priority */
     UDMA_CH4_I2S_RX, UDMA_CH5_I2S_TX}
};

const I2S_Config I2S_config[] = {
    {&I2SCC3200DMA_fxnTable, &i2sCC3200Objects[0], &i2sCC3200HWAttrs[0]},
    {NULL, NULL, NULL}
};

/*
 *  ======== CC3200_LP_initI2S ========
 */
void CC3200_LP_initI2S(void)
{
    CC3200_LP_initDMA();
    I2S_init();
}

/*
 *  =============================== Power ===============================
 */
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC3200.h>

/*
 *  ======== PowerCC3200_config ========
 *  In this configuration, Power management is disabled since runPolicy
 *  is set to 0.  Power management can be enabled from main() by calling
 *  Power_enablePolicy(), or by changing runPolicy to 1 in this structure.
 */
const PowerCC3200_Config PowerCC3200_config = {
    &PowerCC3200_initPolicy,   /* policyInitFxn */
    &PowerCC3200_sleepPolicy,  /* policyFxn */
    NULL,                      /* enterLPDSHookFxn */
    NULL,                      /* resumeLPDSHookFxn */
    0,                         /* enablePolicy */
    1,                         /* enableGPIOWakeupLPDS */
    0,                         /* enableGPIOWakeupShutdown */
    0,                         /* enableNetworkWakeupLPDS */
    PRCM_LPDS_GPIO13,          /* wakeupGPIOSourceLPDS */
    PRCM_LPDS_FALL_EDGE,       /* wakeupGPIOTypeLPDS */
    0,                         /* wakeupGPIOSourceShutdown */
    0,                         /* wakeupGPIOTypeShutdown */
    PRCM_SRAM_COL_1|PRCM_SRAM_COL_2|PRCM_SRAM_COL_3|PRCM_SRAM_COL_4
                               /* ramRetentionMaskLPDS */
};

/*
 *  =============================== PWM ===============================
 */
/* Place into subsections to allow the TI linker to remove items properly */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(PWM_config, ".const:PWM_config")
#pragma DATA_SECTION(pwmTimerCC3200HWAttrs, ".const:pwmTimerCC3200HWAttrs")
#endif

#include <ti/drivers/PWM.h>
#include <ti/drivers/pwm/PWMTimerCC3200.h>
#include <driverlib/timer.h>

PWMTimerCC3200_Object pwmTimerCC3200Objects[CC3200_LP_PWMCOUNT];

/* PWM configuration structure */
const PWMTimerCC3200_HWAttrs pwmTimerCC3200HWAttrs[CC3200_LP_PWMCOUNT] = {
    {TIMERA3_BASE, TIMER_A},    /* CC3200_LP_PWM6 */
    {TIMERA3_BASE, TIMER_B}     /* CC3200_LP_PWM7 */
};

const PWM_Config PWM_config[] = {
    {
        &PWMTimerCC3200_fxnTable,
        &pwmTimerCC3200Objects[0],
        &pwmTimerCC3200HWAttrs[0]
    },
    {
        &PWMTimerCC3200_fxnTable,
        &pwmTimerCC3200Objects[1],
        &pwmTimerCC3200HWAttrs[1]
    },
    {NULL, NULL, NULL}
};

/*
 *  ======== CC3200_LP_initPWM ========
 */
void CC3200_LP_initPWM(void)
{
    PWM_init();
}

/*
 *  =============================== SDSPI ===============================
 */
/* Place into subsections to allow the TI linker to remove items properly */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(SDSPI_config, ".const:SDSPI_config")
#pragma DATA_SECTION(sdspiCC3200HWattrs, ".const:sdspiCC3200HWattrs")
#endif

#include <ti/drivers/SDSPI.h>
#include <ti/drivers/sdspi/SDSPICC3200.h>
#include <driverlib/gpio.h>
#include <driverlib/pin.h>

SDSPICC3200_Object sdspiCC3200Objects[CC3200_LP_SDSPICOUNT];

/* SDSPI configuration structure, describing which pins are to be used */
const SDSPICC3200_HWAttrs sdspiCC3200HWattrs[CC3200_LP_SDSPICOUNT] = {
    {
        GSPI_BASE,      /* SPI Peripheral's base address */
        PRCM_GSPI,      /* SPI PRCM peripheral number */

        GPIOA0_BASE,    /* CS GPIO base */
        GPIO_PIN_7,     /* CS GPIO pin number */

        GPIOA2_BASE,    /* GPIO base addr when using MOSI as GPIO */
        GPIO_PIN_0,     /* GPIO pin number when using MOSI as GPIO */
        PIN_MODE_0,     /* GPIO pin mode to use MOSI as GPIO */
        PIN_MODE_7,     /* Pin mode to use MOSI */
        PIN_07          /* Package pin number */
    }
};

const SDSPI_Config SDSPI_config[] = {
    {&SDSPICC3200_fxnTable, &sdspiCC3200Objects[0], &sdspiCC3200HWattrs[0]},
    {NULL, NULL, NULL}
};

/*
 *  ======== CC3200_LP_initSDSPI ========
 */
void CC3200_LP_initSDSPI(void)
{
    /* Raise the CS pin to deselect the SD card */
    MAP_GPIOPinWrite(GPIOA0_BASE, GPIO_PIN_7, GPIO_PIN_7);

    SDSPI_init();
}

/*
 *  =============================== SPI ===============================
 */
/* Place into subsections to allow the TI linker to remove items properly */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(SPI_config, ".const:SPI_config")
#pragma DATA_SECTION(spiCC3200DMAHWAttrs, ".const:spiCC3200DMAHWAttrs")
#endif

#include <ti/drivers/SPI.h>
#include <ti/drivers/spi/SPICC3200DMA.h>
#include <driverlib/spi.h>

SPICC3200DMA_Object SPICC3200DMAObjects[CC3200_LP_SPICOUNT];

#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_ALIGN(spiCC3200DMAscratchBuf, 32)
#elif defined(__IAR_SYSTEMS_ICC__)
#pragma data_alignment=32
#elif defined(__GNUC__)
__attribute__ ((aligned (32)))
#endif
uint32_t spiCC3200DMAscratchBuf[CC3200_LP_SPICOUNT];

/* SPI configuration structure */
const SPICC3200DMA_HWAttrs spiCC3200DMAHWAttrs[CC3200_LP_SPICOUNT] = {
    {
        GSPI_BASE,
        INT_GSPI,
        ~0,   /* Interrupt priority */
        PRCM_GSPI,
        SPI_HW_CTRL_CS,
        SPI_CS_ACTIVELOW,
        SPI_4PIN_MODE,
        SPI_TURBO_OFF,
        &spiCC3200DMAscratchBuf[0],
        0,
        UDMA_CH6_GSPI_RX,
        UDMA_CH7_GSPI_TX,
    }
};

const SPI_Config SPI_config[] = {
    {&SPICC3200DMA_fxnTable, &SPICC3200DMAObjects[0], &spiCC3200DMAHWAttrs[0]},
    {NULL, NULL, NULL},
};

/*
 *  ======== CC3200_LP_initSPI ========
 */
void CC3200_LP_initSPI(void)
{
    CC3200_LP_initDMA();
    SPI_init();
}

/*
 *  =============================== UART ===============================
 */
/* Place into subsections to allow the TI linker to remove items properly */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(UART_config, ".const:UART_config")
#pragma DATA_SECTION(uartCC3200HWAttrs, ".const:uartCC3200HWAttrs")
#endif

#include <ti/drivers/UART.h>
#if TI_DRIVERS_UART_DMA
#include <ti/drivers/uart/UARTCC3200DMA.h>

UARTCC3200DMA_Object uartCC3200Objects[CC3200_LP_UARTCOUNT];

/* UART configuration structure */
const UARTCC3200DMA_HWAttrs uartCC3200HWAttrs[CC3200_LP_UARTCOUNT] = {
    {
        /* CC3200_UARTA0 */
        UARTA0_BASE,
        INT_UARTA0,
        ~0,     /* Interrupt priority */
        UDMA_CH8_UARTA0_RX,
        UDMA_CH9_UARTA0_TX
    },
    {
        /* CC3200_UARTA1 */
        UARTA1_BASE,
        INT_UARTA1,
        ~0,     /* Interrupt priority */
        UDMA_CH10_UARTA1_RX,
        UDMA_CH11_UARTA1_TX
    },
};

const UART_Config UART_config[] = {
    {
        &UARTCC3200DMA_fxnTable,
        &uartCC3200Objects[0],
        &uartCC3200HWAttrs[0]
    },
    {
        &UARTCC3200DMA_fxnTable,
        &uartCC3200Objects[1],
        &uartCC3200HWAttrs[1]
    },
    {NULL, NULL, NULL}
};
#else
#include <ti/drivers/uart/UARTCC3200.h>

UARTCC3200_Object uartCC3200Objects[CC3200_LP_UARTCOUNT];
unsigned char uartCC3200RingBuffer[CC3200_LP_UARTCOUNT][32];

/* UART configuration structure */
const UARTCC3200_HWAttrs uartCC3200HWAttrs[CC3200_LP_UARTCOUNT] = {
    {/* CC3200_UARTA0 */
        .baseAddr = UARTA0_BASE,
        .intNum = INT_UARTA0,
        .intPriority = ~0,
        .flowControl = UART_FLOWCONTROL_NONE,
        .ringBufPtr  = uartCC3200RingBuffer[0],
        .ringBufSize = sizeof(uartCC3200RingBuffer[0]),
    },
    {/* CC3200_UARTA1 */
        .baseAddr = UARTA1_BASE,
        .intNum = INT_UARTA1,
        .intPriority = ~0,
        .flowControl = UART_FLOWCONTROL_NONE,
        .ringBufPtr  = uartCC3200RingBuffer[1],
        .ringBufSize = sizeof(uartCC3200RingBuffer[1]),
    }
};

const UART_Config UART_config[] = {
    {
        &UARTCC3200_fxnTable,
        &uartCC3200Objects[0],
        &uartCC3200HWAttrs[0]
    },
    {
        &UARTCC3200_fxnTable,
        &uartCC3200Objects[1],
        &uartCC3200HWAttrs[1]
    },
    {NULL, NULL, NULL}
};
#endif /* TI_DRIVERS_UART_DMA */

/*
 *  ======== CC3200_LP_initUART ========
 */
void CC3200_LP_initUART(void)
{
#if TI_DRIVERS_UART_DMA
    CC3200_LP_initDMA();
#endif
    UART_init();
}

/*
 *  =============================== Watchdog ===============================
 */
/* Place into subsections to allow the TI linker to remove items properly */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(Watchdog_config, ".const:Watchdog_config")
#pragma DATA_SECTION(watchdogCC3200HWAttrs, ".const:watchdogCC3200HWAttrs")
#endif

#include <ti/drivers/Watchdog.h>
#include <ti/drivers/watchdog/WatchdogCC3200.h>
#include <driverlib/wdt.h>

WatchdogCC3200_Object watchdogCC3200Objects[CC3200_LP_WATCHDOGCOUNT];

/* Watchdog configuration structure */
const WatchdogCC3200_HWAttrs watchdogCC3200HWAttrs[CC3200_LP_WATCHDOGCOUNT] = {
    /* CC3200_WDA0 with 1 sec period at default CPU clock freq */
    {WDT_BASE, INT_WDT, ~0 /* interrupt priority */, 80000000},
};

const Watchdog_Config Watchdog_config[] = {
    {
        &WatchdogCC3200_fxnTable,
        &watchdogCC3200Objects[0],
        &watchdogCC3200HWAttrs[0]
    },
    {NULL, NULL, NULL},
};

/*
 *  ======== CC3200_LP_initWatchdog ========
 */
void CC3200_LP_initWatchdog(void)
{
    MAP_PRCMPeripheralClkEnable(PRCM_WDT, PRCM_RUN_MODE_CLK);
    MAP_PRCMPeripheralReset(PRCM_WDT);

    Watchdog_init();
}
