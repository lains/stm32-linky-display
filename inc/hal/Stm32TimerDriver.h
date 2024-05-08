#ifndef _STM32TIMERDRIVER_H_
#define _STM32TIMERDRIVER_H_

#include <stdint.h>

typedef void(*FHalDelayRefreshFunc)(void* context);
typedef bool(*FHalDelayConditionFunc)(void* context);

/**
 * @brief Wait for a given delay in ms, while running a function in background. Stop waiting immediately if a condition becomes false
 * 
 * @param toRunWhileWaiting A function to run continously while waiting
 * @param conditionCheck A function returning false to immediately stop the wait period before timer elapses
 * @param context A context pointer provided to toRunWhileWaiting and/or conditionCheck as argument
 * @param delay The delay to wait for (in ms)
 * 
 */
void waitDelayAndCondition(uint32_t delay, FHalDelayRefreshFunc toRunWhileWaiting = nullptr, FHalDelayConditionFunc conditionCheck = nullptr, void* context = nullptr);

/**
 * @brief Wait for a given delay in ms, while running a function in background
 * 
 * @param toRunWhileWaiting A function to run continously while waiting
 * @param context A context pointer provided to toRunWhileWaiting as argument
 * @param delay The delay to wait for (in ms)
 */
void waitDelay(uint32_t delay, FHalDelayRefreshFunc toRunWhileWaiting = nullptr, void* context = nullptr);

/**
 * @brief Time measurement class
 */
class Stm32MeasurementTimer {
public:
    Stm32MeasurementTimer(bool start = false);

    /**
     * @brief Reset the timer
    */
    void reset();

    /**
     * @brief Start the timer
    */
    void start();

    /**
     * @brief Get the elapsed time in ms since start() was last invoked
     * 
     * @note We can handle 32-bit values max, thus 4294967s max, or roughly 1193. This should be enough however
    */
    uint32_t get() const;

private:
    bool running;   /*!< Timer starting point has been set, we can measure a duration */
    uint32_t startTick; /*!< The machine-specific tick that we recorded when starting the timer */
};

#endif // _STM32TIMERDRIVER_H_