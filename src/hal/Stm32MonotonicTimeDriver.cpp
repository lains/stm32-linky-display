#include "Stm32MonotonicTimeDriver.h"
#include "Stm32DebugOutput.h" // FIXME: for debug only!

extern "C" {
#include "main.h"
}

extern "C" {
extern __IO uint32_t uwTick;
extern HAL_TickFreqTypeDef uwTickFreq;

/**
 * @brief This function overrides __weak void HAL_IncTick(void) in stm32fxxx_hal.c
 * 
 * @warning This code is running in interrupt context and HAL relies on uwTick to run properly. This has consequences:
 * - we should thus take as little time as possible
 * - we should not invoke and HAL delay here to avoid deadlocks
 */
void HAL_IncTick(void) { 
    static bool lastTickValid = false;
    static uint32_t lastTick;

    uwTick += uwTickFreq;
    Stm32MonotonicTimeDriver::get().millisecondsElapsed(uwTickFreq);
}
}

Stm32MonotonicTimeDriver::Stm32MonotonicTimeDriver() :
    htim(),
    suspended(true),
    initialized(false),
    millisecondsRemainder(0),
    onPeriodElapsedCallback(nullptr),
    onPeriodElapsedContext(nullptr) {
}

Stm32MonotonicTimeDriver Stm32MonotonicTimeDriver::instance=Stm32MonotonicTimeDriver();


Stm32MonotonicTimeDriver::~Stm32MonotonicTimeDriver() {
}

Stm32MonotonicTimeDriver& Stm32MonotonicTimeDriver::get() {
    return Stm32MonotonicTimeDriver::instance;
}

void Stm32MonotonicTimeDriver::start() {
    this->initialized = true;
    this->suspended = false;
}

/**
 * @brief Suspend tick
 */
void Stm32MonotonicTimeDriver::suspend() {
    this->suspended = true;
}

/**
 * @brief Resume tick
 */
void Stm32MonotonicTimeDriver::resume(void) {
    if (!this->initialized) {
        this->start();
    }
    if (this->suspended) {
        this->suspended = false;
    }
}

void Stm32MonotonicTimeDriver::millisecondsElapsed(unsigned int ms) {
    if (this->suspended)
        return;
    this->millisecondsRemainder += ms;
    if (this->millisecondsRemainder >= 1000) {
        if (this->onPeriodElapsedCallback) {
            this->onPeriodElapsedCallback(this->onPeriodElapsedContext);
        }
        this->millisecondsRemainder -= 1000;
    }
}

void Stm32MonotonicTimeDriver::setOnPeriodElapsed(FOnPeriodElapsedFunc toRunOnPeriodElapsed, void* context) {
    this->onPeriodElapsedCallback = toRunOnPeriodElapsed;
    this->onPeriodElapsedContext = context;
}

