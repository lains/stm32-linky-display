/* Includes ------------------------------------------------------------------*/
#include "Stm32SerialDriver.h"
#include "Stm32LcdDriver.h"
#include "Stm32TimerDriver.h"
#include "Stm32DebugOutput.h"
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

#ifdef USE_STM32F769I_DISCO
static void MPU_Config(void);
static void CPU_CACHE_Enable(void);
#endif

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
#ifdef LED_ERROR
        BSP_LED_On(LED_ERROR);
#endif
        while(1) { ; } /* Blocking on error */
    }
}

void Error_Handler() {
	  OnError_Handler(1);
}

/**
 * @brief  Main program
 */
int main(void) {
    /* STM32Fxxx HAL library initialization */
#ifdef USE_STM32F769I_DISCO
    /* Configure the MPU attributes */
    MPU_Config();

    /* Enable the CPU Cache */
    CPU_CACHE_Enable();
#endif

    HAL_Init();
  
    /* Configure the system clock to 180 MHz */
    SystemClock_Config();

    /* Configure the 4 available LEDs on USE_STM32469I_DISCOVERY */
#ifdef USE_STM32469I_DISCOVERY
    BSP_LED_Init(LED_GREEN);
    BSP_LED_Init(LED_RED);
    BSP_LED_Init(LED_ORANGE);
    BSP_LED_Init(LED_BLUE);
#endif
    /* Configure the 2 available LEDs on STM32F769I DISCOVERY */
#ifdef USE_STM32F769I_DISCO
    BSP_LED_Init(LED_RED);
    BSP_LED_Init(LED_GREEN);
#endif
    BSP_LED_On(LED_STARTUP_BLINK);
    waitDelay(250);
    BSP_LED_Off(LED_STARTUP_BLINK);

#ifdef USE_STM32F769I_DISCO
    /* Configure the Tamper push-button in GPIO Mode */
    BSP_PB_Init(BUTTON_WAKEUP, BUTTON_MODE_GPIO);
#endif

    /* Initialize the SDRAM */
    BSP_SDRAM_Init();
    //BSP_SD_Init();

    Stm32DebugOutput::get().start();
    Stm32DebugOutput::get().send("Init\n");

    Stm32SerialDriver& ticSerial = Stm32SerialDriver::get();

    //ticSerial.start(9600); /* For standard TIC */
    ticSerial.start(1200); /* For historical TIC */

    Stm32LcdDriver& lcd = Stm32LcdDriver::get();

    //BSP_TS_Init(800,480);
    /* Initialize the LCD */
    OnError_Handler(!lcd.start());

    PowerHistory powerHistory(PowerHistory::Per5Seconds);

    TicFrameParser ticParser(PowerHistory::unWrapOnNewPowerData, (void *)(&powerHistory));

    auto onFrameCompleteBlinkGreenLedAndInvokeHandler = [](void* context) {
#ifdef LED_TIC_FRAME_RX
        Stm32DebugOutput::get().send("Frame complete\n");
        BSP_LED_Toggle(LED_TIC_FRAME_RX); // Toggle the green LED when a frame has been completely received
#endif
        TicFrameParser::unwrapInvokeOnFrameComplete(context);   /* Invoke the frameparser's onFrameComplete handler */
    };

    TIC::Unframer ticUnframer(TicFrameParser::unwrapInvokeOnFrameNewBytes,
                              onFrameCompleteBlinkGreenLedAndInvokeHandler,
                              (void *)(&ticParser));

    TicProcessingContext ticContext(ticSerial, ticUnframer);

    powerHistory.setContext(&ticContext);

    Stm32DebugOutput::get().send("Waiting for TIC data...\n");

#ifdef SIMULATE_POWER_VALUES_WITHOUT_TIC
    auto streamTicRxBytesToUnframer = [](void* context) { }; /* Discard any TIC data */
#else
    auto streamTicRxBytesToUnframer = [](void* context) {
        if (context == nullptr)
            return;
        TicProcessingContext* ticContext = static_cast<TicProcessingContext*>(context);
        uint8_t streamedBytesBuffer[256];  /* We allow copies of max 256 bytes at a time */
        size_t incomingBytesCount = ticContext->ticSerial.read(streamedBytesBuffer, sizeof(streamedBytesBuffer));
        if (incomingBytesCount == 0)
            return;
        // char countAsStr[4];
        // unsigned int pos = 0;
        // if (incomingBytesCount >= 1000) {
        //     countAsStr[0]='+';
        //     countAsStr[1]='+';
        //     countAsStr[2]='+';
        //     countAsStr[3]='\0';
        // }
        // else {
        //     if (incomingBytesCount >= 100) {
        //         countAsStr[pos++] = '0' + (incomingBytesCount / 100)%10;
        //     }
        //     if (incomingBytesCount >= 10) {
        //         countAsStr[pos++] = '0' + (incomingBytesCount / 10)%10;
        //     }
        //     countAsStr[pos++] = '0' + (incomingBytesCount / 1)%10;
        //     countAsStr[pos++] = '\0';
        // }
        // Stm32DebugOutput::get().send("TIC RX ");
        // Stm32DebugOutput::get().send(countAsStr);
        // Stm32DebugOutput::get().send(" bytes: ");
        // Stm32DebugOutput::get().hexdumpBuffer(streamedBytesBuffer, incomingBytesCount);
        // Stm32DebugOutput::get().send("\n");
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
        if (nbSamples == 1 && lastMeasurement.timestamp.isValid) {   /* We have a valid last measurement */
            const Timestamp& displayedHorodate = lastMeasurement.timestamp;
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
        if (isNoNewPowerReceivedSinceLastDisplay(static_cast<void*>(&ticContext))) {
            Stm32DebugOutput::get().send("LCD refresh without TIC rx\n");
        }
        else {
            Stm32DebugOutput::get().send("LCD refresh on TIC power rx\n");
        }
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

#ifdef USE_STM32469I_DISCOVERY
    /* Enable Power Control clock */
    __HAL_RCC_PWR_CLK_ENABLE();

    /* The voltage scaling allows optimizing the power consumption when the device is 
      clocked below the maximum system frequency, to update the voltage scaling value 
      regarding system frequency refer to product datasheet.  */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
#endif

    /* Enable HSE Oscillator and activate PLL with HSE as source */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
#ifdef USE_STM32469I_DISCOVERY
#if defined(USE_STM32469I_DISCO_REVA)
    RCC_OscInitStruct.PLL.PLLM = 25;
#else
    RCC_OscInitStruct.PLL.PLLM = 8;
#endif /* USE_STM32469I_DISCO_REVA */

    RCC_OscInitStruct.PLL.PLLN = 360;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    RCC_OscInitStruct.PLL.PLLR = 6;
#endif
#ifdef USE_STM32F769I_DISCO
    RCC_OscInitStruct.PLL.PLLM = 25;
    RCC_OscInitStruct.PLL.PLLN = 400;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 8;
    RCC_OscInitStruct.PLL.PLLR = 7;
#endif

    ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
    if (ret != HAL_OK) {
        while(1) { ; }
    }
  
    /* Activate the OverDrive to reach:
       - the 180 MHz Frequency on STM32F469I DISCOVERY
       - the 200 MHz Frequency on STM32F769I DISCOVERY
    */
    ret = HAL_PWREx_EnableOverDrive();
    if (ret != HAL_OK) {
       while(1) { ; }
    }
  
    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers */
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
#ifdef USE_STM32469I_DISCOVERY
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV4; // Default value (DIV2) does not allow to go down to 1200 bauds for historical TIC
    /* We change APB2 divider because TIC USART (USART6) is connected to APB2 as per RM0386 datasheet, 2.2.2 table 1.
       This has consequences on other devices like DSI Host and LCD-TFT however, so be careful.
    */
#else
#ifdef USE_STM32F769I_DISCO
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV4; // Default value (DIV2) does not allow to go down to 1200 bauds for historical TIC
    /* We change APB2 divider because TIC USART (USART6) is connected to APB2 */
#else
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
#endif // USE_STM32F769I_DISCO
#endif

#ifdef USE_STM32469I_DISCOVERY
#define BOARD_FLASH_LATENCY FLASH_LATENCY_5
#endif
#ifdef USE_STM32F769I_DISCO
#define BOARD_FLASH_LATENCY FLASH_LATENCY_6
#endif
    ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, BOARD_FLASH_LATENCY);

    if (ret != HAL_OK) {
        while(1) { ; }
    }
}
} // extern "C"


#ifdef USE_STM32F769I_DISCO
/**
  * @brief  CPU L1-Cache enable.
  * @param  None
  * @retval None
  */
static void CPU_CACHE_Enable(void) {
    /* Enable I-Cache */
    SCB_EnableICache();

    /* Enable D-Cache */
    SCB_EnableDCache();
}

/**
  * @brief  Configure the MPU attributes
  * @param  None
  * @retval None
  */
static void MPU_Config(void) {
    MPU_Region_InitTypeDef MPU_InitStruct;

    /* Disable the MPU */
    HAL_MPU_Disable();

    /* Configure the MPU as Strongly ordered for not defined regions */
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.BaseAddress = 0x00;
    MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
    MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER0;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
    MPU_InitStruct.SubRegionDisable = 0x87;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;

    HAL_MPU_ConfigRegion(&MPU_InitStruct);

    /* Configure the MPU attributes as WT for SDRAM */
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.BaseAddress = 0xC0000000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_32MB;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER1;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
    MPU_InitStruct.SubRegionDisable = 0x00;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

    HAL_MPU_ConfigRegion(&MPU_InitStruct);

    /* Configure the MPU attributes FMC control registers */
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.BaseAddress = 0xA0000000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_8KB;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER2;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
    MPU_InitStruct.SubRegionDisable = 0x0;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    
    HAL_MPU_ConfigRegion(&MPU_InitStruct);

    /* Enable the MPU */
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}
#endif

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
