/* Includes ------------------------------------------------------------------*/
#include "Stm32SerialDriver.h"
#include "Stm32LcdDriver.h"
#include "Stm32TimerDriver.h"
#include "TIC/Unframer.h"
#include "TicProcessingContext.h"
#include "PowerHistory.h"
#include "TicFrameParser.h"
#include "HistoryDraw.h"

//#define SIMULATE_POWER_VALUES_WITHOUT_TIC

extern "C" {
#include "main.h"
#include <stdio.h>
#include "font58.h"

static void SystemClock_Config(void); /* Defined below */
}

static void CPU_CACHE_Enable(void);

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
        BSP_LED_On(LED1);
        while(1) { ; } /* Blocking on error */
    }
}

void Error_Handler() {
	  OnError_Handler(1);
}

class Stm32DebugOutput {
public:
    Stm32DebugOutput() : inError(false) {
        this->handle.Instance = USART3;
        this->handle.Init.BaudRate = 57600;
        this->handle.Init.WordLength = UART_WORDLENGTH_8B;
        this->handle.Init.StopBits = UART_STOPBITS_1;
        this->handle.Init.Parity = UART_PARITY_NONE;
        this->handle.Init.Mode = UART_MODE_TX_RX;
        this->handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;

        HAL_UART_DeInit(&(this->handle));
        if (HAL_UART_Init(&handle) != HAL_OK) {
            this->inError = true;
        }
    }

    bool send(const uint8_t* buffer, unsigned int len) {
        if (this->inError) return false;
        HAL_StatusTypeDef result = HAL_UART_Transmit(&(this->handle), (uint8_t*)buffer, len, 5000);
        if (result != HAL_OK)
            this->inError = true;
        return this->inError;
    }

    bool send(const char* text) {
        return this->send(reinterpret_cast<const uint8_t*>(text), strlen(text));
    }

    bool inError;
    UART_HandleTypeDef handle;
};

/**
 * @brief  Main program
 */
int main(void) {
    /* STM32F7xx HAL library initialization:
         - Configure the Flash ART accelerator on ITCM interface
         - Systick timer is configured by default as source of time base, but user 
             can eventually implement his proper time base source (a general purpose 
             timer for example or other time source), keeping in mind that Time base 
             duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and 
             handled in milliseconds basis.
         - Set NVIC Group Priority to 4
         - Low Level Initialization
     */
    /* Enable the CPU Cache */
    CPU_CACHE_Enable();

    HAL_Init();
  
    /* Configure the system clock to 180 MHz */
    SystemClock_Config();

    /* Configure LED1 (red, left most) and LED2 (green, second left) */
    // FIXME: can we also use a third and fourth LED?
    /* On STMF7, we have at least LED_GREEN, LED_RED */
    BSP_LED_Init(LED1);
    BSP_LED_Init(LED2);
    //BSP_LED_Init(LED3);
    //BSP_LED_Init(LED4);
    BSP_LED_On(LED2);
    waitDelay(250);
    BSP_LED_Off(LED2);

    /* Configure the Tamper push-button in GPIO Mode */
    BSP_PB_Init(BUTTON_WAKEUP, BUTTON_MODE_GPIO);

    /* Initialize the SDRAM */
    //BSP_SDRAM_Init(); // Not required?
    //BSP_SD_Init();

    Stm32DebugOutput debugSerial;

    Stm32SerialDriver& ticSerial = Stm32SerialDriver::get();

    ticSerial.start();

    Stm32LcdDriver& lcd = Stm32LcdDriver::get();

    /* Initialize the LCD */
    OnError_Handler(!lcd.start());

    PowerHistory powerHistory(PowerHistory::Per5Seconds);

    TicFrameParser ticParser(PowerHistory::unWrapOnNewPowerData, (void *)(&powerHistory));

    auto onFrameCompleteBlinkGreenLedAndInvokeHandler = [](void* context) {
        BSP_LED_Toggle(LED1); // Toggle the green LED when a frame has been completely received
        TicFrameParser::unwrapInvokeOnFrameComplete(context);   /* Invoke the frameparser's onFrameComplete handler */
    };

    TIC::Unframer ticUnframer(TicFrameParser::unwrapInvokeOnFrameNewBytes,
                              onFrameCompleteBlinkGreenLedAndInvokeHandler,
                              (void *)(&ticParser));

    TicProcessingContext ticContext(ticSerial, ticUnframer);

    powerHistory.setContext(&ticContext);

#ifdef SIMULATE_POWER_VALUES_WITHOUT_TIC
    auto streamTicRxBytesToUnframer = [](void* context) { }; /* Discard any TIC data */
#else
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
#endif
    auto isNoNewPowerReceivedSinceLastDisplay = [](void* context) -> bool {
        if (context == nullptr)
            return true;
        TicProcessingContext* ticContext = static_cast<TicProcessingContext*>(context);
        return (ticContext->lastParsedFrameNb == ticContext->lastDisplayedPowerFrameNb);
    };

    unsigned int lcdRefreshCount = 0;
#ifdef SIMULATE_POWER_VALUES_WITHOUT_TIC
    char sampleHorodateAsCString[] = "e230502000000";
	TIC::Horodate fakeHorodate = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodateAsCString), strlen(sampleHorodateAsCString));
#endif

    TicEvaluatedPower lastReceivedPower;

    uint32_t debugContext = 0;
    while (1) {
        lcd.waitForFinalDisplayed(streamTicRxBytesToUnframer, static_cast<void*>(&ticContext)); /* Wait until the LCD displays the final framebuffer */
        //debugTerm.send("Display refresh\r\n");

        Stm32MeasurementTimer fullDisplayCycleTimeMs(true);

#ifdef SIMULATE_POWER_VALUES_WITHOUT_TIC
        int fakePowerRefV = static_cast<int>(3000) - (static_cast<int>((lcdRefreshCount*30) % 6000)); /* Results in sweeping from +3000 to -3000W */
        int fakeMinPower = fakePowerRefV;
        int fakeMaxPower = fakePowerRefV;
        if (fakePowerRefV < 0) { /* When reaching negative values, forge a range instead of an exact measurement to be more realistic */
            fakeMinPower = fakePowerRefV/3;
            fakeMaxPower = fakePowerRefV/4; /* If negative, corrects to -3000 to [-1000;-750] */
        }
        TicEvaluatedPower fakePower(fakeMinPower, fakeMaxPower);
        fakeHorodate.addSeconds(30); /* Fast forward in time */
        ticParser.lastFrameMeasurements.instPower = fakePower;
        powerHistory.onNewPowerData(fakePower, fakeHorodate, ticContext.lastParsedFrameNb);
        ticParser.nbFramesParsed++; /* Introspection for debug */
        ticContext.lastParsedFrameNb = ticParser.nbFramesParsed;
#endif
        /* We can now work on draft buffer */
        uint8_t pos = 0;
        char statusLine[]="@@@@@L @@@@@F @@@@@@@@B @@X @@:@@:@@";
        statusLine[pos++]=(lcdRefreshCount / 10000) % 10 + '0';
        statusLine[pos++]=(lcdRefreshCount / 1000) % 10 + '0';
        statusLine[pos++]=(lcdRefreshCount / 100) % 10 + '0';
        statusLine[pos++]=(lcdRefreshCount / 10) % 10 + '0';
        statusLine[pos++]=(lcdRefreshCount / 1) % 10 + '0';
        pos++; // 'L'

        pos++;

        unsigned int framesCount = ticParser.nbFramesParsed;
        statusLine[pos++]=(framesCount / 10000) % 10 + '0';
        statusLine[pos++]=(framesCount / 1000) % 10 + '0';
        statusLine[pos++]=(framesCount / 100) % 10 + '0';
        statusLine[pos++]=(framesCount / 10) % 10 + '0';
        statusLine[pos++]=(framesCount / 1) % 10 + '0';
        pos++; // 'F'

        pos++;

        unsigned long rxBytesCount = ticSerial.getRxBytesTotal();
        statusLine[pos++]=(rxBytesCount / 10000000) % 10 + '0';
        statusLine[pos++]=(rxBytesCount / 1000000) % 10 + '0';
        statusLine[pos++]=(rxBytesCount / 100000) % 10 + '0';
        statusLine[pos++]=(rxBytesCount / 10000) % 10 + '0';
        statusLine[pos++]=(rxBytesCount / 1000) % 10 + '0';
        statusLine[pos++]=(rxBytesCount / 100) % 10 + '0';
        statusLine[pos++]=(rxBytesCount / 10) % 10 + '0';
        statusLine[pos++]=(rxBytesCount / 1) % 10 + '0';
        pos++; // 'B'

        pos++;

        unsigned int serialRxOverflowCount = ticContext.serialRxOverflowCount;
        statusLine[pos++]=(serialRxOverflowCount / 10) % 10 + '0';
        statusLine[pos++]=(serialRxOverflowCount / 1) % 10 + '0';
        pos++; // 'X'

        pos++;

        unsigned int nbSamples = 1;
        PowerHistoryEntry lastMeasurement;
        powerHistory.getLastPower(nbSamples, &lastMeasurement);
        if (nbSamples == 1 && lastMeasurement.horodate.isValid) {   /* We have a valid last measurement */
            const TIC::Horodate& displayedHorodate = lastMeasurement.horodate;
            unsigned int horodateHour = displayedHorodate.hour;
            statusLine[pos++]=(horodateHour / 10) % 10 + '0';
            statusLine[pos++]=(horodateHour / 1) % 10 + '0';
            pos++;
            unsigned int horodateMinute = displayedHorodate.minute;
            statusLine[pos++]=(horodateMinute / 10) % 10 + '0';
            statusLine[pos++]=(horodateMinute / 1) % 10 + '0';
            pos++;
            unsigned int horodateSecond = displayedHorodate.second;
            statusLine[pos++]=(horodateSecond / 10) % 10 + '0';
            statusLine[pos++]=(horodateSecond / 1) % 10 + '0';
            pos++;
        }
        else {
            pos += 9;
        }

        BSP_LCD_SetFont(&Font24);
        lcd.fillRect(0, 3*24, lcd.getWidth(), 24, Stm32LcdDriver::LCD_Color::Black);
        BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
        BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
        auto get_font24_ptr = [](const char c) {
            unsigned int bytesPerGlyph = Font24.Height * ((Font24.Width + 7) / 8);
            return &(Font24.table[(c-' ') * bytesPerGlyph]);
        };
        
        lcd.drawText(0, 3*24, statusLine, Font24.Width, Font24.Height, get_font24_ptr, Stm32LcdDriver::LCD_Color::White, Stm32LcdDriver::LCD_Color::Black);

        lcd.fillRect(0, 4*24, lcd.getWidth(), lcd.getHeight() - 4*24, Stm32LcdDriver::LCD_Color::White);
        ticContext.lastDisplayedPowerFrameNb = ticContext.lastParsedFrameNb; /* Used to detect a new TIC frame and display it as soon as it appears */

        if (ticContext.instantaneousPower.isValid) {
            lastReceivedPower = ticContext.instantaneousPower;
        }
        if (lastReceivedPower.isValid) {   /* We have a valid last measurement */
            char mainInstPowerText[] = "-[9999;9999]W";
            const TicEvaluatedPower& instantaneousPower = lastReceivedPower;
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
            lcd.drawText(0, lcd.getHeight()/2 - 120, mainInstPowerText, 60, 120, get_font58_ptr, Stm32LcdDriver::LCD_Color::Blue, Stm32LcdDriver::LCD_Color::White);
        }
        drawHistory(lcd, 1, lcd.getHeight()/2, lcd.getWidth()-2, lcd.getHeight() - lcd.getHeight()/2 - 1, powerHistory, static_cast<void*>(&debugContext));

        debugContext = fullDisplayCycleTimeMs.get();
        /* Counts to 121-157ms depending on the number of columns drawn */
        /* Or without main power display, reduces to 39-75ms */
        /* When using DMA2D (rectangle) to draw solid lines, counts to 118-131ms but a few columns are not drawn properly*/

        {
            Stm32MeasurementTimer displayTimer(true);
            lcd.displayDraft(streamTicRxBytesToUnframer, static_cast<void*>(&ticContext)); /* While waiting, continue forwarding incoming TIC bytes to the unframer */
            //debugContext = displayTimer.get(); /* Counts to 9-10ms */
        }

        {
            Stm32MeasurementTimer fbCopyTimer(true);
            lcd.copyDraftToFinal(); /* 25643 loops without any forced read/write, or 20691 (read+write) loops in the HAL_DMA2D_PollForTransfer() subroutine */
            //debugContext = fbCopyTimer.get(); /* Counts to 16-17ms */
        }

        lcd.requestDisplayFinal(); /* Now we have copied the content to display to final framebuffer, we can perform the switch */

        /* While waiting, continue forwarding incoming TIC bytes to the unframer */
        /* But inject a condition to immediately exit the loop to refresh the display if a new power measurement is received from TIC before the expiration of the wait delay */
        /* The 5s delay should never been reached because TIC data on power comes in more frequently */
#ifndef SIMULATE_POWER_VALUES_WITHOUT_TIC
        waitDelayAndCondition(5000, streamTicRxBytesToUnframer, isNoNewPowerReceivedSinceLastDisplay, static_cast<void*>(&ticContext));
#endif
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

    /* Enable HSE Oscillator and activate PLL with HSE as source */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 25;
    RCC_OscInitStruct.PLL.PLLN = 400;  
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 8;
    RCC_OscInitStruct.PLL.PLLR = 7;
    
    ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
    if(ret != HAL_OK) {
        while(1) { ; }
    }
    
    /* Activate the OverDrive to reach the 200 MHz Frequency */  
    ret = HAL_PWREx_EnableOverDrive();
    if(ret != HAL_OK) {
        while(1) { ; }
    }

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers */
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2; 
    
    ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_6);
    if(ret != HAL_OK) {
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

/**
  * @brief  CPU L1-Cache enable.
  * @param  None
  * @retval None
  */
static void CPU_CACHE_Enable(void)
{
  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}