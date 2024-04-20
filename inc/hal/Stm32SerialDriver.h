#ifndef _STM32SERIALDRIVER_H_
#define _STM32SERIALDRIVER_H_

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

/* Definition for the USART used to receive the TIC signal */
#if defined(USE_STM32469I_DISCOVERY) || defined(USE_STM32F769I_DISCO)
/**
 * USART6 GPIO Configuration
 * PC6     ------> USART6_TX
 * PC7     ------> USART6_RX
 */
/* Definition for USART6 HAL functions */
#define USART6_CLK_ENABLE()              __HAL_RCC_USART6_CLK_ENABLE()
#define USART6_CLK_DISABLE()             __HAL_RCC_USART6_CLK_DISABLE()
#define USART6_RX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOC_CLK_ENABLE()
#define USART6_TX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOC_CLK_ENABLE()
#define USART6_FORCE_RESET()             __HAL_RCC_USART6_FORCE_RESET()
#define USART6_RELEASE_RESET()           __HAL_RCC_USART6_RELEASE_RESET()
#define USART6_TX_PIN                    GPIO_PIN_6
#define USART6_TX_GPIO_PORT              GPIOC
#define USART6_TX_AF                     GPIO_AF8_USART6
#define USART6_RX_PIN                    GPIO_PIN_7
#define USART6_RX_GPIO_PORT              GPIOC
#define USART6_RX_AF                     GPIO_AF8_USART6

#define USART_TIC                        USART6
#define USART_TIC_CLK_ENABLE()           USART6_CLK_ENABLE()
#define USART_TIC_CLK_DISABLE()          USART6_CLK_DISABLE()
#define USART_TIC_RX_GPIO_CLK_ENABLE()   USART6_RX_GPIO_CLK_ENABLE()
#define USART_TIC_TX_GPIO_CLK_ENABLE()   USART6_TX_GPIO_CLK_ENABLE()
#define USART_TIC_FORCE_RESET()          USART6_FORCE_RESET()
#define USART_TIC_RELEASE_RESET()        USART6_RELEASE_RESET()
#define USART_TIC_TX_PIN                 USART6_TX_PIN
#define USART_TIC_TX_GPIO_PORT           USART6_TX_GPIO_PORT
#define USART_TIC_TX_AF                  USART6_TX_AF
#define USART_TIC_RX_PIN                 USART6_RX_PIN
#define USART_TIC_RX_GPIO_PORT           USART6_RX_GPIO_PORT
#define USART_TIC_RX_AF                  USART6_RX_AF
#define USART_TIC_IRQn                   USART6_IRQn
#endif

/* Definition for the USART forwarded to ST-Link's virtual com port */
#ifdef USE_STM32469I_DISCOVERY
/**
 * USART3 GPIO Configuration
 * PD8     ------> USART3_TX
 * PD9     ------> USART3_RX
 */
/* Definition for USART3 HAL functions */
#define USART3_CLK_ENABLE()              __HAL_RCC_USART3_CLK_ENABLE()
#define USART3_CLK_DISABLE()             __HAL_RCC_USART3_CLK_DISABLE()
#define USART3_RX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOB_CLK_ENABLE()
#define USART3_TX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOB_CLK_ENABLE()
#define USART3_FORCE_RESET()             __HAL_RCC_USART3_FORCE_RESET()
#define USART3_RELEASE_RESET()           __HAL_RCC_USART3_RELEASE_RESET()
#define USART3_TX_PIN                    GPIO_PIN_10
#define USART3_TX_GPIO_PORT              GPIOB
#define USART3_TX_AF                     GPIO_AF7_USART3
#define USART3_RX_PIN                    GPIO_PIN_11
#define USART3_RX_GPIO_PORT              GPIOB
#define USART3_RX_AF                     GPIO_AF7_USART3

#define USART_DBG                        USART3
#define USART_DBG_CLK_ENABLE()           USART3_CLK_ENABLE()
#define USART_DBG_CLK_DISABLE()          USART3_CLK_DISABLE()
#define USART_DBG_RX_GPIO_CLK_ENABLE()   USART3_RX_GPIO_CLK_ENABLE()
#define USART_DBG_TX_GPIO_CLK_ENABLE()   USART3_TX_GPIO_CLK_ENABLE()
#define USART_DBG_FORCE_RESET()          USART3_FORCE_RESET()
#define USART_DBG_RELEASE_RESET()        USART3_RELEASE_RESET()
#define USART_DBG_TX_PIN                 USART3_TX_PIN
#define USART_DBG_TX_GPIO_PORT           USART3_TX_GPIO_PORT
#define USART_DBG_TX_AF                  USART3_TX_AF
#define USART_DBG_RX_PIN                 USART3_RX_PIN
#define USART_DBG_RX_GPIO_PORT           USART3_RX_GPIO_PORT
#define USART_DBG_RX_AF                  USART3_RX_AF
#endif
#ifdef USE_STM32F769I_DISCO
/**
 * USART1 GPIO Configuration
 * PA9     ------> USART1_TX
 * PA10    ------> USART1_RX
 */
/* Definition for USART1 HAL functions */
#define USART1_CLK_ENABLE()              __HAL_RCC_USART1_CLK_ENABLE()
#define USART1_CLK_DISABLE()             __HAL_RCC_USART1_CLK_DISABLE()
#define USART1_RX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()
#define USART1_TX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()
#define USART1_FORCE_RESET()             __HAL_RCC_USART1_FORCE_RESET()
#define USART1_RELEASE_RESET()           __HAL_RCC_USART1_RELEASE_RESET()
#define USART1_TX_PIN                    GPIO_PIN_9
#define USART1_TX_GPIO_PORT              GPIOA
#define USART1_TX_AF                     GPIO_AF7_USART1
#define USART1_RX_PIN                    GPIO_PIN_10
#define USART1_RX_GPIO_PORT              GPIOA
#define USART1_RX_AF                     GPIO_AF7_USART1

#define USART_DBG                        USART1
#define USART_DBG_CLK_ENABLE()           USART1_CLK_ENABLE()
#define USART_DBG_CLK_DISABLE()          USART1_CLK_DISABLE()
#define USART_DBG_RX_GPIO_CLK_ENABLE()   USART1_RX_GPIO_CLK_ENABLE()
#define USART_DBG_TX_GPIO_CLK_ENABLE()   USART1_TX_GPIO_CLK_ENABLE()
#define USART_DBG_FORCE_RESET()          USART1_FORCE_RESET()
#define USART_DBG_RELEASE_RESET()        USART1_RELEASE_RESET()
#define USART_DBG_TX_PIN                 USART1_TX_PIN
#define USART_DBG_TX_GPIO_PORT           USART1_TX_GPIO_PORT
#define USART_DBG_TX_AF                  USART1_TX_AF
#define USART_DBG_RX_PIN                 USART1_RX_PIN
#define USART_DBG_RX_GPIO_PORT           USART1_RX_GPIO_PORT
#define USART_DBG_RX_AF                  USART1_RX_AF
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
     * 
     * @param baudrate The baudrate to use on the serial port
     */
    void start(uint32_t baudrate);

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

#endif // _STM32SERIALDRIVER_H_
