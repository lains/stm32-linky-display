#include "Stm32LcdDriver.h"
extern "C" {
#include "main.h"
#include "stm32f769i_discovery.h"
#include "stm32f769i_discovery_lcd.h"

extern LTDC_HandleTypeDef hltdc_discovery; // Imported definition from stm32f769i_discovery_lcd.c
extern DSI_HandleTypeDef hdsi_discovery; // Imported definition from stm32f769i_discovery_lcd.c
static DSI_VidCfgTypeDef hdsivideo_handle;
}

const unsigned int LCDWidth = 800;
const unsigned int LCDHeight = 480;
const unsigned int BytesPerPixel = 4; /* For ARGB8888 mode */

#define VACT                LCDHeight
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
 * @note When returning from this function, the framebuffer is not yet displayed on the LCD
 *       When the display is efefctive, callback HAL_DSI_EndOfRefreshCallback() will be invoked
 * @param fb A pointer to the framebuffer to display
 */
void set_active_fb(void* fb) {
    /* Disable DSI Wrapper */
    __HAL_DSI_WRAPPER_DISABLE(getLcdDsiHandle());
    /* Update LTDC configuration */
    LTDC_LAYER(getLcdLtdcHandle(), 0)->CFBAR = (uint32_t)(fb);
    __HAL_LTDC_RELOAD_IMMEDIATE_CONFIG(getLcdLtdcHandle());
    /* Enable DSI Wrapper */
    __HAL_DSI_WRAPPER_ENABLE(getLcdDsiHandle());
}

extern "C" {
/**
  * @brief  End of Refresh DSI callback.
  * @param  hdsi: pointer to a DSI_HandleTypeDef structure that contains
  *               the configuration information for the DSI.
  * The blue LED is On when we are displaying the displayed fb, and Off when where are temporarily switching to the pending buffer while copying to the displayed
  */
void HAL_DSI_EndOfRefreshCallback(DSI_HandleTypeDef *hdsi) {
    if (Stm32LcdDriver::get().displayState == Stm32LcdDriver::SwitchToDraftIsPending) {
        set_active_fb(Stm32LcdDriver::get().draftFramebuffer);
        Stm32LcdDriver::get().displayState = Stm32LcdDriver::DisplayingDraft;
        BSP_LED_Off(LED1);
    }
    else if (Stm32LcdDriver::get().displayState == Stm32LcdDriver::SwitchToFinalIsPending) {
        set_active_fb(Stm32LcdDriver::get().finalFramebuffer);
        Stm32LcdDriver::get().displayState = Stm32LcdDriver::DisplayingFinal;
        BSP_LED_On(LED1);
    }
}

/**
  * @brief  
  * @param  None
  * @retval None
  */
void LTDC_Init(LTDC_HandleTypeDef* hltdc) {
    // FIXME move LTDC-related code from LCD_Init into this function
}

#if defined(USE_LCD_HDMI)
#define HDMI_ASPECT_RATIO_16_9  ADV7533_ASPECT_RATIO_16_9
#define HDMI_ASPECT_RATIO_4_3   ADV7533_ASPECT_RATIO_4_3
#endif /* USE_LCD_HDMI */    
#define LCD_DSI_ID              0x11
#define LCD_DSI_ID_REG          0xA8

/**
  * @brief  Returns the ID of connected screen by checking the HDMI
  *        (adv7533 component) ID or LCD DSI (via TS ID) ID.
  * @retval LCD ID
  */
static uint16_t LCD_IO_GetID(void)
{ 
#if defined(USE_LCD_HDMI)  
  HDMI_IO_Init();
  
  HDMI_IO_Delay(120);
  
  if(ADV7533_ID == adv7533_drv.ReadID(ADV7533_CEC_DSI_I2C_ADDR))
  {
    return ADV7533_ID;
  }  
  else if(((HDMI_IO_Read(LCD_DSI_ADDRESS, LCD_DSI_ID_REG) == LCD_DSI_ID)) || \
           (HDMI_IO_Read(LCD_DSI_ADDRESS_A02, LCD_DSI_ID_REG) == LCD_DSI_ID))
  {
    return LCD_DSI_ID;
  }
  else
  {
    return 0;
  }
#else 
  return LCD_DSI_ID; 
#endif /* USE_LCD_HDMI */
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
    OnError_Handler(BSP_LCD_Init() != LCD_OK);

    return LCD_OK; /* Delegated to original BSP code for now */
  LCD_OrientationTypeDef orientation = LCD_ORIENTATION_LANDSCAPE;
  DSI_PLLInitTypeDef dsiPllInit;
  static RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct;
  uint32_t LcdClock  = 27429; /*!< LcdClk = 27429 kHz */
  uint16_t read_id = 0;

  uint32_t laneByteClk_kHz = 0;
  uint32_t                   VSA; /*!< Vertical start active time in units of lines */
  uint32_t                   VBP; /*!< Vertical Back Porch time in units of lines */
  uint32_t                   VFP; /*!< Vertical Front Porch time in units of lines */
  uint32_t                   VACT; /*!< Vertical Active time in units of lines = imageSize Y in pixels to display */
  uint32_t                   HSA; /*!< Horizontal start active time in units of lcdClk */
  uint32_t                   HBP; /*!< Horizontal Back Porch time in units of lcdClk */
  uint32_t                   HFP; /*!< Horizontal Front Porch time in units of lcdClk */
  uint32_t                   HACT; /*!< Horizontal Active time in units of lcdClk = imageSize X in pixels to display */

  /* Toggle Hardware Reset of the DSI LCD using
  * its XRES signal (active low) */
  BSP_LCD_Reset();

  /* Check the connected monitor */
  read_id = LCD_IO_GetID();

#if defined(USE_LCD_HDMI)   
  if(read_id == ADV7533_ID)
  {
    return BSP_LCD_HDMIInitEx(HDMI_FORMAT_720_576); 
  }  
  else if(read_id != LCD_DSI_ID)
  {
    return LCD_ERROR;  
  }
#else
  if(read_id != LCD_DSI_ID)
  {
    return LCD_ERROR;  
  }  
#endif /* USE_LCD_HDMI */ 

  /* Call first MSP Initialize only in case of first initialization
  * This will set IP blocks LTDC, DSI and DMA2D
  * - out of reset
  * - clocked
  * - NVIC IRQ related to IP blocks enabled
  */
  BSP_LCD_MspInit();

/*************************DSI Initialization***********************************/  

  /* Base address of DSI Host/Wrapper registers to be set before calling De-Init */
  hdsi->Instance = DSI;

  HAL_DSI_DeInit(hdsi);

  dsiPllInit.PLLNDIV  = 100;
  dsiPllInit.PLLIDF   = DSI_PLL_IN_DIV5;
  dsiPllInit.PLLODF  = DSI_PLL_OUT_DIV1;
  laneByteClk_kHz = 62500; /* 500 MHz / 8 = 62.5 MHz = 62500 kHz */

  /* Set number of Lanes */
  hdsi->Init.NumberOfLanes = DSI_TWO_DATA_LANES;

  /* TXEscapeCkdiv = f(LaneByteClk)/15.62 = 4 */
  hdsi->Init.TXEscapeCkdiv = laneByteClk_kHz/15620; 

  HAL_DSI_Init(hdsi, &(dsiPllInit));

  /* Timing parameters for all Video modes
  * Set Timing parameters of LTDC depending on its chosen orientation
  */

  /* The following values are same for portrait and landscape orientations */
#if defined (USE_STM32F769I_DISCO_REVB03)
  VSA  = NT35510_480X800_VSYNC;
  VBP  = NT35510_480X800_VBP;
  VFP  = NT35510_480X800_VFP;
  HSA  = NT35510_480X800_HSYNC;
  HBP  = NT35510_480X800_HBP;
  HFP  = NT35510_480X800_HFP;  
#else
  VSA  = OTM8009A_480X800_VSYNC;
  VBP  = OTM8009A_480X800_VBP;
  VFP  = OTM8009A_480X800_VFP;
  HSA  = OTM8009A_480X800_HSYNC;
  HBP  = OTM8009A_480X800_HBP;
  HFP  = OTM8009A_480X800_HFP;
#endif /* USE_STM32F769I_DISCO_REVB03 */

  hdsivideo_handle.VirtualChannelID = LCD_OTM8009A_ID;
  hdsivideo_handle.ColorCoding = LCD_DSI_PIXEL_DATA_FMT_RBG888;
  hdsivideo_handle.VSPolarity = DSI_VSYNC_ACTIVE_HIGH;
  hdsivideo_handle.HSPolarity = DSI_HSYNC_ACTIVE_HIGH;
  hdsivideo_handle.DEPolarity = DSI_DATA_ENABLE_ACTIVE_HIGH;  
  hdsivideo_handle.Mode = DSI_VID_MODE_BURST; /* Mode Video burst ie : one LgP per line */
  hdsivideo_handle.NullPacketSize = 0xFFF;
  hdsivideo_handle.NumberOfChunks = 0;
  hdsivideo_handle.PacketSize                = HACT; /* Value depending on display orientation choice portrait/landscape */ 
  hdsivideo_handle.HorizontalSyncActive      = (HSA * laneByteClk_kHz)/LcdClock;
  hdsivideo_handle.HorizontalBackPorch       = (HBP * laneByteClk_kHz)/LcdClock;
  hdsivideo_handle.HorizontalLine            = ((HACT + HSA + HBP + HFP) * laneByteClk_kHz)/LcdClock; /* Value depending on display orientation choice portrait/landscape */
  hdsivideo_handle.VerticalSyncActive        = VSA;
  hdsivideo_handle.VerticalBackPorch         = VBP;
  hdsivideo_handle.VerticalFrontPorch        = VFP;
  hdsivideo_handle.VerticalActive            = VACT; /* Value depending on display orientation choice portrait/landscape */

  /* Enable or disable sending LP command while streaming is active in video mode */
  hdsivideo_handle.LPCommandEnable = DSI_LP_COMMAND_ENABLE; /* Enable sending commands in mode LP (Low Power) */

  /* Largest packet size possible to transmit in LP mode in VSA, VBP, VFP regions */
  /* Only useful when sending LP packets is allowed while streaming is active in video mode */
  hdsivideo_handle.LPLargestPacketSize = 16;

  /* Largest packet size possible to transmit in LP mode in HFP region during VACT period */
  /* Only useful when sending LP packets is allowed while streaming is active in video mode */
  hdsivideo_handle.LPVACTLargestPacketSize = 0;

  /* Specify for each region of the video frame, if the transmission of command in LP mode is allowed in this region */
  /* while streaming is active in video mode                                                                         */
  hdsivideo_handle.LPHorizontalFrontPorchEnable = DSI_LP_HFP_ENABLE;   /* Allow sending LP commands during HFP period */
  hdsivideo_handle.LPHorizontalBackPorchEnable  = DSI_LP_HBP_ENABLE;   /* Allow sending LP commands during HBP period */
  hdsivideo_handle.LPVerticalActiveEnable = DSI_LP_VACT_ENABLE;  /* Allow sending LP commands during VACT period */
  hdsivideo_handle.LPVerticalFrontPorchEnable = DSI_LP_VFP_ENABLE;   /* Allow sending LP commands during VFP period */
  hdsivideo_handle.LPVerticalBackPorchEnable = DSI_LP_VBP_ENABLE;   /* Allow sending LP commands during VBP period */
  hdsivideo_handle.LPVerticalSyncActiveEnable = DSI_LP_VSYNC_ENABLE; /* Allow sending LP commands during VSync = VSA period */

  /* Configure DSI Video mode timings with settings set above */
  HAL_DSI_ConfigVideoMode(hdsi, &(hdsivideo_handle));

/*************************End DSI Initialization*******************************/ 
  
    /* Initialize LTDC */
    //LTDC_Init(hltdc); //FIXME: we should call our own LTDC_Init() above here instead of directly using the code inserted below

/************************LTDC Initialization***********************************/  

  /* Timing Configuration */    
  hltdc->Init.HorizontalSync = (HSA - 1);
  hltdc->Init.AccumulatedHBP = (HSA + HBP - 1);
  hltdc->Init.AccumulatedActiveW = (LCDWidth+ HSA + HBP - 1);
  hltdc->Init.TotalWidth = (LCDWidth + HSA + HBP + HFP - 1);

  /* Initialize the LCD pixel width and pixel height */
  hltdc->LayerCfg->ImageWidth  = LCDWidth;
  hltdc->LayerCfg->ImageHeight = LCDHeight;   

  /** LCD clock configuration
    * Note: The following values should not be changed as the PLLSAI is also used 
    *      to clock the USB FS
    * PLLSAI_VCO Input = HSE_VALUE/PLL_M = 1 Mhz 
    * PLLSAI_VCO Output = PLLSAI_VCO Input * PLLSAIN = 384 Mhz 
    * PLLLCDCLK = PLLSAI_VCO Output/PLLSAIR = 384 MHz / 7 = 54.85 MHz 
    * LTDC clock frequency = PLLLCDCLK / LTDC_PLLSAI_DIVR_2 = 54.85 MHz / 2 = 27.429 MHz 
    */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
  PeriphClkInitStruct.PLLSAI.PLLSAIN = 384;
  PeriphClkInitStruct.PLLSAI.PLLSAIR = 7;
  PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_2;
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

  /* Background value */
  hltdc->Init.Backcolor.Blue = 0;
  hltdc->Init.Backcolor.Green = 0;
  hltdc->Init.Backcolor.Red = 0;
  hltdc->Init.PCPolarity = LTDC_PCPOLARITY_IPC;
  hltdc->Instance = LTDC;

  /* Get LTDC Configuration from DSI Configuration */
  HAL_LTDC_StructInitFromVideoConfig(hltdc, &(hdsivideo_handle));

  /* Initialize the LTDC */  
  HAL_LTDC_Init(hltdc);

  /* Enable the DSI host and wrapper after the LTDC initialization
     To avoid any synchronization issue, the DSI shall be started after enabling the LTDC */
  HAL_DSI_Start(hdsi);

#if !defined(DATA_IN_ExtSDRAM)
  /* Initialize the SDRAM */
  BSP_SDRAM_Init();
#endif /* DATA_IN_ExtSDRAM */

  /* Initialize the font */
  BSP_LCD_SetFont(&LCD_DEFAULT_FONT);

/************************End LTDC Initialization*******************************/
  
#if defined(USE_STM32F769I_DISCO_REVB03)
/***********************NT35510 Initialization********************************/  
  
  /* Initialize the NT35510 LCD Display IC Driver (TechShine LCD IC Driver)
   * depending on configuration set in 'hdsivideo_handle'.
   */
  NT35510_Init(NT35510_FORMAT_RGB888, orientation);
/***********************End NT35510 Initialization****************************/
#else
  
/***********************OTM8009A Initialization********************************/ 

  /* Initialize the OTM8009A LCD Display IC Driver (KoD LCD IC Driver)
  *  depending on configuration set in 'hdsivideo_handle'.
  */
  OTM8009A_Init(OTM8009A_FORMAT_RGB888, LCD_ORIENTATION_LANDSCAPE);

/***********************End OTM8009A Initialization****************************/ 
#endif /* USE_STM32F769I_DISCO_REVB03 */


  return LCD_OK; 
}

} // extern "C"


Stm32LcdDriver::Stm32LcdDriver() :
displayState(SwitchToDraftIsPending),
hltdc(hltdc_discovery),
hdsi(hdsi_discovery)
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

    this->fillRect(0, 0, this->getWidth(), this->getHeight(), LCD_Color::White);
    BSP_LCD_SetTextColor(LCD_COLOR_RED);
    BSP_LCD_FillCircle(30, 30, 30);

    this->displayState = SwitchToDraftIsPending;

    HAL_DSI_Refresh(&(this->hdsi));
    BSP_LED_On(LED_RED);

    // Currently hanging here...
    while (this->displayState == SwitchToDraftIsPending);	/* Wait until the LCD displays the draft framebuffer */
    HAL_Delay(250);
    BSP_LED_Off(LED_RED);
    BSP_LED_On(LED_GREEN);
    HAL_Delay(250);
    BSP_LED_Off(LED_GREEN);

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

uint16_t Stm32LcdDriver::getWidth() const {
    return LCDWidth;
}

uint16_t Stm32LcdDriver::getHeight() const {
    return LCDHeight;
}

void Stm32LcdDriver::drawVerticalLine(uint16_t x, uint16_t y, uint16_t yPlus, LCD_Color color, LCD_Color alternateColor, uint8_t alternateRatio) {
#if 1
    if (x >= this->getWidth() || y >= this->getHeight() )
        return;
    if (alternateColor == LCD_Color::None || alternateRatio == 0) { /* For solid lines, use the DMA2D and draw using a flattened rectangle */
        if (color == LCD_Color::Transparent || yPlus == 0)
            return;
        this->fillRect(x, y, 1, yPlus, color);
    }
    else {
        for (unsigned int yPos = y; yPos < y+yPlus; yPos++) {
            if (yPos >= this->getHeight())
                return;
            LCD_Color pixColor = color;
            if (alternateColor != LCD_Color::None && alternateRatio != 0) {
                if ((x + yPos) % (alternateRatio + 1) != 0)
                    pixColor = alternateColor;
            }
            if (pixColor != LCD_Color::Transparent)
                *(volatile uint32_t*) (static_cast<uint8_t*>(this->draftFramebuffer) + (4*(yPos*this->getWidth() + x))) = pixColor; // equivalent to BSP_LCD_DrawPixel(x, yPos, pixColor);
        }
    }
#else
    uint32_t *topPixelPtr = static_cast<uint32_t*>(this->draftFramebuffer) + (this->getWidth()*y + x); /* Because we are using a uint32_t*, offset will shift the address by 32bits per pixels */
    LL_FillBuffer(1, topPixelPtr, 1, yPlus, (this->getWidth() - 1), color);
#endif
}

void Stm32LcdDriver::drawHorizontalLine(uint16_t x, uint16_t y, uint16_t xPlus, LCD_Color color, LCD_Color alternateColor, uint8_t alternateRatio) {
    if (x >= this->getWidth() || y >= this->getHeight() )
        return;
    if (alternateColor == LCD_Color::None && alternateRatio != 0) { /* For solid lines, use the DMA2D and draw using a flattened rectangle */
        if (color == LCD_Color::Transparent || xPlus == 0)
            return;
        this->fillRect(x, y, xPlus, 1, color);
    }
    else {
        for (unsigned int xPos = x; xPos < x+xPlus; xPos++) {
            if (xPos >= this->getWidth())
                return;
            LCD_Color pixColor = color;
            if (alternateColor != LCD_Color::None && alternateRatio != 0) {
                if ((xPos + y) % (alternateRatio + 1) != 0)
                    pixColor = alternateColor;
            }
            if (pixColor != LCD_Color::Transparent)
                *(volatile uint32_t*) (static_cast<uint8_t*>(this->draftFramebuffer) + (4*(y*this->getWidth() + xPos))) = pixColor; // equivalent to BSP_LCD_DrawPixel(xPos, y, pixColor);
        }
    }
}

void Stm32LcdDriver::drawGlyph(uint16_t x, uint16_t y, const uint8_t *c, unsigned int fontWidth, unsigned int fontHeight, LCD_Color fgColor, LCD_Color bgColor) {
    const uint8_t* glyphDefByte;

    for (unsigned int i = 0; i < fontHeight; i++) {
        for (unsigned int j = 0; j < fontWidth; j++) {
            glyphDefByte = (c + (fontWidth + 7)/8 * i); /* Get first byte at the i for glyph */
            glyphDefByte += j / 8; /* Move forward if the x pos is not in the first byte */
            if (*glyphDefByte & (1 << (7 - (j % 8)))) {
                BSP_LCD_DrawPixel((x + j), (y + i), fgColor);
            } else if (bgColor != LCD_Color::Transparent) {
                BSP_LCD_DrawPixel((x + j), (y + i), bgColor);
            }
        }
    }
}

void Stm32LcdDriver::drawText(uint16_t x, uint16_t y, const char *text, unsigned int fontWidth, unsigned int fontHeight, FCharacterToGlyphPtr charToGlyphFunc, LCD_Color fgColor, LCD_Color bgColor) {
    unsigned int xOriginGlyph = x;
    while ((*text != '\0') && (xOriginGlyph + fontWidth < this->getWidth())) {
        this->drawGlyph(xOriginGlyph, y, charToGlyphFunc(*text), fontWidth, fontHeight, fgColor, bgColor);
        xOriginGlyph += fontWidth;
        text++;
    }
}

void Stm32LcdDriver::fillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, LCD_Color color) {
    /* See https://medium.com/siili-automotive/optimizing-rendering-performance-on-stm32-using-dma2d-5885869de2b5 for good examples on how to draw using DMA2D */

    if (width == 0 || height == 0)
        return; /* Empty rectangle */
    if (x > this->getWidth() || y > this->getHeight())
        return; /* Origin is outside of the display area */

    
    this->hdma2d.Init.Mode         = DMA2D_R2M;
    this->hdma2d.Init.ColorMode    = DMA2D_OUTPUT_ARGB8888;
    this->hdma2d.Init.OutputOffset = LCDWidth - width;

    this->hdma2d.XferCpltCallback  = NULL;

    // this->hdma2d.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
    // this->hdma2d.LayerCfg[1].InputAlpha = 0xFF;
    // this->hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_ARGB8888;
    // this->hdma2d.LayerCfg[1].InputOffset = 0;

    if (x + width >= this->getWidth())
        width = this->getWidth() - x - 1; /* Clip to display right border */
    if (y + height >= this->getHeight())
        height = this->getHeight() - y - 1; /* Clip to display left border */

    if (HAL_DMA2D_Init(&(this->hdma2d)) == HAL_OK) {
        // if (HAL_DMA2D_ConfigLayer(&(this->hdma2d), 1) == HAL_OK) {
            if (HAL_DMA2D_Start(&(this->hdma2d), (uint32_t)color, (uint32_t)(this->draftFramebuffer) + (x + y*LCDWidth) * BytesPerPixel, width, height) == HAL_OK) {
                /* Polling For DMA transfer */
                HAL_DMA2D_PollForTransfer(&(this->hdma2d), 50);
            }
        // }
    }

    // BSP_LCD_SetTextColor(color);
    // for (unsigned int i = 0; i < height; i++) {
    //     BSP_LCD_DrawHLine(x, y + i, width);
    // }
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

void Stm32LcdDriver::LL_FillBuffer(uint32_t LayerIndex, void *pDst, uint32_t xSize, uint32_t ySize, uint32_t OffLine, uint32_t ColorIndex) {
    /* Register to memory mode with ARGB8888 as color Mode */
    this->hdma2d.Init.Mode         = DMA2D_R2M;
    this->hdma2d.Init.ColorMode    = DMA2D_ARGB8888;
    this->hdma2d.Init.OutputOffset = OffLine;

    this->hdma2d.Instance = DMA2D;

    /* DMA2D Initialization */
    if (HAL_DMA2D_Init(&(this->hdma2d)) == HAL_OK) {
        if (HAL_DMA2D_ConfigLayer(&(this->hdma2d), LayerIndex) == HAL_OK) {
            if (HAL_DMA2D_Start(&(this->hdma2d), ColorIndex, (uint32_t)pDst, xSize, ySize) == HAL_OK) {
                /* Polling For DMA transfer */
                HAL_DMA2D_PollForTransfer(&(this->hdma2d), 10);
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

/* C-linkage version for interrupt driver code of the above functions (written in C) */
extern "C" {
LTDC_HandleTypeDef* get_hltdc() {
    return getLcdLtdcHandle();
}

DSI_HandleTypeDef* get_hdsi() {
    return getLcdDsiHandle();
}
} // extern "C"

