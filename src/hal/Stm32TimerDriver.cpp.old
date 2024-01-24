#include "Stm32TimerDriver.h"
extern "C" {
#include "stm32f4xx_hal.h"
}

void waitDelayAndCondition(uint32_t delay, FHalDelayRefreshFunc toRunWhileWaiting, FHalDelayConditionFunc conditionCheck, void* context) {
    uint32_t tickstart = HAL_GetTick();
    uint32_t wait = delay;

    /* Add a freq to guarantee minimum wait */
    if (wait < HAL_MAX_DELAY) {
        wait += (uint32_t)(uwTickFreq);
    }

    while((HAL_GetTick() - tickstart) < wait && (conditionCheck == nullptr || conditionCheck(context))) {
        if (toRunWhileWaiting != nullptr) {
            toRunWhileWaiting(context);
        }
    }
}

void waitDelay(uint32_t delay, FHalDelayRefreshFunc toRunWhileWaiting, void* context) {
    waitDelayAndCondition(delay, toRunWhileWaiting, nullptr, context);
}

Stm32MeasurementTimer::Stm32MeasurementTimer(bool start) :
    running(false),
    startTick(0)
{
    if (start)
        this->start();
}

void Stm32MeasurementTimer::reset() {
    this->running = false;
    this->startTick = 0;
}

void Stm32MeasurementTimer::start() {
    this->startTick = HAL_GetTick();
    this->running = true;
}

uint32_t Stm32MeasurementTimer::get() const {
    if (this->running) {
        uint32_t currentTick = HAL_GetTick();
        return (uint32_t)(currentTick - this->startTick);
    }
    else
        return 0; /* No measurement was performed */
}
