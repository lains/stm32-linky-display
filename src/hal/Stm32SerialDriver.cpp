#include "Stm32SerialDriver.h"
extern "C" {
#include "main.h"
}

/* Definition for USARTx HAL functions */
#define USART6_CLK_ENABLE()              __HAL_RCC_USART6_CLK_ENABLE()
#define USART6_CLK_DISABLE()             __HAL_RCC_USART6_CLK_DISABLE()
#define USART6_RX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOC_CLK_ENABLE()
#define USART6_TX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOC_CLK_ENABLE()

#define USART6_FORCE_RESET()             __HAL_RCC_USART6_FORCE_RESET()
#define USART6_RELEASE_RESET()           __HAL_RCC_USART6_RELEASE_RESET()

#define USART1_CLK_ENABLE()              __HAL_RCC_USART1_CLK_ENABLE()
#define USART1_CLK_DISABLE()             __HAL_RCC_USART1_CLK_DISABLE()
#define USART1_RX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()
#define USART1_TX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()

#define USART1_FORCE_RESET()             __HAL_RCC_USART1_FORCE_RESET()
#define USART1_RELEASE_RESET()           __HAL_RCC_USART1_RELEASE_RESET()

/* Definition for USART1 Pins (forwarded to ST-Link's virtual com port) */
#define USART1_TX_PIN                    GPIO_PIN_9
#define USART1_TX_GPIO_PORT              GPIOA
#define USART1_TX_AF                     GPIO_AF7_USART1
#define USART1_RX_PIN                    GPIO_PIN_10
#define USART1_RX_GPIO_PORT              GPIOA
#define USART1_RX_AF                     GPIO_AF7_USART1

/* Definition for USART6 Pins */
#define USART6_TX_PIN                    GPIO_PIN_6
#define USART6_TX_GPIO_PORT              GPIOC
#define USART6_TX_AF                     GPIO_AF8_USART6
#define USART6_RX_PIN                    GPIO_PIN_7
#define USART6_RX_GPIO_PORT              GPIOC
#define USART6_RX_AF                     GPIO_AF8_USART6

static uint8_t UART6_rxBuffer[1] = {0};   /* Our incoming serial buffer, filled-in by the receive interrupt handler */
static void onTicUartRx(uint8_t incomingByte);

extern "C" {

static void MX_USART6_UART_Init(UART_HandleTypeDef* huart) {
    huart->Instance = USART6;
    huart->Init.BaudRate = 9600;
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
    GPIO_InitTypeDef GPIO_InitStruct;
    memset(&GPIO_InitStruct, 0, sizeof(GPIO_InitStruct));

    if (huart->Instance!=USART6 /*&& huart->Instance!=USART3*/) { /* Initializing USART3 leads to a pinkish display, there is a probably a conflict on PINs */
        return;
    }

    if (huart->Instance==USART6) {
        USART6_TX_GPIO_CLK_ENABLE();
        USART6_RX_GPIO_CLK_ENABLE();

        /* Peripheral clock enable */
        USART6_CLK_ENABLE();

        /**USART6 GPIO Configuration
        PC6     ------> USART6_TX
        PC7     ------> USART6_RX
        */
        GPIO_InitStruct.Pin = USART6_TX_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
        GPIO_InitStruct.Alternate = USART6_TX_AF;
        HAL_GPIO_Init(USART6_TX_GPIO_PORT, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = USART6_RX_PIN;
        GPIO_InitStruct.Alternate = USART6_RX_AF;
        HAL_GPIO_Init(USART6_RX_GPIO_PORT, &GPIO_InitStruct);

        /* USART6 interrupt Init */
        HAL_NVIC_SetPriority(USART6_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(USART6_IRQn);
    }
    else if (huart->Instance==USART1) {
        USART1_TX_GPIO_CLK_ENABLE();
        USART1_RX_GPIO_CLK_ENABLE();

        /* Peripheral clock enable */
        USART1_CLK_ENABLE();

        GPIO_InitStruct.Pin       = USART1_TX_PIN;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_PULLUP;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.Alternate = USART1_TX_AF;
        HAL_GPIO_Init(USART1_TX_GPIO_PORT, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = USART1_RX_PIN;
        GPIO_InitStruct.Alternate = USART1_RX_AF;
        HAL_GPIO_Init(USART1_RX_GPIO_PORT, &GPIO_InitStruct);
    }
}


void HAL_UART_MspDeInit(UART_HandleTypeDef *huart) {
    /*##-1- Reset peripherals ##################################################*/
    if (huart->Instance!=USART6 /*&& huart->Instance!=USART3*/) {
        return;
    }

    if (huart->Instance==USART6) {
        /* Peripheral clock disable */
        USART6_FORCE_RESET();
        USART6_RELEASE_RESET();

        /*##-2- Disable peripherals and GPIO Clocks #################################*/
        USART6_CLK_DISABLE(); // Or __HAL_RCC_USART1_CLK_DISABLE();
        /* De-Initialize USART6 Tx and RX */
        /**USART6 GPIO Configuration
        PC6     ------> USART6_TX
        PC7     ------> USART6_RX
        */
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_6|GPIO_PIN_7);
        /* USART6 interrupt DeInit */
        HAL_NVIC_DisableIRQ(USART6_IRQn);
    }
    else if (huart->Instance==USART1) {
        /* Peripheral clock disable */
        USART1_FORCE_RESET();
        USART1_RELEASE_RESET();

        /*##-2- Disable peripherals and GPIO Clocks #################################*/
        USART1_CLK_DISABLE();
        /* De-Initialize USART1 Tx and RX */
        /**USART6 GPIO Configuration
        PA9     ------> USART1_TX
        PA10    ------> USART1_RX
        */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);
    }
}

inline void UART6_Enable_interrupt_callback(UART_HandleTypeDef* huart) {
    HAL_UART_Receive_IT(huart, UART6_rxBuffer, 1);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart)
{
    if (huart->Instance==USART6) {
        unsigned char Received_Data = UART6_rxBuffer[0];
        onTicUartRx((uint8_t)Received_Data);
        BSP_LED_Toggle(LED2); // Toggle the green LED when new serial data is received on the TIC UART
        UART6_Enable_interrupt_callback(huart);
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
    MX_USART6_UART_Init(&(this->huart));
    UART6_Enable_interrupt_callback(&(this->huart));
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