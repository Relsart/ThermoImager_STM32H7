#pragma once
#include <stdint.h>
#include <etl/limits.h>

namespace driver {

/**
 * @brief    DWT timer manager
 * @details  DWT not depend on interruptions (unlike SysTick) and can be used where IRQ are disabled
 *           (or can't go to systick increment handler because of the current interruption priority)
 * 
 * @details  For safety own DWT counter never writes (only sets to zero in constructor)! 
 *           Timeout and delay functions only read it value and use own counters.
 *           Also it has no saved timeout counters here: every consumer creates and stores it, so
 *           it wouldn't be re-writed by other functions 
 * 
 * ACHTUNG:  For correct DWT working SystemCoreClock should have correct core clock frequency value
 */

class DwtTimer
{
private:
    DwtTimer();
    DwtTimer(DwtTimer&) = delete;

    static constexpr uint32_t max32 = etl::numeric_limits<uint32_t>::max();   // Max value of 32-bit unsigned variable 
    uint32_t m_ticksLeft;    // Number of DWT ticks for delays (decremental counter)

    /**
     * @brief Time delay in DWT ticks
     */
    void ticksDelay();

public:
    /**
     * @brief Data storage for timeout checking
     */
    struct TimeoutData
    {
        uint32_t timeoutTicks = 0;  // Value of timeout setting in DWT ticks
        uint32_t lastDwtCount = 0;  // Last counter value
    };

    /**
     * @brief Singleton
     */
    static DwtTimer& getInstance();

    /**
     * @brief Initialise tiomeout value in milliseconds
     * @param [in] ms timeout set value
     */
    TimeoutData timeoutInit(uint32_t ms);

    /**
     * @brief Check for timeout
     * @return True == set time has elapsed
     */
    bool checkTimeout(TimeoutData& params);

    /**
     * @brief Delays in milliseconds or microseconds (IRQ independed)
     */
    void delayMs(uint32_t ms);
    void delayUs(uint32_t us);
};

}   // namespace driver
