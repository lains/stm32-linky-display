/**
  ******************************************************************************
  * @file    LCD_DSI/LCD_DSI_CmdMode_DoubleBuffering/Inc/main.h
  * @author  MCD Application Team
  * @brief   Header for main.c module
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

#include "stm32469i_discovery.h"
//#include "stm32469i_discovery_ts.h"
#include "stm32469i_discovery_sdram.h"
//#include "stm32469i_discovery_sd.h"
#include "stm32469i_discovery_lcd.h"
#include "stm32469i_discovery_qspi.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
#define USART6_CLK_ENABLE()              __HAL_RCC_USART6_CLK_ENABLE()
#define USART6_CLK_DISABLE()             __HAL_RCC_USART6_CLK_DISABLE()
#define USART6_RX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOC_CLK_ENABLE()
#define USART6_TX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOC_CLK_ENABLE()

#define USART6_FORCE_RESET()             __HAL_RCC_USART6_FORCE_RESET()
#define USART6_RELEASE_RESET()           __HAL_RCC_USART6_RELEASE_RESET()

/* Definition for USARTx Pins */
#define USART6_TX_PIN                    GPIO_PIN_6
#define USART6_TX_GPIO_PORT              GPIOC
#define USART6_TX_AF                     GPIO_AF8_USART6
#define USART6_RX_PIN                    GPIO_PIN_7
#define USART6_RX_GPIO_PORT              GPIOC
#define USART6_RX_AF                     GPIO_AF8_USART6

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void Toggle_Leds(void);


#endif /* __MAIN_H */
