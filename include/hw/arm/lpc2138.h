#ifndef LPC2148_H_

#include "qemu-common.h"
#include "qemu/error-report.h"
#include "hw/char/serial.h"
#include "hw/arm/arm.h"
//#include "hw/timer/allwinner-a10-pit.h"
//#include "hw/intc/allwinner-a10-pic.h"
//#include "hw/net/allwinner_emac.h"

#include "sysemu/sysemu.h"
#include "exec/address-spaces.h"

#define LPC2138_WATCHDOG_BASE       0xE0000000 
#define LPC2138_TIMER_0_BASE        0xE0004000
#define LPC2138_TIMER_1_BASE        0xE0008000
#define LPC2138_UART_0_BASE         0xE000C000
#define LPC2138_UART_1_BASE         0xE0010000
#define LPC2138_PWM_BASE            0xE0014000
#define LPC2138_I2C_0_BASE          0xE001C000
#define LPC2138_SPI_0_BASE          0xE0020000
#define LPC2138_RTC_BASE            0xE0024000
#define LPC2138_GPIO_BASE           0xE0028000
#define LPC2138_PIN_CONNECT_BASE    0xE002C000
#define LPC2138_AD0_BASE            0xE0034000
#define LPC2138_I2C_1_BASE          0xE005C000
#define LPC2138_SSP_BASE            0xE0068000
#define LPC2138_DAC_BASE            0xE006C000
#define LPC2138_SYSTEM_CTRL_BASE    0xE00FC000

//#define TYPE_AW_A10 "allwinner-a10"
//#define AW_A10(obj) OBJECT_CHECK(AwA10State, (obj), TYPE_AW_A10)

#define LPC2138_H_
#endif
