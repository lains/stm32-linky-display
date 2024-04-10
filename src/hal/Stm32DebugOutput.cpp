#include "Stm32DebugOutput.h"
extern "C" {
#include "main.h"
}

Stm32DebugOutput::Stm32DebugOutput() : inError(true) {
    this->handle.Instance = nullptr;
}

void Stm32DebugOutput::start() {
    this->inError = false;
    this->handle.Instance = USART_DBG;
    this->handle.Init.BaudRate = 115200;
    this->handle.Init.WordLength = UART_WORDLENGTH_8B;
    this->handle.Init.StopBits = UART_STOPBITS_1;
    this->handle.Init.Parity = UART_PARITY_NONE;
    this->handle.Init.Mode = UART_MODE_TX_RX;
    this->handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    this->handle.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_DeInit(&(this->handle)); /* This will call HAL_UART_MspDeInit() */
    if (HAL_UART_Init(&(this->handle)) != HAL_OK) { /* This will call HAL_UART_MspInit() */
        this->inError = true;
        return;
    }
}
Stm32DebugOutput& Stm32DebugOutput::get() {
    return Stm32DebugOutput::instance;
}

bool Stm32DebugOutput::send(const uint8_t* buffer, unsigned int len) {
    if (this->inError) return false;
    HAL_StatusTypeDef result = HAL_UART_Transmit(&(this->handle), (uint8_t*)buffer, len, 5000);
    if (result != HAL_OK)
        this->inError = true;
    return this->inError;
}

bool Stm32DebugOutput::send(const char* text) {
    return this->send(reinterpret_cast<const uint8_t*>(text), strlen(text));
}

#ifdef USE_ALLOCATION
bool send(const std::string& text) {
    return this->send(text.c_str());
}
#endif

bool Stm32DebugOutput::send(unsigned int value) {
    char valueAsInt[10 + 1];
    char *firstNon0Digit = nullptr;
    char *currentDigit = valueAsInt + 10;
    *currentDigit = '\0'; /* Terminate the string */
    do {
        currentDigit--;
        *currentDigit = '0' + (value % 10);
        if (*currentDigit != '0') {
            firstNon0Digit = currentDigit;
        }
        value /= 10;
    } while (currentDigit != valueAsInt /* Check for underflow */);
    if (!firstNon0Digit) {
        valueAsInt[0] = '0';
        firstNon0Digit = valueAsInt;
        valueAsInt[1] = '\0';
    }
    return this->send(firstNon0Digit);
}

bool Stm32DebugOutput::hexdumpBuffer(const uint8_t* buffer, unsigned int len) {
    char byteHexDump[]="@@";
    unsigned char nibble;
    for (unsigned int offsByte=0; offsByte<len; offsByte++) {
        nibble = (((*buffer) >> 4) & 0xf);
        byteHexDump[0]=(nibble<=9)?'0'+nibble:nibble-10+'a';
        nibble = ((*buffer) & 0xf);
        byteHexDump[1]=(nibble<=9)?'0'+nibble:nibble-10+'a';
        if (offsByte != 0)
            this->send(" ");
        if (!this->send(byteHexDump))
            return false;
        offsByte++;
    }
    return true;
}

Stm32DebugOutput Stm32DebugOutput::instance=Stm32DebugOutput();