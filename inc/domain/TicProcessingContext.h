#pragma once

#include "TIC/Unframer.h"
#include "TicFrameParser.h" // For TicEvaluatedPower
#include "Stm32SerialDriver.h"
#include <stdint.h>

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
};