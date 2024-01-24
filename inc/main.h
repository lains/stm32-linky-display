#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "stm32f7xx_hal.h"
#include "stm32f769i_discovery.h"
//#include "stm32f769i_discovery_ts.h"
#include "stm32f769i_discovery_lcd.h"
#include "stm32f769i_discovery_qspi.h"

/* LCD Frame buffer start address : starts at beginning of SDRAM */  
#define ARBG8888_BYTE_PER_PIXEL   4
#define LCD_FB_LENGTH              ((uint32_t)(OTM8009A_480X800_HEIGHT * OTM8009A_480X800_WIDTH * ARBG8888_BYTE_PER_PIXEL))
#define SDRAM_WRITE_READ_ADDR        ((uint32_t)0xC0177000)

/* The Audio file is flashed with ST-Link Utility @ flash address =  AUDIO_SRC_FILE_ADDRESS */
#define AUDIO_SRC_FILE_ADDRESS       0x08080000   /* Audio file address in flash */
#define AUDIO_FILE_SIZE              0x80000

void Error_Handler(void);
void OnError_Handler(uint32_t condition);

#endif /* __MAIN_H */
