#ifndef _STM32MONOTONICTIMEDRIVER_H_
#define _STM32MONOTONICTIMEDRIVER_H_

// We need the include below for TIM_HandleTypeDef
extern "C" {
#ifdef STM32F469xx
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"
#endif
#ifdef STM32F769xx
#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal_tim.h"
#endif
}

#include <stdint.h>

extern "C" {
TIM_HandleTypeDef* get_htim6(void);   // C-linkage exported getter for htim6 handler
}

// In order to implement the monotonic timer on STM32:
// Allocate a timer (revert patch release_tim6_to_reverse.patch)
// Re-use previous clock-adaptive code from DRV_Timer.c in DRV_Timer_at_ba80b2b5879b83208106c78457517f504febe924.c
/**
 * @brief Monotonic timer (time in seconds since startup) class (singleton)
 */
class Stm32MonotonicTimeDriver {
/* Methods */
public:
    typedef void(*FOnPeriodElapsedFunc)(void* context);

    /**
     * @brief Singleton instance getter
     * 
     * @return The singleton instance of this class
     */
    static Stm32MonotonicTimeDriver& get();

    /**
     * @brief Initialize the monotonic timer
     */
    void start();

    /**
     * @brief Suspend the monotonic timer
     * 
     * @note Once the timer is suspended, it can be resumed using method resume()
    */
    void suspend();

    /**
     * @brief Resume the previously suspended monotonic timer
     * 
     * @note If the timer has not been suspended, this method is a no-op
    */
    void resume();

    /**
     * @brief Select the function to run every time the programmed period elapses
     * 
     * @param toRunOnPeriodElapsed The function to run, and an argument to provide to that function
     * @param context An optional argument to provide to function toRunOnPeriodElapsed()
     * 
     * @warning The function provided as argument will be run in interruption context, it should not rely on HAL delays
     *          Work to be done regularly should be handled in a scheduler or main loop
     */
    void setOnPeriodElapsed(FOnPeriodElapsedFunc toRunOnPeriodElapsed = nullptr, void* context = nullptr);

    /**
     * @brief HAL's tick interrupt handler that will update our internal millisecond counter periodically
     */
    friend void HAL_IncTick(void);

private:
    Stm32MonotonicTimeDriver& operator= (const Stm32MonotonicTimeDriver&) { return *this; }
    Stm32MonotonicTimeDriver(const Stm32MonotonicTimeDriver&) {}

    Stm32MonotonicTimeDriver();
    ~Stm32MonotonicTimeDriver();

    /**
     * @brief Internal callback invoked after some time has passed
     * 
     * @param ms The number of milliseconds that have passed since last call
    */
    void millisecondsElapsed(unsigned int ms);
private:

/* Attributes */
    static Stm32MonotonicTimeDriver instance;    /*!< Lazy singleton instance */
    TIM_HandleTypeDef htim; /*!< Internal STM32 low level timer handle */
    bool suspended; /*!< Is the monotonic timer suspended? */
    bool initialized; /*!< Is the underlying hardware timer initialized? */
    unsigned int millisecondsRemainder; /*!< The number of milliseconds that have lapsed without invoking the onSecondElaPerioallback (<1000) */
    FOnPeriodElapsedFunc onPeriodElapsedCallback; /*!< Function to invoke every second */
    void* onPeriodElapsedContext; /*!< An optional context to pass to the function configured as onPeriodElapsed */
};

#endif // _STM32MONOTONICTIMEDRIVER_H_