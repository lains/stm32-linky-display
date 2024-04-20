#ifndef _STM32DEBUGOUTPUT_H_
#define _STM32DEBUGOUTPUT_H_

#ifdef STM32F469xx
#include "stm32f4xx_hal.h"
#endif
#ifdef STM32F769xx
#include "stm32f7xx_hal.h"
#endif
#include <cstdint>
#ifdef USE_ALLOCATION
#include <string>
#endif
#include "Stm32SerialDriver.h" // FIXME: this is just to get USART_DBG, we should probably move this into another file (main.h?)

/**
 * @brief Serial link debug output class (singleton)
 */
class Stm32DebugOutput {
public:
    /**
     * @brief Singleton instance getter
     * 
     * @return The singleton instance of this class
     */
    static Stm32DebugOutput& get();

    /** @brief Reset the debug output serial port
     * 
     * @warning It is required to calling this function once before any other method
     *          The serial port is not initialized at construction, because this class is a singletong and is constructed too early in the program initialization process
     */
    void start();

    /**
     * @brief Send a byte buffer to the debug console
     */
    bool send(const uint8_t* buffer, unsigned int len);

    /**
     * @brief Send a C-formatted string to the debug console
     */
    bool send(const char* text);

#ifdef USE_ALLOCATION
    /**
     * @brief Send a C++ std::string to the debug console
     */
    bool send(const std::string& text);
#endif

    /**
     * @brief Send an unsigned int value to the debug console
     */
    bool send(unsigned int value);

    /**
     * @brief Send the hex dump of a byte buffer to the debug console
     */
    bool hexdumpBuffer(const uint8_t* buffer, unsigned int len);

private:
    Stm32DebugOutput();

/* Attributes */
private:
    static Stm32DebugOutput instance;    /*!< Lazy singleton instance */
    UART_HandleTypeDef handle; /*!< The UART handle to use for output */
public:
    bool inError; /*!< Is the output currently in error? */
};

#endif // _STM32DEBUGOUTPUT_H_
