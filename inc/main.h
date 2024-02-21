#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#ifdef STM32F469xx
#include "stm32f4xx_hal.h"
#endif
#ifdef STM32F769xx
#include "stm32f7xx_hal.h"
#endif

#ifdef USE_STM32469I_DISCOVERY
#include "stm32469i_discovery.h"
//#include "stm32469i_discovery_ts.h"
#include "stm32469i_discovery_sdram.h"
//#include "stm32469i_discovery_sd.h"
#include "stm32469i_discovery_lcd.h"
#include "stm32469i_discovery_qspi.h"
#endif

#ifdef USE_STM32F769I_DISCO
#include "stm32f769i_discovery.h"
//#include "stm32f769i_discovery_ts.h"
#include "stm32f769i_discovery_lcd.h"
#include "stm32f769i_discovery_qspi.h"
#endif

#ifdef USE_STM32469I_DISCOVERY
#define LED_GREEN LED1
#define LED_ORANGE LED2
#define LED_RED LED3
#define LED_BLUE LED4
#endif
#ifdef USE_STM32F769I_DISCO
#define LED_RED LED1
#define LED_GREEN LED2
/* The defines below are probably not required */
/* LCD Frame buffer start address : starts at beginning of SDRAM */  
#define ARBG8888_BYTE_PER_PIXEL   4
#define LCD_FB_LENGTH              ((uint32_t)(OTM8009A_480X800_HEIGHT * OTM8009A_480X800_WIDTH * ARBG8888_BYTE_PER_PIXEL))
#define SDRAM_WRITE_READ_ADDR        ((uint32_t)0xC0177000)

#endif

#ifdef LED_BLUE
#define LED_LCD_REFRESH LED_BLUE
#endif
#ifdef LED_RED
#define LED_ERROR LED_RED
#endif
#ifdef LED_ORANGE
#define LED_SERIAL_RX LED_ORANGE
#endif
#if defined(USE_STM32469I_DISCOVERY) && defined(LED_ORANGE)
#define LED_STARTUP_BLINK LED_ORANGE
#endif
#if defined(USE_STM32F769I_DISCO) && defined(LED_RED)
#define LED_STARTUP_BLINK LED_RED
#endif
#ifdef LED_GREEN
#define LED_TIC_FRAME_RX LED_GREEN
#endif

void Error_Handler(void);
void OnError_Handler(uint32_t condition);

#endif /* __MAIN_H */
