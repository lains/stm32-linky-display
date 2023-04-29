/* Includes ------------------------------------------------------------------*/
#include "Stm32SerialDriver.h"
#include "Stm32LcdDriver.h"
#include "TIC/Unframer.h"
#include "TicFrameParser.h"

extern "C" {
#include "main.h"
#include <stdio.h>
#include "font58.h"

static void SystemClock_Config(void); /* Defined below */
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

//static uint32_t ImageIndex = 0;
/*static const uint32_t * Images[] = 
{
    image_320x240_argb8888,
    life_augmented_argb8888,
};*/

/**
  * @brief  On Error Handler on condition TRUE.
  * @param  condition : Can be TRUE or FALSE
  * @retval None
  */
void OnError_Handler(uint32_t condition)
{
    if(condition) {
        BSP_LED_On(LED2);
        while(1) { ; } /* Blocking on error */
    }
}

void Error_Handler() {
	  OnError_Handler(1);
}

typedef void(*FHalDelayRefreshFunc)(void* context);

/**
 * @brief Wait for a given delay in ms, while running a function in background
 * 
 * @param toRunWhileWaiting A function to run continously while waiting
 * @param context A context pointer provided to toRunWhileWaiting as argument
 * @param Delay The delay to wait for (in ms)
 * 
 */
void waitDelay(uint32_t Delay, FHalDelayRefreshFunc toRunWhileWaiting = nullptr, void* context = nullptr) {
    uint32_t tickstart = HAL_GetTick();
    uint32_t wait = Delay;

    /* Add a freq to guarantee minimum wait */
    if (wait < HAL_MAX_DELAY) {
        wait += (uint32_t)(uwTickFreq);
    }

    while((HAL_GetTick() - tickstart) < wait) {
        if (toRunWhileWaiting != nullptr) {
            toRunWhileWaiting(context);
        }
    }
}

/**
 * @brief  Main program
 */
int main(void) {
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

    /* Configure LED1, LED2, LED3 and LED4 */
    BSP_LED_Init(LED1);
    BSP_LED_Init(LED2);
    BSP_LED_Init(LED3);
    BSP_LED_Init(LED4);
    BSP_LED_On(LED2);
    waitDelay(250);
    BSP_LED_Off(LED2);

    /* Initialize the SDRAM */
    BSP_SDRAM_Init();
    //BSP_SD_Init();

    Stm32SerialDriver& ticSerial = Stm32SerialDriver::get();

    ticSerial.start();

    Stm32LcdDriver& lcd = Stm32LcdDriver::get();

    //BSP_TS_Init(800,480);
    /* Initialize the LCD */
    OnError_Handler(!lcd.start());

    //ticSerial.print("Buffers created. Starting...\r\n");

    TicFrameParser ticParser;

    TIC::Unframer ticUnframer(TicFrameParser::unwrapInvokeOnFrameNewBytes,
                              TicFrameParser::unwrapInvokeOnFrameComplete,
                              (void *)(&ticParser));

    struct TicProcessingContext {
        /* Default constructor */
        TicProcessingContext(Stm32SerialDriver& ticSerial, TIC::Unframer& ticUnframer) :
            ticSerial(ticSerial),
            ticUnframer(ticUnframer),
            lostTicBytes(0),
            serialRxOverflowCount(0),
            instantaneousPower(static_cast<uint32_t>(-1)),
            lastParsedFrameNb(static_cast<unsigned int>(-1)) { }

        Stm32SerialDriver& ticSerial; /*!< The encapsulated TIC serial bytes receive handler */
        TIC::Unframer& ticUnframer;   /*!< The encapsulated TIC frame delimiter handler */
        unsigned int lostTicBytes;    /*!< How many TIC bytes were lost due to forwarding queue overflow? */
        unsigned int serialRxOverflowCount;  /*!< How many times we didnot read fast enough the serial buffer and bytes where thus lost due to incoming serial buffer overflow */
        uint32_t instantaneousPower;    /*!< A place to store the instantaneous power measurement */
        unsigned int lastParsedFrameNb; /*!< The ID of the last received TIC frame */
    };

    TicProcessingContext ticContext(ticSerial, ticUnframer);

    auto streamTicRxBytesToUnframer = [](void* context) {
        if (context == nullptr)
            return;
        TicProcessingContext* ticContext = static_cast<TicProcessingContext*>(context);
        uint8_t streamedBytesBuffer[256];  /* We allow copies of max 256 bytes at a time */
        size_t incomingBytesCount = ticContext->ticSerial.read(streamedBytesBuffer, sizeof(streamedBytesBuffer));
        std::size_t processedBytesCount = ticContext->ticUnframer.pushBytes(streamedBytesBuffer, incomingBytesCount);
        if (processedBytesCount < incomingBytesCount) {
            size_t lostBytesCount = incomingBytesCount - processedBytesCount;
            if (lostBytesCount > static_cast<unsigned int>(-1)) {
                lostBytesCount = static_cast<unsigned int>(-1); /* Does not fit in an unsigned int! */
            }
            if (static_cast<unsigned int>(-1) - lostBytesCount < ticContext->lostTicBytes) {
                /* Adding lostBytesCount will imply an overflow an overflow of our counter */
                ticContext->lostTicBytes = static_cast<unsigned int>(-1); /* Maxmimize our counter, we can't do better than this */
            }
            else {
                ticContext->lostTicBytes += lostBytesCount;
            }
        }
        unsigned int serialRxOverflowCount = ticContext->ticSerial.getRxOverflowCount(true);
        if (static_cast<unsigned int>(-1) - serialRxOverflowCount < ticContext->serialRxOverflowCount) {
            /* Adding serialRxOverflowBytes will imply an overflow of our counter */
            ticContext->serialRxOverflowCount = static_cast<unsigned int>(-1); /* Maxmimize our counter, we can't do better than this */
        }
        else {
            ticContext->serialRxOverflowCount += serialRxOverflowCount;
        }
    };

    unsigned int lcdRefreshCount = 0;
    while (1) {
        lcd.waitForFinalDisplayed(streamTicRxBytesToUnframer, static_cast<void*>(&ticContext)); /* Wait until the LCD displays the final framebuffer */

        ticContext.lastParsedFrameNb = ticParser.lastFrameMeasurements.fromFrameNb;

        /* We can now work on draft buffer */
        char statusLine[]="@@@@L - @@@@F - @@@@@@B - @@@@XR";
        statusLine[0]=(lcdRefreshCount / 1000) % 10 + '0';
        statusLine[1]=(lcdRefreshCount / 100) % 10 + '0';
        statusLine[2]=(lcdRefreshCount / 10) % 10 + '0';
        statusLine[3]=(lcdRefreshCount / 1) % 10 + '0';

        unsigned int framesCount = ticParser.nbFramesParsed;
        statusLine[8]=(framesCount / 1000) % 10 + '0';
        statusLine[9]=(framesCount / 100) % 10 + '0';
        statusLine[10]=(framesCount / 10) % 10 + '0';
        statusLine[11]=(framesCount / 1) % 10 + '0';

        unsigned long rxBytesCount = ticSerial.getRxBytesTotal();
        statusLine[16]=(rxBytesCount / 100000) % 10 + '0';
        statusLine[17]=(rxBytesCount / 10000) % 10 + '0';
        statusLine[18]=(rxBytesCount / 1000) % 10 + '0';
        statusLine[19]=(rxBytesCount / 100) % 10 + '0';
        statusLine[20]=(rxBytesCount / 10) % 10 + '0';
        statusLine[21]=(rxBytesCount / 1) % 10 + '0';

        unsigned int serialRxOverflowCount = ticContext.serialRxOverflowCount;
        statusLine[26]=(serialRxOverflowCount / 1000) % 10 + '0';
        statusLine[27]=(serialRxOverflowCount / 100) % 10 + '0';
        statusLine[28]=(serialRxOverflowCount / 10) % 10 + '0';
        statusLine[29]=(serialRxOverflowCount / 1) % 10 + '0';

        /* We're getting a lot of bytes in RX, but somehow we're missing frames */
        BSP_LCD_SetFont(&Font24);
        lcd.fillRect(0, 3*24, lcd.getWidth(), 24, Stm32LcdDriver::LCD_Color::Black);
        BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
        BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
        auto get_font24_ptr = [](const char c) {
            unsigned int bytesPerGlyph = Font24.Height * ((Font24.Width + 7) / 8);
            return &(Font24.table[(c-' ') * bytesPerGlyph]);
        };
        
        lcd.drawText(0, 3*24, statusLine, Font24.Width, Font24.Height, get_font24_ptr, Stm32LcdDriver::LCD_Color::White, Stm32LcdDriver::LCD_Color::Black);

        char mainInstPowerText[] = "-[9999;9999]W";
        lcd.fillRect(0, 4*24, lcd.getWidth(), lcd.getHeight() - 4*24, Stm32LcdDriver::LCD_Color::White);
        const TicEvaluatedPower& instantaneousPower = ticParser.lastFrameMeasurements.instPower;
        if (instantaneousPower.isValid) {
            if (instantaneousPower.isExact && instantaneousPower.minValue>0) {  /* We are withdrawing power */
                unsigned int withdrawnPower = static_cast<unsigned int>(instantaneousPower.minValue);
                if (withdrawnPower <= 9999) {
                    for (unsigned int pos=0; pos<8; pos++)
                        mainInstPowerText[pos] = ' ';   /* Fill leading characters with blank */
                    
                    if (withdrawnPower < 1000) {
                        mainInstPowerText[8]=' ';
                    } else {
                        uint8_t digit1000 = (withdrawnPower / 1000) % 10;
                        mainInstPowerText[8]=digit1000 + '0';
                    }
                    if (withdrawnPower < 100) {
                        mainInstPowerText[9]=' ';
                    } else {
                        uint8_t digit100 = (withdrawnPower / 100) % 10;
                        mainInstPowerText[9]=digit100 + '0';
                    }
                    if (withdrawnPower < 10) {
                        mainInstPowerText[10]=' ';
                    } else {
                        uint8_t digit10 = (withdrawnPower / 10) % 10;
                        mainInstPowerText[10]=digit10  + '0';
                    }
                    {
                        uint8_t digit1 = (withdrawnPower / 1) % 10;
                        mainInstPowerText[11]=digit1 + '0';
                    }
                    mainInstPowerText[12]='W';
                }
                else {
                    for (unsigned int pos=0; pos<sizeof(mainInstPowerText)-1; pos++)
                        mainInstPowerText[pos] = '?';   /* Fill all characters with blank */
                }
            }
            else { /* We are injecting power */
                if (instantaneousPower.isValid) {
                    unsigned int minPower = -(instantaneousPower.minValue); /* min and max will be negative */
                    unsigned int maxPower = -(instantaneousPower.maxValue);
                    mainInstPowerText[0]='-';
                    mainInstPowerText[1]='[';
                    if (minPower < 1000) {
                        mainInstPowerText[2]=' ';
                    } else {
                        uint8_t digit1000 = (minPower / 1000) % 10;
                        mainInstPowerText[2]=digit1000 + '0';
                    }
                    if (minPower < 100) {
                        mainInstPowerText[3]=' ';
                    } else {
                        uint8_t digit100 = (minPower / 100) % 10;
                        mainInstPowerText[3]=digit100 + '0';
                    }
                    if (minPower < 10) {
                        mainInstPowerText[4]=' ';
                    } else {
                        uint8_t digit10 = (minPower / 10) % 10;
                        mainInstPowerText[4]=digit10  + '0';
                    }
                    {
                        uint8_t digit1 = (minPower / 1) % 10;
                        mainInstPowerText[5]=digit1 + '0';
                    }
                    mainInstPowerText[6]=';';

                    if (maxPower < 1000) {
                        mainInstPowerText[7]=' ';
                    } else {
                        uint8_t digit1000 = (maxPower / 1000) % 10;
                        mainInstPowerText[7]=digit1000 + '0';
                    }
                    if (maxPower < 100) {
                        mainInstPowerText[8]=' ';
                    } else {
                        uint8_t digit100 = (maxPower / 100) % 10;
                        mainInstPowerText[8]=digit100 + '0';
                    }
                    if (maxPower < 10) {
                        mainInstPowerText[9]=' ';
                    } else {
                        uint8_t digit10 = (maxPower / 10) % 10;
                        mainInstPowerText[9]=digit10  + '0';
                    }
                    {
                        uint8_t digit1 = (maxPower / 1) % 10;
                        mainInstPowerText[10]=digit1 + '0';
                    }
                    mainInstPowerText[11]=']';
                    mainInstPowerText[12]='W';
                }
            }
            lcd.drawText(00, lcd.getHeight()/2 - 120, mainInstPowerText, 60, 120, get_font58_ptr, Stm32LcdDriver::LCD_Color::Blue, Stm32LcdDriver::LCD_Color::White);
        }


        //BSP_LED_On(LED1);
        //waitDelay(250, streamTicRxBytesToUnframer, static_cast<void*>(&ticContext)););
        //BSP_LED_Off(LED1);

        lcd.displayDraft(streamTicRxBytesToUnframer, static_cast<void*>(&ticContext)); /* While waiting, continue forwarding incoming TIC bytes to the unframer */

        lcd.copyDraftToFinal();

        lcd.requestDisplayFinal(); /* Now we have copied the content to display to final framebuffer, we can perform the switch */

        waitDelay(1000, streamTicRxBytesToUnframer, static_cast<void*>(&ticContext)); /* While waiting, continue forwarding incoming TIC bytes to the unframer */
        lcdRefreshCount++;
    }
}

extern "C" {
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
    if (ret != HAL_OK) {
        while(1) { ; }
    }
  
    /* Activate the OverDrive to reach the 180 MHz Frequency */
    ret = HAL_PWREx_EnableOverDrive();
    if (ret != HAL_OK) {
       while(1) { ; }
    }
  
    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers */
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
    if (ret != HAL_OK) {
        while(1) { ; }
    }
}
} // extern "C"

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line) {
    /* User can add his own implementation to report the file name and line number,
      ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1) { ; }
}
#endif
