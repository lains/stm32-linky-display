#include "UARTDriver.h"
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
static unsigned char TIC_rxBuffer[256];
static unsigned int TIC_rxBufferLen = 0;

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

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart)
{
    unsigned char Received_Data = UART6_rxBuffer[0];

    TIC_rxBuffer[TIC_rxBufferLen] = Received_Data;
    TIC_rxBufferLen++;
    if (TIC_rxBufferLen>=sizeof(TIC_rxBuffer)) {
        TIC_rxBufferLen=0;	/* FIXME: Wrap around in case of buffer overflow */
    }

    BSP_LED_Toggle(LED2);
    HAL_UART_Receive_IT(huart, UART6_rxBuffer, 1);
}
} // extern "C"

TICUart::TICUart() : TIC_rxBufferLen(0) {
    memset(this->TIC_rxBuffer, 0, sizeof(this->TIC_rxBuffer));
}

TICUart TICUart::instance=TICUart();


TICUart::~TICUart() {
}

TICUart& TICUart::get() {
    return TICUart::instance;
}

void TICUart::start() {
    MX_USART6_UART_Init(&(this->huart));
    HAL_UART_Receive_IT(&(this->huart), UART6_rxBuffer, 1);
}

void TICUart::writeByteHexdump(unsigned char byte) {
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

UART_HandleTypeDef* getTicUartHandle() {
    return &(TICUart::get().huart);
}

extern "C" {
UART_HandleTypeDef* get_huart6() {
    return getTicUartHandle();
}
} // extern "C"