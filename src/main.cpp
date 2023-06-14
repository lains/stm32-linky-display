/* Includes ------------------------------------------------------------------*/
#include <climits>

#include "Stm32SerialDriver.h"
#include "Stm32LcdDriver.h"
#include "TIC/Unframer.h"
#include "TicProcessingContext.h"
#include "PowerHistory.h"
#include "TicFrameParser.h"

//#define SIMULATE_POWER_VALUES_WITHOUT_TIC

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
typedef bool(*FHalDelayConditionFunc)(void* context);

/**
 * @brief Wait for a given delay in ms, while running a function in background. Stop waiting immediately if a condition becomes false
 * 
 * @param toRunWhileWaiting A function to run continously while waiting
 * @param conditionCheck A function returning false to immediately stop the wait period before timer elapses
 * @param context A context pointer provided to toRunWhileWaiting and/or conditionCheck as argument
 * @param delay The delay to wait for (in ms)
 * 
 */
void waitDelayAndCondition(uint32_t delay, FHalDelayRefreshFunc toRunWhileWaiting = nullptr, FHalDelayConditionFunc conditionCheck = nullptr, void* context = nullptr) {
    uint32_t tickstart = HAL_GetTick();
    uint32_t wait = delay;

    /* Add a freq to guarantee minimum wait */
    if (wait < HAL_MAX_DELAY) {
        wait += (uint32_t)(uwTickFreq);
    }

    while((HAL_GetTick() - tickstart) < wait && (conditionCheck != nullptr && conditionCheck(context))) {
        if (toRunWhileWaiting != nullptr) {
            toRunWhileWaiting(context);
        }
    }
}

/**
 * @brief Wait for a given delay in ms, while running a function in background
 * 
 * @param toRunWhileWaiting A function to run continously while waiting
 * @param context A context pointer provided to toRunWhileWaiting as argument
 * @param delay The delay to wait for (in ms)
 */
void waitDelay(uint32_t delay, FHalDelayRefreshFunc toRunWhileWaiting = nullptr, void* context = nullptr) {
    waitDelayAndCondition(delay, toRunWhileWaiting, nullptr, context);
}

void drawHistory(Stm32LcdDriver& lcd, uint16_t x, uint16_t y, uint16_t width, uint16_t height, const PowerHistory& history) {
    if (width == 0 || height == 0)
        return;
    
    uint16_t xright = x + width - 1;

    unsigned int nbHistoryEntries = width; /* Initially try to fill-in the full width of the area */

    PowerHistoryEntry powerMeasurements[nbHistoryEntries];
    history.getLastPower(nbHistoryEntries, powerMeasurements);
    if (nbHistoryEntries > width) {    /* Sanity, overflow , exclude oldest entries to fit on display */
        nbHistoryEntries = width;
    }

    uint16_t debugX = UINT16_MAX;
    uint16_t debugYtop = UINT16_MAX;
    uint16_t debugYbottom = UINT16_MAX;
    int debugPower = INT_MIN;
    bool debugPowerIsExact = false;
    int debugValue = INT_MIN;

    const int maxPower = 3000;
    const int minPower = -1500;
    uint16_t zeroSampleAbsoluteY = y;
    zeroSampleAbsoluteY += static_cast<uint16_t>(static_cast<unsigned long int>(maxPower) * static_cast<unsigned long int>(height) / static_cast<unsigned long int>(maxPower - minPower));
    for (unsigned int measurementAge = 0; measurementAge < nbHistoryEntries; measurementAge++) {
        uint16_t thisSampleAbsoluteX = xright - measurementAge;   /* First measurement sample is at the extreme right of the allocated area, next samples will be placed to the left */
        if (debugX == UINT16_MAX) debugX = thisSampleAbsoluteX;
        PowerHistoryEntry& thisSampleEntry = powerMeasurements[measurementAge];
        TicEvaluatedPower& thisSamplePower = thisSampleEntry.power;

        if (debugValue == INT_MIN) {
            if (nbHistoryEntries>0) {
                debugValue = thisSampleEntry.nbSamples;
            }
        }
        if (debugPower == INT_MIN) {
            if (thisSamplePower.isValid) {
                if (thisSamplePower.isExact) {
                    debugPower = thisSamplePower.minValue;
                    debugPowerIsExact = true;
                }
                else {
                    if (thisSamplePower.maxValue > 0) {
                        debugPower = thisSamplePower.minValue;
                        debugPowerIsExact = true;
                    }
                    else {  /* For negative value, compute an average */
                        debugPower = (thisSamplePower.minValue + thisSamplePower.maxValue) / 2;
                        debugPowerIsExact = false;
                    }
                }
            }
            else {
                debugPower = 0;
                debugPowerIsExact = false;
            }
        }

        if (thisSamplePower.isValid) {
            /* Review the code below */
            if (thisSamplePower.maxValue > 0) { /* Positive value, even if range, display the highest value of the range (worst case) */
                uint16_t thisSampleTopAbsoluteY = 0;
                uint16_t thisSampleBottomAbsoluteY = 0;
                uint16_t thisSampleRelativeY = 0;
                if (true || thisSamplePower.maxValue > 0) { /* Positive and exact power */
                    thisSampleRelativeY = (static_cast<unsigned long int>(thisSamplePower.maxValue) * static_cast<unsigned long int>(height) / static_cast<unsigned long int>(maxPower - minPower));
                    thisSampleTopAbsoluteY = zeroSampleAbsoluteY - thisSampleRelativeY;
                    thisSampleBottomAbsoluteY = zeroSampleAbsoluteY;
                    if (debugYtop == UINT16_MAX) debugYtop = thisSampleTopAbsoluteY;
                    if (debugYbottom == UINT16_MAX) debugYbottom = thisSampleBottomAbsoluteY;
                    lcd.drawVerticalLine(thisSampleAbsoluteX, thisSampleTopAbsoluteY, thisSampleBottomAbsoluteY-thisSampleTopAbsoluteY, Stm32LcdDriver::Red);
                }
                else if (thisSamplePower.maxValue < 0) { /* Negative and exact power */
                    thisSampleRelativeY = (static_cast<unsigned long int>(-thisSamplePower.maxValue) * static_cast<unsigned long int>(height) / static_cast<unsigned long int>(maxPower - minPower));
                    thisSampleTopAbsoluteY = zeroSampleAbsoluteY;
                    thisSampleBottomAbsoluteY = zeroSampleAbsoluteY + thisSampleRelativeY;
                    if (debugYtop == UINT16_MAX) debugYtop = thisSampleTopAbsoluteY;
                    if (debugYbottom == UINT16_MAX) debugYbottom = thisSampleBottomAbsoluteY;
                    lcd.drawVerticalLine(thisSampleAbsoluteX, thisSampleTopAbsoluteY, thisSampleBottomAbsoluteY-thisSampleTopAbsoluteY, Stm32LcdDriver::DarkGreen);
                }
            }
            else {  /* Max value is negative, we are injecting, display the range */
                uint16_t thisSampleMaxRelativeY = 0;
                uint16_t thisSampleMinRelativeY = 0;
                bool maxIsBelow0 = (thisSamplePower.maxValue < 0);
                bool minIsBelow0 = (thisSamplePower.minValue < 0);
                unsigned int maxAbsValue = maxIsBelow0?(-thisSamplePower.maxValue):thisSamplePower.maxValue;
                unsigned int minAbsValue = minIsBelow0?(-thisSamplePower.minValue):thisSamplePower.minValue;
                thisSampleMaxRelativeY = (static_cast<unsigned int>(maxAbsValue) * static_cast<unsigned long int>(height) / static_cast<unsigned int>(maxPower - minPower));
                thisSampleMinRelativeY = (static_cast<unsigned int>(minAbsValue) * static_cast<unsigned long int>(height) / static_cast<unsigned int>(maxPower - minPower));
                uint16_t thisSampleTopAbsoluteY = 0;
                uint16_t thisSampleBottomAbsoluteY = 0;
                if (maxIsBelow0)
                    thisSampleTopAbsoluteY = zeroSampleAbsoluteY + thisSampleMaxRelativeY;
                else
                    thisSampleTopAbsoluteY = zeroSampleAbsoluteY - thisSampleMaxRelativeY;
                if (minIsBelow0)
                    thisSampleBottomAbsoluteY = zeroSampleAbsoluteY + thisSampleMinRelativeY;
                else
                    thisSampleBottomAbsoluteY = zeroSampleAbsoluteY - thisSampleMinRelativeY;
                
                if (thisSampleTopAbsoluteY > thisSampleBottomAbsoluteY) {   /* Failsafe, should not occur */
                    std::swap(thisSampleTopAbsoluteY, thisSampleBottomAbsoluteY);
                }
                if (debugYtop == UINT16_MAX) debugYtop = thisSampleTopAbsoluteY;
                if (debugYbottom == UINT16_MAX) debugYbottom = thisSampleBottomAbsoluteY;
                lcd.drawVerticalLine(thisSampleAbsoluteX, thisSampleTopAbsoluteY, thisSampleBottomAbsoluteY-thisSampleTopAbsoluteY, Stm32LcdDriver::DarkGreen, Stm32LcdDriver::Green, 5);
            }
        }
    }

    auto get_font24_ptr = [](const char c) {
        unsigned int bytesPerGlyph = Font24.Height * ((Font24.Width + 7) / 8);
        return &(Font24.table[(c-' ') * bytesPerGlyph]);
    };

    lcd.drawHorizontalLine(x, zeroSampleAbsoluteY, width, Stm32LcdDriver::Black); /* Draw 0 */
    uint16_t gridY;
    uint16_t gridWidth = nbHistoryEntries;
    uint16_t gridX = width - gridWidth;
    gridY = y + static_cast<uint16_t>(static_cast<unsigned long int>(maxPower - 3000) * static_cast<unsigned long int>(height) / static_cast<unsigned long int>(maxPower - minPower));
    lcd.drawHorizontalLine(gridX, gridY, gridWidth, Stm32LcdDriver::Grey, Stm32LcdDriver::Transparent, 2); /* Draw +3000W) */
    gridY = y + static_cast<uint16_t>(static_cast<unsigned long int>(maxPower - 2000) * static_cast<unsigned long int>(height) / static_cast<unsigned long int>(maxPower - minPower));
    lcd.drawHorizontalLine(gridX, gridY, gridWidth, Stm32LcdDriver::Grey, Stm32LcdDriver::Transparent, 2); /* Draw +2000W */
    if (gridWidth > 24*5 + 10) /* Enough room to draw 2000W label (5 chars). Draw above the line (substracting text height from y) */
        lcd.drawText(gridX + 2, gridY - 22, "2000W", Font24.Width, Font24.Height, get_font24_ptr, Stm32LcdDriver::LCD_Color::Black, Stm32LcdDriver::LCD_Color::Transparent);
    gridY = y + static_cast<uint16_t>(static_cast<unsigned long int>(maxPower - 1000) * static_cast<unsigned long int>(height) / static_cast<unsigned long int>(maxPower - minPower));
    lcd.drawHorizontalLine(gridX, gridY, gridWidth, Stm32LcdDriver::Grey, Stm32LcdDriver::Transparent, 2); /* Draw +1000W */
    if (gridWidth > 24*5 + 10) /* Enough room to draw 1000W label (5 chars). Draw above the line (substracting text height from y) */
        lcd.drawText(gridX + 2, gridY - 22, "1000W", Font24.Width, Font24.Height, get_font24_ptr, Stm32LcdDriver::LCD_Color::Black, Stm32LcdDriver::LCD_Color::Transparent);
    gridY = y + static_cast<uint16_t>(static_cast<unsigned long int>(maxPower + 1000) * static_cast<unsigned long int>(height) / static_cast<unsigned long int>(maxPower - minPower));
    lcd.drawHorizontalLine(gridX, gridY, gridWidth, Stm32LcdDriver::Grey, Stm32LcdDriver::Transparent, 2); /* Draw -1000W */

    debugValue = nbHistoryEntries * 4 / history.getPowerRecordsPerHour();
    for (unsigned int quarter = 1; quarter <= nbHistoryEntries * 4 / history.getPowerRecordsPerHour(); quarter++) {
        uint16_t gridX = width;
        if (gridX >= history.getPowerRecordsPerHour() * quarter / 4) {
            gridX -= history.getPowerRecordsPerHour() * quarter / 4;
            if (quarter % 4 == 0)  /* One hour */
                lcd.drawVerticalLine(gridX, y, height, Stm32LcdDriver::Black); /* Draw each hour */
            else
                lcd.drawVerticalLine(gridX, y, height, Stm32LcdDriver::Grey, Stm32LcdDriver::Transparent, 2); /* Draw each other quarter */
        }
    }

    /* FIXME: for debug only - start */
    uint8_t pos = 0;
    char statusLine[]="DH=@@@@ X=@@@ Yt=@@@ Yb=@@@ Dbg=@@@@@ P=@@@@@@@";
    pos+=3; // "DH="
    statusLine[pos++]=(nbHistoryEntries / 1000) % 10 + '0';
    statusLine[pos++]=(nbHistoryEntries / 100) % 10 + '0';
    statusLine[pos++]=(nbHistoryEntries / 10) % 10 + '0';
    statusLine[pos++]=(nbHistoryEntries / 1) % 10 + '0';
    pos++;

    pos+=2;
    statusLine[pos++]=(debugX / 100) % 10 + '0';
    statusLine[pos++]=(debugX / 10) % 10 + '0';
    statusLine[pos++]=(debugX / 1) % 10 + '0';
    pos++;

    pos+=3;
    if (debugYtop < 1000) {
        statusLine[pos++]=(debugYtop / 100) % 10 + '0';
        statusLine[pos++]=(debugYtop / 10) % 10 + '0';
        statusLine[pos++]=(debugYtop / 1) % 10 + '0';
    }
    else if (debugYtop == UINT16_MAX) {
        statusLine[pos++]='!';
        statusLine[pos++]='!';
        statusLine[pos++]='!';
    }
    else
        pos+=3;
    pos++;

    pos+=3;
    if (debugYbottom < 1000) {
        statusLine[pos++]=(debugYbottom / 100) % 10 + '0';
        statusLine[pos++]=(debugYbottom / 10) % 10 + '0';
        statusLine[pos++]=(debugYbottom / 1) % 10 + '0';
    }
    else if (debugYbottom == UINT16_MAX) {
        statusLine[pos++]='!';
        statusLine[pos++]='!';
        statusLine[pos++]='!';
    }
    else
        pos+=3;
    pos++;

    pos+=4;
    if (debugValue != INT_MIN) {
        if (debugValue < 0) {
            statusLine[pos++]='-';
            debugValue = -debugValue;
        }
        else
            statusLine[pos++]=' ';
        statusLine[pos++]=(debugValue / 1000) % 10 + '0';
        statusLine[pos++]=(debugValue / 100) % 10 + '0';
        statusLine[pos++]=(debugValue / 10) % 10 + '0';
        statusLine[pos++]=(debugValue / 1) % 10 + '0';
    }
    pos++;

    pos+=2; // "P="
    if (debugPower != INT_MIN) {
        if (debugPower < -99999 || debugPower > 99999) { /* Max displayable value +/-99kW */
            statusLine[pos++]='*'; /* Overflow/underflow */
        }
        else {
            statusLine[pos++]=debugPowerIsExact?' ':'~';
        }
        if (debugPower < 0) {
            statusLine[pos++]='-';
            debugPower = -debugPower;
        }
        else {
            statusLine[pos++]=' ';
        }
        statusLine[pos++]=(debugPower / 10000) % 10 + '0';
        statusLine[pos++]=(debugPower / 1000) % 10 + '0';
        statusLine[pos++]=(debugPower / 100) % 10 + '0';
        statusLine[pos++]=(debugPower / 10) % 10 + '0';
        statusLine[pos++]=(debugPower / 1) % 10 + '0';
    }
     
    lcd.drawText(0, 4*24, statusLine, Font24.Width, Font24.Height, get_font24_ptr, Stm32LcdDriver::LCD_Color::White, Stm32LcdDriver::LCD_Color::Black);
    /* FIXME: end */
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

    Stm32DebugOutput debugSerial;

    Stm32SerialDriver& ticSerial = Stm32SerialDriver::get();

    ticSerial.start();

    Stm32LcdDriver& lcd = Stm32LcdDriver::get();

    //BSP_TS_Init(800,480);
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

    while (1) {
        lcd.waitForFinalDisplayed(streamTicRxBytesToUnframer, static_cast<void*>(&ticContext)); /* Wait until the LCD displays the final framebuffer */
        //debugTerm.send("Display refresh\r\n");

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
        drawHistory(lcd, 0, lcd.getHeight()/2, lcd.getWidth(), lcd.getHeight() - lcd.getHeight()/2 - 1, powerHistory);

        lcd.displayDraft(streamTicRxBytesToUnframer, static_cast<void*>(&ticContext)); /* While waiting, continue forwarding incoming TIC bytes to the unframer */

        lcd.copyDraftToFinal();

        lcd.requestDisplayFinal(); /* Now we have copied the content to display to final framebuffer, we can perform the switch */

        /* While waiting, continue forwarding incoming TIC bytes to the unframer */
        /* But inject a condition to immediately exit the loop to refresh the display if a new power measurement is received from TIC before the expiration of the wait delay */
        /* The 5s delay should never been reached because TIC data on power comes in more frequently */
        waitDelayAndCondition(5000, streamTicRxBytesToUnframer, isNoNewPowerReceivedSinceLastDisplay, static_cast<void*>(&ticContext));
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
