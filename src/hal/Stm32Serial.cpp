#include "Stm32Serial.h"
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

/* Definition for USARTx Pins */
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

    if (huart->Instance!=USART6 /*&& huart->Instance!=USART3*/) {
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
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *huart) {
    /*##-1- Reset peripherals ##################################################*/
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
}

inline void UART6_Enable_interrupt_callback(UART_HandleTypeDef* huart) {
    HAL_UART_Receive_IT(huart, UART6_rxBuffer, 1);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart)
{
    if (huart->Instance==USART6) {
        unsigned char Received_Data = UART6_rxBuffer[0];
        onTicUartRx((uint8_t)Received_Data);
        BSP_LED_Toggle(LED3); // Toggle the orange LED when new serial data is received on the TIC UART
        UART6_Enable_interrupt_callback(huart);
    }
}
} // extern "C"

Stm32Serial::Stm32Serial() :
serialRxBufferLen(0),
serialRxBufferOverflowed(false) {
    memset(this->serialRxBuffer, 0, sizeof(this->serialRxBuffer));
}

Stm32Serial Stm32Serial::instance=Stm32Serial();


Stm32Serial::~Stm32Serial() {
}

Stm32Serial& Stm32Serial::get() {
    return Stm32Serial::instance;
}

void Stm32Serial::start() {
    MX_USART6_UART_Init(&(this->huart));
    UART6_Enable_interrupt_callback(&(this->huart));
}

void Stm32Serial::resetRxOverflowFlag() {
    uint32_t primask = __get_PRIMASK(); /* Read PRIMASK register, will contain 0 if interrupts are enabled, or non-zero if disabled */
    __disable_irq();
    this->serialRxBufferOverflowed = false;
    if (!primask) {
        __enable_irq();
    }
}

bool Stm32Serial::getRxOverflowFlag(bool reset) {
    uint32_t primask = __get_PRIMASK(); /* Read PRIMASK register, will contain 0 if interrupts are enabled, or non-zero if disabled */
    __disable_irq();
    bool result = this->serialRxBufferOverflowed;
    if (reset) {
        this->resetRxOverflowFlag();
    }
    if (!primask) {
		__enable_irq();
	}
    return result;
}

void Stm32Serial::pushReceivedByte(uint8_t incomingByte) {
    /* This code is called in an interrupt context */
    this->serialRxBytesTotal++;
    this->serialRxBuffer[this->serialRxBufferLen] = incomingByte;
    this->serialRxBufferLen++;
    if (this->serialRxBufferLen >= sizeof(this->serialRxBuffer)) {
        this->serialRxBufferOverflowed = true;
        this->serialRxBufferLen = 0;	/* FIXME: Wrap around in case of buffer overflow */
    }
}

unsigned long Stm32Serial::getRxBytesTotal() const {
    unsigned long result;
    uint32_t primask = __get_PRIMASK(); /* Read PRIMASK register, will contain 0 if interrupts are enabled, or non-zero if disabled */
    __disable_irq();
    result = this->serialRxBytesTotal;
    if (!primask) {
        __enable_irq();
    }
    return result;
}
size_t Stm32Serial::read(uint8_t* buffer, size_t maxLen) {
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

void Stm32Serial::writeByteHexdump(unsigned char byte) {
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

void Stm32Serial::print(const char* str) {
    if (HAL_UART_Transmit(&(this->huart), (uint8_t*)str, (uint16_t)strlen(str), 500)!= HAL_OK) {
        Error_Handler();
    }
}

#ifdef USE_ALLOCATION
void Stm32Serial::print(const std::string& str) {
    this->print(str.c_str());
}
#endif

UART_HandleTypeDef* getTicUartHandle() {
    return &(Stm32Serial::get().huart);
}

void onTicUartRx(uint8_t incomingByte) {
    Stm32Serial::get().pushReceivedByte(incomingByte);
}

extern "C" {
UART_HandleTypeDef* get_huart6() {
    return getTicUartHandle();
}
} // extern "C"