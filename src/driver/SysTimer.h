#pragma once
#include <stdint.h>

/**
 * @brief This variable lives (and increments by SysTick_Handler) in Nvic.cpp
 */
extern volatile uint64_t msTicks;

namespace driver {
/**
 * @brief    System Timer functions. 
 * @details  They powered by SysTick interruptions. So for correct working they must be enabled.
 *           SysTick frequency adjusted by invoking SysTick_Config functiion in Platform.cpp
 *           if it necessary to use time delays or timeouts, independed from IRQ, use DWT timer.
 */   

/**
 * @brief Make delay
 * @param [in] ms milliseconds pause set
 */
void sysDelay(uint32_t ms);

/**
 * @brief Get value of SysTicker (in milliseconds)
 */
uint64_t getMsTicks();

/**
 * @brief Set value of SysTicker (in milliseconds)
 */
void setMsTicks(uint64_t ms);

}   // namespace driver
