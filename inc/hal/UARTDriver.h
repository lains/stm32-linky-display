#ifndef _UARTDRIVER_H_
#define _UARTDRIVER_H_

#include "stm32f4xx_hal.h"
#include <cstdint>

extern "C" {
UART_HandleTypeDef* get_huart6(void);   // C-linkage exported getter for huart6 handler
}

class TICUart {
public:
    static TICUart& get();
    void start();
    void writeByteHexdump(unsigned char byte);

    friend UART_HandleTypeDef* getTicUartHandle();

private:
    TICUart& operator= (const TICUart&) { return *this; }
    TICUart(const TICUart&) {}

    TICUart();
    ~TICUart();

    static TICUart instance;    /*!< Lazy singleton instance */
    uint8_t UART6_rxBuffer[1] = {0};   /* Our incoming serial buffer, filled-in by the receive interrupt handler */
    unsigned char TIC_rxBuffer[256];
    unsigned int TIC_rxBufferLen;
    UART_HandleTypeDef huart;  /*!< Internal UART handle */
};

#endif
