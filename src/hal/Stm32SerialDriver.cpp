#include "Stm32SerialDriver.h"
extern "C" {
#include "main.h"
}

static uint8_t UART_TIC_rxBuffer[1] = {0};   /* Our incoming serial buffer, filled-in by the receive interrupt handler */
static void onTicUartRx(uint8_t incomingByte);

extern "C" {

static void MX_USART_TIC_UART_Init(UART_HandleTypeDef* huart) {
    huart->Instance = USART_TIC;
    huart->Init.BaudRate = 1200;
    huart->Init.WordLength = UART_WORDLENGTH_8B;  // Note 7bits+parity bit
    huart->Init.StopBits = UART_STOPBITS_1;
    huart->Init.Parity = UART_PARITY_EVEN;
    huart->Init.Mode = UART_MODE_TX_RX;
    huart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    // if (HAL_UART_DeInit(huart) != HAL_OK) {
    //     OnError_Handler(1);
    // }
    if (HAL_UART_Init(huart) != HAL_OK) {
	      OnError_Handler(1);
    }
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
    /* FIXME: use USE_HAL_UART_REGISTER_CALLBACKS, and register different callbacks for USART_TIC and USART_DBG for both MspInit and MspDeInit */
    GPIO_InitTypeDef GPIO_InitStruct;
    memset(&GPIO_InitStruct, 0, sizeof(GPIO_InitStruct));

    if (huart->Instance!=USART_TIC && huart->Instance!=USART_DBG) { /* Initializing USART3 leads to a pinkish display on STM32F7, there is a probably a conflict on PINs */
        return;
    }

    if (huart->Instance==USART_TIC) {
        USART_TIC_TX_GPIO_CLK_ENABLE();
        USART_TIC_RX_GPIO_CLK_ENABLE();

        /* Peripheral clock enable */
        USART_TIC_CLK_ENABLE();

        GPIO_InitStruct.Pin = USART_TIC_TX_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
        GPIO_InitStruct.Alternate = USART_TIC_TX_AF;
        HAL_GPIO_Init(USART_TIC_TX_GPIO_PORT, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = USART_TIC_RX_PIN;
        GPIO_InitStruct.Alternate = USART_TIC_RX_AF;
        HAL_GPIO_Init(USART_TIC_RX_GPIO_PORT, &GPIO_InitStruct);

        /* USART_TIC interrupt Init */
        HAL_NVIC_SetPriority(USART_TIC_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(USART_TIC_IRQn);
    }
    else if (huart->Instance==USART_DBG) {
        USART_DBG_TX_GPIO_CLK_ENABLE();
        USART_DBG_RX_GPIO_CLK_ENABLE();

        /* Peripheral clock enable */
        USART_DBG_CLK_ENABLE();

        GPIO_InitStruct.Pin       = USART_DBG_TX_PIN;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_PULLUP;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.Alternate = USART_DBG_TX_AF;
        HAL_GPIO_Init(USART_DBG_TX_GPIO_PORT, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = USART_DBG_RX_PIN;
        GPIO_InitStruct.Alternate = USART_DBG_RX_AF;
        HAL_GPIO_Init(USART_DBG_RX_GPIO_PORT, &GPIO_InitStruct);
    }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *huart) {
    /*##-1- Reset peripherals ##################################################*/
    if (huart->Instance!=USART_TIC && huart->Instance!=USART_DBG) {
        return;
    }

    if (huart->Instance==USART_TIC) {
        /* Peripheral clock disable */
        USART_TIC_FORCE_RESET();
        USART_TIC_RELEASE_RESET();

        /*##-2- Disable peripherals and GPIO Clocks #################################*/
        USART_TIC_CLK_DISABLE(); // Or __HAL_RCC_USART1_CLK_DISABLE();
        /* De-Initialize USART TIC Tx and RX */
        HAL_GPIO_DeInit(USART_TIC_TX_GPIO_PORT, USART_TIC_TX_PIN);
        HAL_GPIO_DeInit(USART_TIC_RX_GPIO_PORT, USART_TIC_RX_PIN);
        /* USART_TIC interrupt DeInit */
        HAL_NVIC_DisableIRQ(USART_TIC_IRQn);
    }
    else if (huart->Instance==USART_DBG) {
        /* Peripheral clock disable */
        USART_DBG_FORCE_RESET();
        USART_DBG_RELEASE_RESET();

        /*##-2- Disable peripherals and GPIO Clocks #################################*/
        USART_DBG_CLK_DISABLE();
        /* De-Initialize USART_DBG Tx and RX */
        HAL_GPIO_DeInit(USART_DBG_TX_GPIO_PORT, USART_DBG_TX_PIN);
        HAL_GPIO_DeInit(USART_DBG_RX_GPIO_PORT, USART_DBG_RX_PIN);
    }
}

inline void UART_TIC_Enable_interrupt_callback(UART_HandleTypeDef* huart) {
    HAL_UART_Receive_IT(huart, UART_TIC_rxBuffer, 1);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart)
{
    if (huart->Instance==USART_TIC) {
        unsigned char Received_Data = UART_TIC_rxBuffer[0];
        onTicUartRx((uint8_t)Received_Data);
#ifdef LED_SERIAL_RX
        BSP_LED_Toggle(LED_SERIAL_RX); // Toggle the orange LED when new serial data is received on the TIC UART
#endif
        UART_TIC_Enable_interrupt_callback(huart);
    }
}
} // extern "C"

Stm32SerialDriver::Stm32SerialDriver() :
serialRxBufferLen(0),
serialRxBufferOverflowCount(0) {
    memset(this->serialRxBuffer, 0, sizeof(this->serialRxBuffer));
}

Stm32SerialDriver Stm32SerialDriver::instance=Stm32SerialDriver();


Stm32SerialDriver::~Stm32SerialDriver() {
}

Stm32SerialDriver& Stm32SerialDriver::get() {
    return Stm32SerialDriver::instance;
}

void Stm32SerialDriver::start() {
    MX_USART_TIC_UART_Init(&(this->huart));
    UART_TIC_Enable_interrupt_callback(&(this->huart));
}

void Stm32SerialDriver::resetRxOverflowCount() {
    uint32_t primask = __get_PRIMASK(); /* Read PRIMASK register, will contain 0 if interrupts are enabled, or non-zero if disabled */
    __disable_irq();
    this->serialRxBufferOverflowCount = 0;
    if (!primask) {
        __enable_irq();
    }
}

unsigned int Stm32SerialDriver::getRxOverflowCount(bool reset) {
    uint32_t primask = __get_PRIMASK(); /* Read PRIMASK register, will contain 0 if interrupts are enabled, or non-zero if disabled */
    __disable_irq();
    unsigned int result = this->serialRxBufferOverflowCount;
    if (reset) {
        this->resetRxOverflowCount();
    }
    if (!primask) {
		__enable_irq();
	}
    return result;
}

void Stm32SerialDriver::pushReceivedByte(uint8_t incomingByte) {
    /* This code is called in an interrupt context */
    this->serialRxBytesTotal++;
    this->serialRxBuffer[this->serialRxBufferLen] = incomingByte;
    this->serialRxBufferLen++;
    if (this->serialRxBufferLen >= sizeof(this->serialRxBuffer)) {
        this->serialRxBufferOverflowCount++;
        this->serialRxBufferLen = 0;	/* FIXME: Wrap around in case of buffer overflow */
    }
}

unsigned long Stm32SerialDriver::getRxBytesTotal() const {
    unsigned long result;
    uint32_t primask = __get_PRIMASK(); /* Read PRIMASK register, will contain 0 if interrupts are enabled, or non-zero if disabled */
    __disable_irq();
    result = this->serialRxBytesTotal;
    if (!primask) {
        __enable_irq();
    }
    return result;
}
size_t Stm32SerialDriver::read(uint8_t* buffer, size_t maxLen) {
    uint32_t primask = __get_PRIMASK(); /* Read PRIMASK register, will contain 0 if interrupts are enabled, or non-zero if disabled */
    __disable_irq();
    size_t copiedBytesCount = this->serialRxBufferLen;
    if (copiedBytesCount > 0) {
        if (copiedBytesCount > maxLen) {    /* Too many pending bytes, won't fit in the provided buffer */
            copiedBytesCount = maxLen;
        }
        /* FIXME: race condition with interrupt here */
        memcpy(static_cast<void *>(buffer), static_cast<void *>(this->serialRxBuffer), copiedBytesCount);
        memmove(static_cast<void *>(this->serialRxBuffer), static_cast<void *>(this->serialRxBuffer + copiedBytesCount), this->serialRxBufferLen - copiedBytesCount);
        this->serialRxBufferLen -= copiedBytesCount;
    }
    if (!primask) {
        __enable_irq();
    }
    return copiedBytesCount;
}

void Stm32SerialDriver::writeByteHexdump(unsigned char byte) {
    char msg[]="0x@@";
    unsigned char nibble;
    nibble = ((byte >> 4) & 0xf);
    msg[2]=(nibble<=9)?'0'+nibble:nibble-10+'a';
    nibble = (byte & 0xf);
    msg[3]=(nibble<=9)?'0'+nibble:nibble-10+'a';
    if (HAL_UART_Transmit(&(this->huart), (uint8_t*)msg, (uint16_t)strlen(msg), 500)!= HAL_OK) {
        Error_Handler();
    }
}

void Stm32SerialDriver::print(const char* str) {
    if (HAL_UART_Transmit(&(this->huart), (uint8_t*)str, (uint16_t)strlen(str), 500)!= HAL_OK) {
        Error_Handler();
    }
}

#ifdef USE_ALLOCATION
void Stm32SerialDriver::print(const std::string& str) {
    this->print(str.c_str());
}
#endif

UART_HandleTypeDef* getTicUartHandle() {
    return &(Stm32SerialDriver::get().huart);
}

void onTicUartRx(uint8_t incomingByte) {
    Stm32SerialDriver::get().pushReceivedByte(incomingByte);
}

extern "C" {
UART_HandleTypeDef* get_huart6() {
    return getTicUartHandle();
}
} // extern "C"
