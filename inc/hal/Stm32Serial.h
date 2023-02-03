#ifndef _UARTDRIVER_H_
#define _UARTDRIVER_H_

#include "stm32f4xx_hal.h"
#include <cstdint>
#ifdef USE_ALLOCATION
#include <string>
#endif

extern "C" {
UART_HandleTypeDef* get_huart6(void);   // C-linkage exported getter for huart6 handler
}

/**
 * @brief Serial link communication class (singleton)
 */
class Stm32Serial {
public:
    /**
     * @brief Singleton instance getter
     * 
     * @return The singleton instance of this class
     */
    static Stm32Serial& get();

    /**
     * @brief Initialize the serial link and start receiving data from it
     */
    void start();

    /**
     * @brief Write the human-readable hexadecimal value of a data byte to the serial link (formatted as ASCII)
     * 
     * @param byte The byte to dump
     */
    void writeByteHexdump(unsigned char byte);

    /**
     * @brief Print a string to the serial link
     * 
     * @param str The C-style string to output
     */
    void print(const char* str);

#ifdef USE_ALLOCATION
    /**
     * @brief Print a string to the serial link
     * 
     * @param str The C++ std::string to output
     */
    void print(const std::string& str);
#endif

    /**
     * @brief Receive one data byte from the serial link
     * 
     * This method is to be used as the callback for new data bytes coming in on the serial link
     * 
     * @param incomingByte The new data byte
     */
    void onRx(uint8_t incomingByte);

    /**
     * @brief Get the low-level serial link handler object
     * 
     * @return UART_HandleTypeDef* The STM32 low level UART hanlder
     */
    friend UART_HandleTypeDef* getTicUartHandle();

private:
    Stm32Serial& operator= (const Stm32Serial&) { return *this; }
    Stm32Serial(const Stm32Serial&) {}

    Stm32Serial();
    ~Stm32Serial();

    static Stm32Serial instance;    /*!< Lazy singleton instance */
    unsigned char TIC_rxBuffer[256];    /*!< Internal serial reception buffer */
    unsigned int TIC_rxBufferLen;   /*!< Length of valid data bytes in the above buffer */
    UART_HandleTypeDef huart;  /*!< Internal STM32 low level UART handle */
};

#endif