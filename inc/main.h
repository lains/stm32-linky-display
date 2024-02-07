#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#ifdef STM32F469xx
#include "stm32f4xx_hal.h"
#endif

#ifdef USE_STM32469I_DISCOVERY
#include "stm32469i_discovery.h"
//#include "stm32469i_discovery_ts.h"
#include "stm32469i_discovery_sdram.h"
//#include "stm32469i_discovery_sd.h"
#include "stm32469i_discovery_lcd.h"
#include "stm32469i_discovery_qspi.h"
#endif

void Error_Handler(void);
void OnError_Handler(uint32_t condition);

#endif /* __MAIN_H */
