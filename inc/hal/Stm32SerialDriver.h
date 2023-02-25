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
class Stm32SerialDriver {
public:
    /**
     * @brief Singleton instance getter
     * 
     * @return The singleton instance of this class
     */
    static Stm32SerialDriver& get();

    /**
     * @brief Initialize the serial link and start receiving data from it
     */
    void start();

    /**
     * @brief Reset the reception buffer overflow counter
     */
    void resetRxOverflowCount();

    /**
     * @brief Get the reception buffer overflow count
     * 
     * @param reset Shall we reset the overflow flag once returned?
     * @return The number of times the internal reception buffer overflowed since the last reset of this counter (incoming data bytes have been lost)
     */
    unsigned int getRxOverflowCount(bool reset = false);

    /**
     * @brief Get the total number of received bytes on the serial port since the last reset
     * 
     * @return The number of received bytes
     */
    unsigned long getRxBytesTotal() const;

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
    void pushReceivedByte(uint8_t incomingByte);

    /**
     * @brief Collect the new data bytes read from the serial link and store them in the caller's buffer
     * 
     * @param buffer The buffer where new data bytes will be written
     * @param maxLen The maximum number of bytes that can be stored in @p buffer (aka the storage size of buffer in bytes)
     * 
     * @return The number of bytes actually copied to @p buffer (will be <= maxLen, can be 0 if no data was available)
     */
    size_t read(uint8_t* buffer, size_t maxLen);

    /**
     * @brief Get the low-level serial link handler object
     * 
     * @return UART_HandleTypeDef* The STM32 low level UART hanlder
     */
    friend UART_HandleTypeDef* getTicUartHandle();

private:
    Stm32SerialDriver& operator= (const Stm32SerialDriver&) { return *this; }
    Stm32SerialDriver(const Stm32SerialDriver&) {}

    Stm32SerialDriver();
    ~Stm32SerialDriver();

    static Stm32SerialDriver instance;    /*!< Lazy singleton instance */
    unsigned char serialRxBuffer[256];    /*!< Internal serial reception buffer */
    unsigned int serialRxBufferLen;   /*!< Length of valid data bytes in the above buffer */
    unsigned int serialRxBufferOverflowCount;  /*!< How many times the serial reception buffer overflowed since last reset */
    unsigned long serialRxBytesTotal;   /*!< How many bytes were received since last reset? */
    UART_HandleTypeDef huart;  /*!< Internal STM32 low level UART handle */
};

#endif
