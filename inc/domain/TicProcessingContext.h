#pragma once

#include "TIC/Unframer.h"
#include "TicFrameParser.h" // For TicEvaluatedPower
#include "TimeOfDay.h"
#ifndef __UNIT_TEST__
#include "../hal/Stm32SerialDriver.h"
#else
struct Stm32SerialDriver {
    Stm32SerialDriver() {}
};
#endif
#include <stdint.h>

struct SystemCurrentTime {
    SystemCurrentTime();

    /**
     * @brief React to a new day starting (just after midnight)
    */
    void startNewDayAtMidnight();

/* Attributes */
    TimeOfDay time; /*!< The current time of day */
    bool relativeToBoot; /*!< Is the time of day relative to boot (uptime) or absolute to day start */
};

struct TicProcessingContext {
    /**
     * @brief Constructor
     * 
     * @param ticSerial The serial byte receive instance
     * @param ticUnframer A TIC frame delimiter instance
     */
    TicProcessingContext(Stm32SerialDriver& ticSerial, TIC::Unframer& ticUnframer);

/* Attributes */
    Stm32SerialDriver& ticSerial; /*!< The encapsulated TIC serial bytes receive handler */
    TIC::Unframer& ticUnframer;   /*!< The encapsulated TIC frame delimiter handler */
    unsigned int lostTicBytes;    /*!< How many TIC bytes were lost due to forwarding queue overflow? */
    unsigned int serialRxOverflowCount;  /*!< How many times we didnot read fast enough the serial buffer and bytes where thus lost due to incoming serial buffer overflow */
    TicEvaluatedPower instantaneousPower;    /*!< A place to store the instantaneous power measurement */
    unsigned int lastParsedFrameNb; /*!< The ID of the last received TIC frame */
    unsigned int lastDisplayedPowerFrameNb; /*!< The ID of the last TIC frame for which the instantanous power was displayed on the screen */
    uint32_t displayTimeMs; /*!< The last duration it took (in ms) to switch display from one frame to another, for statistics */
    uint32_t fbCopyTimeMs; /*!< The last duration it took (in ms) to copy a full framebuffer content, for statistics */
    SystemCurrentTime currentTime; /*!< The current system time of day */
};