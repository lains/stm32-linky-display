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

#endif // _STM32TIMERDRIVER_H_