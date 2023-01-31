/* Includes ------------------------------------------------------------------*/
extern "C" {
#include "main.h"
#include <string.h>
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/
extern LTDC_HandleTypeDef hltdc_eval;
static DMA2D_HandleTypeDef           hdma2d;
extern DSI_HandleTypeDef hdsi_eval;
UART_HandleTypeDef huart6;
}

/* Private define ------------------------------------------------------------*/
const unsigned int LCDWidth = 800;
const unsigned int LCDHeight = 480;
const unsigned int BytesPerPixel = 4; /* For ARGB8888 mode */

#define VSYNC               1 
#define VBP                 1 
#define VFP                 1
#define VACT                LCDHeight
#define HSYNC               1
#define HBP                 1
#define HFP                 1
#define HACT                LCDWidth

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

typedef enum {
	SwitchToDraftIsPending = 0,
	DisplayingDraft,
	CopyingDraftToFinalIsPending,
	CopyDraftToFinalIsDone, /* Unused, would be nice not to poll on DMA2D while copying */
	SwitchToFinalIsPending,
	DisplayingFinal
} LCD_Display_Update_State;

static __IO LCD_Display_Update_State display_state = SwitchToDraftIsPending;

//static uint32_t ImageIndex = 0;
/*static const uint32_t * Images[] = 
{
  image_320x240_argb8888,
  life_augmented_argb8888,  
};*/

extern "C" {
  static void* const draft_fb_address = (void *)LCD_FB_START_ADDRESS;
  static void* const final_fb_address = draft_fb_address + LCDWidth*LCDHeight*BytesPerPixel; //(void *)(LCD_FB_START_ADDRESS + LCDWidth*LCDHeight*BytesPerPixel);
}

static uint8_t UART6_rxBuffer[1] = {0};   /* Our incoming serial buffer, filled-in by the receive interrupt handler */
static unsigned char TIC_rxBuffer[256];
static unsigned int TIC_rxBufferLen = 0;

/* Private function prototypes -----------------------------------------------*/
extern "C" {
static void SystemClock_Config(void);
static void OnError_Handler(uint32_t condition);
static void copy_framebuffer(const uint32_t *pSrc, uint32_t *pDst, uint16_t x, uint16_t y, uint16_t xsize, uint16_t ysize);
static uint8_t LCD_Init(void);
void LTDC_Init(void);
static void MX_USART6_UART_Init(void);
}

/* Private functions ---------------------------------------------------------*/

extern "C" {

static void MX_USART6_UART_Init(void)
{
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 9600;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;  // Note 7bits+parity bit
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_EVEN;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
/*  if (HAL_UART_DeInit(&huart6) != HAL_OK)
  {
    OnError_Handler(1);
  }*/
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
	  OnError_Handler(1);
  }
}

/**
  * @brief  On Error Handler on condition TRUE.
  * @param  condition : Can be TRUE or FALSE
  * @retval None
  */
static void OnError_Handler(uint32_t condition)
{
  if(condition)
  {
    BSP_LED_On(LED3);
    while(1) { ; } /* Blocking on error */
  }
}

static void Error_Handler()
{
	OnError_Handler(1);
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
 {
	GPIO_InitTypeDef GPIO_InitStruct;
  memset(&GPIO_InitStruct, 0, sizeof(GPIO_InitStruct));

    if(huart->Instance!=USART6 /*&& huart->Instance!=USART3*/)
    {
      return;
    }

    if(huart->Instance==USART6)
    {
      USART6_TX_GPIO_CLK_ENABLE();
      USART6_RX_GPIO_CLK_ENABLE();

      /* Peripheral clock enable */
      USART6_CLK_ENABLE();

      /**USART6 GPIO Configuration
      PC6     ------> USART6_TX
      PC7     ------> USART6_RX
      */
      GPIO_InitStruct.Pin = USART6_TX_PIN;
      GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
      GPIO_InitStruct.Pull = GPIO_PULLUP;
      GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
      GPIO_InitStruct.Alternate = USART6_TX_AF;
      HAL_GPIO_Init(USART6_TX_GPIO_PORT, &GPIO_InitStruct);

      GPIO_InitStruct.Pin = USART6_RX_PIN;
      GPIO_InitStruct.Alternate = USART6_RX_AF;
      HAL_GPIO_Init(USART6_RX_GPIO_PORT, &GPIO_InitStruct);

      /* USART6 interrupt Init */
      HAL_NVIC_SetPriority(USART6_IRQn, 0, 0);
      HAL_NVIC_EnableIRQ(USART6_IRQn);
    }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
    /*##-1- Reset peripherals ##################################################*/
  if(huart->Instance==USART6)
  {
    /* Peripheral clock disable */
    USART6_FORCE_RESET();
    USART6_RELEASE_RESET();

    /*##-2- Disable peripherals and GPIO Clocks #################################*/
    USART6_CLK_DISABLE(); // Or __HAL_RCC_USART1_CLK_DISABLE();
    /* De-Initialize USART6 Tx and RX */
    /**USART6 GPIO Configuration
    PC6     ------> USART6_TX
    PC7     ------> USART6_RX
    */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_6|GPIO_PIN_7);
    /* USART6 interrupt DeInit */
    HAL_NVIC_DisableIRQ(USART6_IRQn);
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    unsigned char Received_Data = UART6_rxBuffer[0];

//    if (USB_Hooks->On_Byte_Received)
//    {
//        USB_Hooks->On_Byte_Received((u8) Received_Data);
//    }
    TIC_rxBuffer[TIC_rxBufferLen] = Received_Data;
    TIC_rxBufferLen++;
    if (TIC_rxBufferLen>=sizeof(TIC_rxBuffer)) {
    	TIC_rxBufferLen=0;	/* FIXME: Wrap around in case of buffer overflow */
    }

    BSP_LED_Toggle(LED2);
    HAL_UART_Receive_IT(&huart6, UART6_rxBuffer, 1);
}
} // extern C

// void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) { }

//void DRV_Com_UART_1_Send(u8* Data, u16 Length) {
//    HAL_UART_Transmit_IT(&huart6, (uint8_t*)Data, (uint16_t)Length);
//}

void write_byte_as_hex(unsigned char byte) {
  char msg[]="0x@@";
  unsigned char nibble;
  nibble = ((byte >> 4) & 0xf);
  msg[2]=(nibble<=9)?'0'+nibble:nibble-10+'a';
  nibble = (byte & 0xf);
  msg[3]=(nibble<=9)?'0'+nibble:nibble-10+'a';
  if(HAL_UART_Transmit(&huart6, (uint8_t*)msg, (uint16_t)strlen(msg), 500)!= HAL_OK)
  {
	Error_Handler();
  }
}

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  uint8_t  lcd_status = LCD_OK;
  
  /* STM32F4xx HAL library initialization:
    - Configure the Flash prefetch, instruction and Data caches
    - Systick timer is configured by default as source of time base, but user 
      can eventually implement his proper time base source (a general purpose 
      timer for example or other time source), keeping in mind that Time base 
      duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and 
      handled in milliseconds basis.
    - Set NVIC Group Priority to 4
    - Low Level Initialization: global MSP (MCU Support Package) initialization
  */
  HAL_Init();
  
  /* Configure the system clock to 180 MHz */
  SystemClock_Config();
  
  memset(TIC_rxBuffer, 0, sizeof(TIC_rxBuffer));
  /* Configure LED1, LED2, LED3 and LED4 */
  BSP_LED_Init(LED1);
  BSP_LED_Init(LED2);
  BSP_LED_Init(LED3);
  BSP_LED_Init(LED4);
  BSP_LED_On(LED2);
  HAL_Delay(250);
  BSP_LED_Off(LED2);

  /* Initialize the SDRAM */
  BSP_SDRAM_Init();
  //BSP_SD_Init();
  
  /* Initialize the USART for TIC */
  MX_USART6_UART_Init();

  HAL_UART_Receive_IT(&huart6, UART6_rxBuffer, 1);

  //BSP_TS_Init(800,480);
  /* Initialize the LCD   */
  lcd_status = LCD_Init();
  OnError_Handler(lcd_status != LCD_OK); 
  
  BSP_LCD_LayerDefaultInit(0, (uint32_t)draft_fb_address);
  BSP_LCD_SelectLayer(0); 

  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  BSP_LCD_FillRect(0, 0, LCDWidth, LCDHeight);
  BSP_LCD_SetTextColor(LCD_COLOR_RED);
  BSP_LCD_FillCircle(30, 30, 30);

  display_state = SwitchToDraftIsPending;

  HAL_DSI_Refresh(&hdsi_eval);

  while (display_state==SwitchToDraftIsPending);	/* Wait until the LCD displays the draft framebuffer */

  copy_framebuffer((const uint32_t*)draft_fb_address, (uint32_t*)final_fb_address, 0, 0, LCDWidth, LCDHeight);
  display_state = SwitchToFinalIsPending;

  HAL_DSI_Refresh(&hdsi_eval);

  char msg1[]="Buffers created. Starting...\r\n";
  if (HAL_UART_Transmit(&huart6, (uint8_t*)msg1, (uint16_t)strlen(msg1), 500)!= HAL_OK)
  {
    Error_Handler();
  }

  //set_active_fb_addr(final_fb_address);	/* Draw the copy on the LCD, not the pending one */

  unsigned int count = 0;
  while (1) {
	  while (display_state == SwitchToDraftIsPending); /* Wait until the LCD displays the final framebuffer */
	  /* We can now work on pending buffer */
	  char count_as_str[]="@@@@";
	  count_as_str[0]=(count / 1000) % 10 + '0';
	  count_as_str[1]=(count / 100) % 10 + '0';
	  count_as_str[2]=(count / 10) % 10 + '0';
	  count_as_str[3]=(count / 1) % 10 + '0';
      BSP_LCD_SetFont(&Font24);
      BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
      BSP_LCD_FillRect(0, 24*3, 800, 24*2);
      BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
      BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
      BSP_LCD_DisplayStringAtLine(4, (uint8_t *)count_as_str);
	  BSP_LED_On(LED1);
	  HAL_Delay(250);
	  BSP_LED_Off(LED1);

      display_state = SwitchToDraftIsPending;

      HAL_DSI_Refresh(&hdsi_eval);

      while (display_state==SwitchToDraftIsPending);	/* Wait until the LCD displays the draft framebuffer */

      copy_framebuffer((const uint32_t*)draft_fb_address, (uint32_t*)final_fb_address, 0, 0, LCDWidth, LCDHeight);

      display_state = SwitchToFinalIsPending;	/* Now we have copied the content to display to final framebuffer, we can perform the switch */

      HAL_DSI_Refresh(&hdsi_eval);

      HAL_Delay(1000);
      count++;
  }
}

void set_active_fb_addr(void* fb) {
    /* Disable DSI Wrapper */
    __HAL_DSI_WRAPPER_DISABLE(&hdsi_eval);
    /* Update LTDC configuration */
    LTDC_LAYER(&hltdc_eval, 0)->CFBAR = (uint32_t)(fb);
    __HAL_LTDC_RELOAD_IMMEDIATE_CONFIG(&hltdc_eval);
    /* Enable DSI Wrapper */
    __HAL_DSI_WRAPPER_ENABLE(&hdsi_eval);
}

extern "C" {
/**
  * @brief  End of Refresh DSI callback.
  * @param  hdsi: pointer to a DSI_HandleTypeDef structure that contains
  *               the configuration information for the DSI.
  * @retval None
  *
  * The blue LED is On when we are displaying the displayed fb, and Off when where are temporarily switching to the pending buffer while copying to the displayed
  */
void HAL_DSI_EndOfRefreshCallback(DSI_HandleTypeDef *hdsi)
{
	if (display_state == SwitchToDraftIsPending) {
		set_active_fb_addr(draft_fb_address);
		display_state = DisplayingDraft;
		BSP_LED_Off(LED4);
	}
	else if (display_state == SwitchToFinalIsPending) {
		set_active_fb_addr(final_fb_address);
		display_state = DisplayingFinal;
		BSP_LED_On(LED4);
	}
}


/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 180000000
  *            HCLK(Hz)                       = 180000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 8000000
  *            PLL_M                          = 8
  *            PLL_N                          = 360
  *            PLL_P                          = 2
  *            PLL_Q                          = 7
  *            PLL_R                          = 6
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 5
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  HAL_StatusTypeDef ret = HAL_OK;

  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();

  /* The voltage scaling allows optimizing the power consumption when the device is 
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
#if defined(USE_STM32469I_DISCO_REVA)
  RCC_OscInitStruct.PLL.PLLM = 25;
#else
  RCC_OscInitStruct.PLL.PLLM = 8;
#endif /* USE_STM32469I_DISCO_REVA */
  RCC_OscInitStruct.PLL.PLLN = 360;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  RCC_OscInitStruct.PLL.PLLR = 6;
  
  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  if(ret != HAL_OK)
  {
    while(1) { ; }
  }
  
  /* Activate the OverDrive to reach the 180 MHz Frequency */  
  ret = HAL_PWREx_EnableOverDrive();
  if(ret != HAL_OK)
  {
    while(1) { ; }
  }
  
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;  
  
  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
  if(ret != HAL_OK)
  {
    while(1) { ; }
  }
}

/**
  * @brief  Initializes the DSI LCD. 
  * The ititialization is done as below:
  *     - DSI PLL ititialization
  *     - DSI ititialization
  *     - LTDC ititialization
  *     - OTM8009A LCD Display IC Driver ititialization
  * @param  None
  * @retval LCD state
  */
static uint8_t LCD_Init(void)
{
  static DSI_PHY_TimerTypeDef PhyTimings;
  static DSI_CmdCfgTypeDef CmdCfg;
  static DSI_LPCmdTypeDef LPCmd;
  static DSI_PLLInitTypeDef dsiPllInit;
  static RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct;
  
  /* Toggle Hardware Reset of the DSI LCD using
  * its XRES signal (active low) */
  BSP_LCD_Reset();
  
  /* Call first MSP Initialize only in case of first initialization
  * This will set IP blocks LTDC, DSI and DMA2D
  * - out of reset
  * - clocked
  * - NVIC IRQ related to IP blocks enabled
  */
  BSP_LCD_MspInit();
  
  /* LCD clock configuration */
  /* PLLSAI_VCO Input = HSE_VALUE/PLL_M = 1 Mhz */
  /* PLLSAI_VCO Output = PLLSAI_VCO Input * PLLSAIN = 417 Mhz */
  /* PLLLCDCLK = PLLSAI_VCO Output/PLLSAIR = 417 MHz / 5 = 83.4 MHz */
  /* LTDC clock frequency = PLLLCDCLK / LTDC_PLLSAI_DIVR_2 = 83.4 / 2 = 41.7 MHz */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
  PeriphClkInitStruct.PLLSAI.PLLSAIN = 417;
  PeriphClkInitStruct.PLLSAI.PLLSAIR = 5;
  PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_2;
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
  
  /* Base address of DSI Host/Wrapper registers to be set before calling De-Init */
  hdsi_eval.Instance = DSI;
  
  HAL_DSI_DeInit(&(hdsi_eval));

#if defined(USE_STM32469I_DISCO_REVA)  
  dsiPllInit.PLLNDIV  = 100;
  dsiPllInit.PLLIDF   = DSI_PLL_IN_DIV5;
#else
  dsiPllInit.PLLNDIV  = 125;
  dsiPllInit.PLLIDF   = DSI_PLL_IN_DIV2;  
#endif  /* USE_STM32469I_DISCO_REVA */
  dsiPllInit.PLLODF  = DSI_PLL_OUT_DIV1;  

  hdsi_eval.Init.NumberOfLanes = DSI_TWO_DATA_LANES;
  hdsi_eval.Init.TXEscapeCkdiv = 0x4;
  HAL_DSI_Init(&(hdsi_eval), &(dsiPllInit));
    
  /* Configure the DSI for Command mode */
  CmdCfg.VirtualChannelID      = 0;
  CmdCfg.HSPolarity            = DSI_HSYNC_ACTIVE_HIGH;
  CmdCfg.VSPolarity            = DSI_VSYNC_ACTIVE_HIGH;
  CmdCfg.DEPolarity            = DSI_DATA_ENABLE_ACTIVE_HIGH;
  CmdCfg.ColorCoding           = DSI_RGB888;
  CmdCfg.CommandSize           = HACT;
  CmdCfg.TearingEffectSource   = DSI_TE_DSILINK;
  CmdCfg.TearingEffectPolarity = DSI_TE_RISING_EDGE;
  CmdCfg.VSyncPol              = DSI_VSYNC_FALLING;
  CmdCfg.AutomaticRefresh      = DSI_AR_DISABLE;
  CmdCfg.TEAcknowledgeRequest  = DSI_TE_ACKNOWLEDGE_ENABLE;
  HAL_DSI_ConfigAdaptedCommandMode(&hdsi_eval, &CmdCfg);
  
  LPCmd.LPGenShortWriteNoP    = DSI_LP_GSW0P_ENABLE;
  LPCmd.LPGenShortWriteOneP   = DSI_LP_GSW1P_ENABLE;
  LPCmd.LPGenShortWriteTwoP   = DSI_LP_GSW2P_ENABLE;
  LPCmd.LPGenShortReadNoP     = DSI_LP_GSR0P_ENABLE;
  LPCmd.LPGenShortReadOneP    = DSI_LP_GSR1P_ENABLE;
  LPCmd.LPGenShortReadTwoP    = DSI_LP_GSR2P_ENABLE;
  LPCmd.LPGenLongWrite        = DSI_LP_GLW_ENABLE;
  LPCmd.LPDcsShortWriteNoP    = DSI_LP_DSW0P_ENABLE;
  LPCmd.LPDcsShortWriteOneP   = DSI_LP_DSW1P_ENABLE;
  LPCmd.LPDcsShortReadNoP     = DSI_LP_DSR0P_ENABLE;
  LPCmd.LPDcsLongWrite        = DSI_LP_DLW_ENABLE;
  HAL_DSI_ConfigCommand(&hdsi_eval, &LPCmd);

  /* Configure DSI PHY HS2LP and LP2HS timings */
  PhyTimings.ClockLaneHS2LPTime = 35;
  PhyTimings.ClockLaneLP2HSTime = 35;
  PhyTimings.DataLaneHS2LPTime = 35;
  PhyTimings.DataLaneLP2HSTime = 35;
  PhyTimings.DataLaneMaxReadTime = 0;
  PhyTimings.StopWaitTime = 10;
  HAL_DSI_ConfigPhyTimer(&hdsi_eval, &PhyTimings);

  /* Initialize LTDC */
  LTDC_Init();
  
  /* Start DSI */
  HAL_DSI_Start(&(hdsi_eval));
    
#if defined (USE_STM32469I_DISCO_REVC)
  /* Initialize the NT35510 LCD Display IC Driver (3K138 LCD IC Driver) */
  NT35510_Init(NT35510_FORMAT_RGB888, LCD_ORIENTATION_LANDSCAPE);
#else
  /* Initialize the OTM8009A LCD Display IC Driver (KoD LCD IC Driver) */
  OTM8009A_Init(OTM8009A_COLMOD_RGB888, LCD_ORIENTATION_LANDSCAPE);
#endif
  
  LPCmd.LPGenShortWriteNoP    = DSI_LP_GSW0P_DISABLE;
  LPCmd.LPGenShortWriteOneP   = DSI_LP_GSW1P_DISABLE;
  LPCmd.LPGenShortWriteTwoP   = DSI_LP_GSW2P_DISABLE;
  LPCmd.LPGenShortReadNoP     = DSI_LP_GSR0P_DISABLE;
  LPCmd.LPGenShortReadOneP    = DSI_LP_GSR1P_DISABLE;
  LPCmd.LPGenShortReadTwoP    = DSI_LP_GSR2P_DISABLE;
  LPCmd.LPGenLongWrite        = DSI_LP_GLW_DISABLE;
  LPCmd.LPDcsShortWriteNoP    = DSI_LP_DSW0P_DISABLE;
  LPCmd.LPDcsShortWriteOneP   = DSI_LP_DSW1P_DISABLE;
  LPCmd.LPDcsShortReadNoP     = DSI_LP_DSR0P_DISABLE;
  LPCmd.LPDcsLongWrite        = DSI_LP_DLW_DISABLE;
  HAL_DSI_ConfigCommand(&hdsi_eval, &LPCmd);
  
   HAL_DSI_ConfigFlowControl(&hdsi_eval, DSI_FLOW_CONTROL_BTA);
  /* Refresh the display */
  HAL_DSI_Refresh(&hdsi_eval);
  
  return LCD_OK; 
}

/**
  * @brief  
  * @param  None
  * @retval None
  */
void LTDC_Init(void)
{
  /* DeInit */
  hltdc_eval.Instance = LTDC;
  HAL_LTDC_DeInit(&hltdc_eval);
  
  /* LTDC Config */
  /* Timing and polarity */
  hltdc_eval.Init.HorizontalSync = HSYNC;
  hltdc_eval.Init.VerticalSync = VSYNC;
  hltdc_eval.Init.AccumulatedHBP = HSYNC+HBP;
  hltdc_eval.Init.AccumulatedVBP = VSYNC+VBP;
  hltdc_eval.Init.AccumulatedActiveH = VSYNC+VBP+VACT;
  hltdc_eval.Init.AccumulatedActiveW = HSYNC+HBP+HACT;
  hltdc_eval.Init.TotalHeigh = VSYNC+VBP+VACT+VFP;
  hltdc_eval.Init.TotalWidth = HSYNC+HBP+HACT+HFP;
  
  /* background value */
  hltdc_eval.Init.Backcolor.Blue = 0;
  hltdc_eval.Init.Backcolor.Green = 0;
  hltdc_eval.Init.Backcolor.Red = 0;
  
  /* Polarity */
  hltdc_eval.Init.HSPolarity = LTDC_HSPOLARITY_AL;
  hltdc_eval.Init.VSPolarity = LTDC_VSPOLARITY_AL;
  hltdc_eval.Init.DEPolarity = LTDC_DEPOLARITY_AL;
  hltdc_eval.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
  hltdc_eval.Instance = LTDC;

  HAL_LTDC_Init(&hltdc_eval);
}
}

extern "C" {
/**
  * @brief  Converts a line to an ARGB8888 pixel format.
  * @param  pSrc: Pointer to source buffer
  * @param  pDst: Output color
  * @param  xSize: Buffer width
  * @param  ColorMode: Input color mode   
  * @retval None
  */
static void copy_framebuffer(const uint32_t *pSrc, uint32_t *pDst, uint16_t x, uint16_t y, uint16_t xsize, uint16_t ysize)
{   
  //const uint32_t* pSrc = static_cast<const uint32_t*>(src);
  //uint32_t* pDst = static_cast<uint32_t*>(dst);
  uint32_t destination = (uint32_t)pDst + (y * LCDWidth + x) * 4;
  uint32_t source      = (uint32_t)pSrc;
  
  /*##-1- Configure the DMA2D Mode, Color Mode and output offset #############*/ 
  hdma2d.Init.Mode         = DMA2D_M2M;
  hdma2d.Init.ColorMode    = DMA2D_ARGB8888;
  hdma2d.Init.OutputOffset = LCDWidth - xsize;
  
  /*##-2- DMA2D Callbacks Configuration ######################################*/
  hdma2d.XferCpltCallback  = NULL;
  
  /*##-3- Foreground Configuration ###########################################*/
  hdma2d.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
  hdma2d.LayerCfg[1].InputAlpha = 0xFF;
  hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_ARGB8888;
  hdma2d.LayerCfg[1].InputOffset = 0;

  hdma2d.Instance          = DMA2D; 
   
  /* DMA2D Initialization */
  if(HAL_DMA2D_Init(&hdma2d) == HAL_OK) 
  {
    if(HAL_DMA2D_ConfigLayer(&hdma2d, 1) == HAL_OK) 
    {
      if (HAL_DMA2D_Start(&hdma2d, source, destination, xsize, ysize) == HAL_OK)
      {
        /* Polling For DMA transfer */  
        HAL_DMA2D_PollForTransfer(&hdma2d, 100);
      }
    }
  }   
}
}
#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif