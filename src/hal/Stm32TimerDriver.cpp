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

    while((HAL_GetTick() - tickstart) < wait && (conditionCheck != nullptr && conditionCheck(context))) {
        if (toRunWhileWaiting != nullptr) {
            toRunWhileWaiting(context);
        }
    }
}

void waitDelay(uint32_t delay, FHalDelayRefreshFunc toRunWhileWaiting, void* context) {
    waitDelayAndCondition(delay, toRunWhileWaiting, nullptr, context);
}
