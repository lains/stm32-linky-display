#include "Stm32LcdDriver.h"
extern "C" {
#include "main.h"
#include "stm32469i_discovery_lcd.h"

extern LTDC_HandleTypeDef hltdc_eval; // Imported definition from stm32469i_discovery_lcd.c
extern DSI_HandleTypeDef hdsi_eval; // Imported definition from stm32469i_discovery_lcd.c
}

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

void* const Stm32LcdDriver::draftFramebuffer = (void *)LCD_FB_START_ADDRESS;
void* const Stm32LcdDriver::finalFramebuffer = (void *)((uint8_t*)Stm32LcdDriver::draftFramebuffer + LCDWidth*LCDHeight*BytesPerPixel); // Final framebuffer directly follows draft framebuffer
 
//static uint32_t ImageIndex = 0;
/*static const uint32_t * Images[] = 
{
    image_320x240_argb8888,
    life_augmented_argb8888,
};*/

/**
 * @brief Set the currently active (displayed) framebuffer
 * 
 * @note When returing from this function, the framebuffer is not yet displayed on the LCD
 *       When the display is efefctive, callback HAL_DSI_EndOfRefreshCallback() will be invoked
 * @param fb A pointer to the framebuffer to display
 */
void set_active_fb(void* fb) {
    /* Disable DSI Wrapper */
    __HAL_DSI_WRAPPER_DISABLE(get_hdsi());
    /* Update LTDC configuration */
    LTDC_LAYER(get_hltdc(), 0)->CFBAR = (uint32_t)(fb);
    __HAL_LTDC_RELOAD_IMMEDIATE_CONFIG(get_hltdc());
    /* Enable DSI Wrapper */
    __HAL_DSI_WRAPPER_ENABLE(get_hdsi());
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
void HAL_DSI_EndOfRefreshCallback(DSI_HandleTypeDef *hdsi) {
    if (Stm32LcdDriver::get().displayState == Stm32LcdDriver::SwitchToDraftIsPending) {
        set_active_fb(Stm32LcdDriver::get().draftFramebuffer);
        Stm32LcdDriver::get().displayState = Stm32LcdDriver::DisplayingDraft;
        BSP_LED_Off(LED4);
    }
    else if (Stm32LcdDriver::get().displayState == Stm32LcdDriver::SwitchToFinalIsPending) {
        set_active_fb(Stm32LcdDriver::get().finalFramebuffer);
        Stm32LcdDriver::get().displayState = Stm32LcdDriver::DisplayingFinal;
        BSP_LED_On(LED4);
    }
}

/**
  * @brief  
  * @param  None
  * @retval None
  */
void LTDC_Init(LTDC_HandleTypeDef* hltdc) {
    /* DeInit */
    hltdc->Instance = LTDC;
    HAL_LTDC_DeInit(hltdc);

    /* LTDC Config */
    /* Timing and polarity */
    hltdc->Init.HorizontalSync = HSYNC;
    hltdc->Init.VerticalSync = VSYNC;
    hltdc->Init.AccumulatedHBP = HSYNC+HBP;
    hltdc->Init.AccumulatedVBP = VSYNC+VBP;
    hltdc->Init.AccumulatedActiveH = VSYNC+VBP+VACT;
    hltdc->Init.AccumulatedActiveW = HSYNC+HBP+HACT;
    hltdc->Init.TotalHeigh = VSYNC+VBP+VACT+VFP;
    hltdc->Init.TotalWidth = HSYNC+HBP+HACT+HFP;

    /* background value */
    hltdc->Init.Backcolor.Blue = 0;
    hltdc->Init.Backcolor.Green = 0;
    hltdc->Init.Backcolor.Red = 0;

    /* Polarity */
    hltdc->Init.HSPolarity = LTDC_HSPOLARITY_AL;
    hltdc->Init.VSPolarity = LTDC_VSPOLARITY_AL;
    hltdc->Init.DEPolarity = LTDC_DEPOLARITY_AL;
    hltdc->Init.PCPolarity = LTDC_PCPOLARITY_IPC;
    hltdc->Instance = LTDC;

    HAL_LTDC_Init(hltdc);
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
static uint8_t LCD_Init(DSI_HandleTypeDef* hdsi, LTDC_HandleTypeDef* hltdc) {
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
    hdsi->Instance = DSI;

    HAL_DSI_DeInit(hdsi);

#if defined(USE_STM32469I_DISCO_REVA)  
    dsiPllInit.PLLNDIV  = 100;
    dsiPllInit.PLLIDF   = DSI_PLL_IN_DIV5;
#else
    dsiPllInit.PLLNDIV  = 125;
    dsiPllInit.PLLIDF   = DSI_PLL_IN_DIV2;
#endif  /* USE_STM32469I_DISCO_REVA */
    dsiPllInit.PLLODF  = DSI_PLL_OUT_DIV1;

    hdsi->Init.NumberOfLanes = DSI_TWO_DATA_LANES;
    hdsi->Init.TXEscapeCkdiv = 0x4;
    HAL_DSI_Init(hdsi, &(dsiPllInit));

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
    HAL_DSI_ConfigAdaptedCommandMode(hdsi, &CmdCfg);
    
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
    HAL_DSI_ConfigCommand(hdsi, &LPCmd);

    /* Configure DSI PHY HS2LP and LP2HS timings */
    PhyTimings.ClockLaneHS2LPTime = 35;
    PhyTimings.ClockLaneLP2HSTime = 35;
    PhyTimings.DataLaneHS2LPTime = 35;
    PhyTimings.DataLaneLP2HSTime = 35;
    PhyTimings.DataLaneMaxReadTime = 0;
    PhyTimings.StopWaitTime = 10;
    HAL_DSI_ConfigPhyTimer(hdsi, &PhyTimings);

    /* Initialize LTDC */
    LTDC_Init(hltdc);
    
    /* Start DSI */
    HAL_DSI_Start(hdsi);

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
    HAL_DSI_ConfigCommand(hdsi, &LPCmd);

    HAL_DSI_ConfigFlowControl(hdsi, DSI_FLOW_CONTROL_BTA);
    /* Refresh the display */
    HAL_DSI_Refresh(hdsi);

    return LCD_OK;
}

} // extern "C"


Stm32LcdDriver::Stm32LcdDriver() :
displayState(SwitchToDraftIsPending),
hltdc(hltdc_eval),
hdsi(hdsi_eval)
{
}

Stm32LcdDriver Stm32LcdDriver::instance=Stm32LcdDriver();


Stm32LcdDriver::~Stm32LcdDriver() {
}

Stm32LcdDriver& Stm32LcdDriver::get() {
    return Stm32LcdDriver::instance;
}

bool Stm32LcdDriver::start() {
    if (!LCD_Init(&(this->hdsi), &(this->hltdc)) == LCD_OK)
        return false;
    
    BSP_LCD_LayerDefaultInit(0, (uint32_t)(this->draftFramebuffer));
    BSP_LCD_SelectLayer(0); 

    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_FillRect(0, 0, LCDWidth, LCDHeight);
    BSP_LCD_SetTextColor(LCD_COLOR_RED);
    BSP_LCD_FillCircle(30, 30, 30);

    this->displayState = SwitchToDraftIsPending;

    HAL_DSI_Refresh(&(this->hdsi));

    while (this->displayState == SwitchToDraftIsPending);	/* Wait until the LCD displays the draft framebuffer */

    this->copyDraftToFinal();

    this->requestDisplayFinal();

    return true;
}

void Stm32LcdDriver::requestDisplayDraft() {
    this->displayState = SwitchToDraftIsPending;

    HAL_DSI_Refresh(&(this->hdsi));
}

void Stm32LcdDriver::waitForDraftDisplayed(FWaitForDisplayRefreshFunc toRunWhileWaiting, void* context) const {
    while (this->displayState != DisplayingDraft) {
        if (toRunWhileWaiting != nullptr) {
            toRunWhileWaiting(context);
        }
    }	/* Wait until the LCD displays the draft framebuffer */
}

void Stm32LcdDriver::requestDisplayFinal() {
    this->displayState = SwitchToFinalIsPending;

    HAL_DSI_Refresh(&(this->hdsi));
}

void Stm32LcdDriver::waitForFinalDisplayed(FWaitForDisplayRefreshFunc toRunWhileWaiting, void* context) const {
    while (this->displayState != DisplayingFinal) {
        if (toRunWhileWaiting != nullptr) {
            toRunWhileWaiting(context);
        }
    }	/* Wait until the LCD displays the finale framebuffer */
}

void Stm32LcdDriver::displayDraft(FWaitForDisplayRefreshFunc toRunWhileWaiting, void* context) {
    this->requestDisplayDraft();
    this->waitForDraftDisplayed(toRunWhileWaiting);
}

void Stm32LcdDriver::copyDraftToFinal() {
    this->hdma2dCopyFramebuffer(this->draftFramebuffer, this->finalFramebuffer, 0, 0, LCDWidth, LCDHeight);
}

void Stm32LcdDriver::hdma2dCopyFramebuffer(const void* src, void* dst, uint16_t x, uint16_t y, uint16_t xsize, uint16_t ysize) {
    uint32_t destination_addr = (uint32_t)dst + (y * LCDWidth + x) * 4;
    uint32_t source_addr      = (uint32_t)src;

    /*##-1- Configure the DMA2D Mode, Color Mode and output offset #############*/
    this->hdma2d.Init.Mode         = DMA2D_M2M;
    this->hdma2d.Init.ColorMode    = DMA2D_ARGB8888;
    this->hdma2d.Init.OutputOffset = LCDWidth - xsize;

    /*##-2- DMA2D Callbacks Configuration ######################################*/
    this->hdma2d.XferCpltCallback  = NULL;

    /*##-3- Foreground Configuration ###########################################*/
    this->hdma2d.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
    this->hdma2d.LayerCfg[1].InputAlpha = 0xFF;
    this->hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_ARGB8888;
    this->hdma2d.LayerCfg[1].InputOffset = 0;

    this->hdma2d.Instance          = DMA2D;

    /* DMA2D Initialization */
    if (HAL_DMA2D_Init(&(this->hdma2d)) == HAL_OK) {
        if (HAL_DMA2D_ConfigLayer(&(this->hdma2d), 1) == HAL_OK) {
            if (HAL_DMA2D_Start(&(this->hdma2d), source_addr, destination_addr, xsize, ysize) == HAL_OK) {
                /* Polling For DMA transfer */
                HAL_DMA2D_PollForTransfer(&(this->hdma2d), 100);
#if 0
    /* STM32's instructions could allow us to switch to interrupt mode and continue forwarding incoming bytes to the tic unframer even during DMA2D transfers
    *** Polling mode IO operation ***
     =================================
    [..]
       (#) Configure pdata parameter (explained hereafter), destination and data length
           and enable the transfer using HAL_DMA2D_Start().
       (#) Wait for end of transfer using HAL_DMA2D_PollForTransfer(), at this stage
           user can specify the value of timeout according to his end application.

     *** Interrupt mode IO operation ***
     ===================================
     [..]
       (#) Configure pdata parameter, destination and data length and enable
           the transfer using HAL_DMA2D_Start_IT().
       (#) Use HAL_DMA2D_IRQHandler() called under DMA2D_IRQHandler() interrupt subroutine.
       (#) At the end of data transfer HAL_DMA2D_IRQHandler() function is executed and user can
           add his own function by customization of function pointer XferCpltCallback (member
           of DMA2D handle structure).
       (#) In case of error, the HAL_DMA2D_IRQHandler() function calls the callback
           XferErrorCallback.

         -@-   In Register-to-Memory transfer mode, pdata parameter is the register
               color, in Memory-to-memory or Memory-to-Memory with pixel format
               conversion pdata is the source address.

         -@-   Configure the foreground source address, the background source address,
               the destination and data length then Enable the transfer using
               HAL_DMA2D_BlendingStart() in polling mode and HAL_DMA2D_BlendingStart_IT()
               in interrupt mode.

         -@-   HAL_DMA2D_BlendingStart() and HAL_DMA2D_BlendingStart_IT() functions
               are used if the memory to memory with blending transfer mode is selected.

      (#) Optionally, configure and enable the CLUT using HAL_DMA2D_CLUTLoad() in polling
          mode or HAL_DMA2D_CLUTLoad_IT() in interrupt mode.

      (#) Optionally, configure the line watermark in using the API HAL_DMA2D_ProgramLineEvent().

      (#) Optionally, configure the dead time value in the AHB clock cycle inserted between two
          consecutive accesses on the AHB master port in using the API HAL_DMA2D_ConfigDeadTime()
          and enable/disable the functionality  with the APIs HAL_DMA2D_EnableDeadTime() or
          HAL_DMA2D_DisableDeadTime().

      (#) The transfer can be suspended, resumed and aborted using the following
          functions: HAL_DMA2D_Suspend(), HAL_DMA2D_Resume(), HAL_DMA2D_Abort().

      (#) The CLUT loading can be suspended, resumed and aborted using the following
          functions: HAL_DMA2D_CLUTLoading_Suspend(), HAL_DMA2D_CLUTLoading_Resume(),
          HAL_DMA2D_CLUTLoading_Abort().

      (#) To control the DMA2D state, use the following function: HAL_DMA2D_GetState().

      (#) To read the DMA2D error code, use the following function: HAL_DMA2D_GetError().
      */
#endif
            }
        }
    }
}

LTDC_HandleTypeDef* getLcdLtdcHandle() {
    return &(Stm32LcdDriver::get().hltdc);
}

DSI_HandleTypeDef* getLcdDsiHandle() {
    return &(Stm32LcdDriver::get().hdsi);
}

/* FIXME: remove before flight... only for debug */
extern "C" {
LTDC_HandleTypeDef* get_hltdc() {
    return getLcdLtdcHandle();
}

DSI_HandleTypeDef* get_hdsi() {
    return getLcdDsiHandle();
}
} // extern "C"

