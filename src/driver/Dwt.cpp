#include "Dwt.h"
#include "stm32h7xx.h"

#define SCB_DEMCR  *(volatile unsigned long *) 0xE000EDFC
#define DWT_CONTROL *(volatile unsigned long *) 0xE0001000

namespace driver {

DwtTimer::DwtTimer()
{
    SET_BIT(SCB_DEMCR, CoreDebug_DEMCR_TRCENA_Msk);   // Enable DWT
    SET_BIT(DWT_CONTROL, DWT_CTRL_CYCCNTENA_Msk);     // Start DWT
    DWT->CYCCNT = 0U;
}

DwtTimer& DwtTimer::getInstance ()
{
    static DwtTimer self;
    return self;
}

DwtTimer::TimeoutData DwtTimer::timeoutInit(uint32_t ms)
{
    TimeoutData params;
    params.timeoutTicks = ms * (SystemCoreClock / 1000);
    params.lastDwtCount = DWT->CYCCNT; // init value
    return params;
}

bool DwtTimer::checkTimeout(TimeoutData& params)
{
    if (params.timeoutTicks > 0)
    {
        // Calculate how many ticks elapsed from last scan:
        uint32_t currentTick = DWT->CYCCNT;
        uint32_t elapsedTicks = 0;
        if (currentTick < params.lastDwtCount) // DWT counter was re-loaded
            elapsedTicks = max32 - params.lastDwtCount + 1 + currentTick;
        else
            elapsedTicks = currentTick - params.lastDwtCount;
        params.lastDwtCount = currentTick;
        if (params.timeoutTicks > 0)
        params.timeoutTicks -= ((params.timeoutTicks >= elapsedTicks) ? elapsedTicks : params.timeoutTicks);
    }
    return (params.timeoutTicks == 0);
}

void DwtTimer::ticksDelay()
{
    uint32_t lastTick = DWT->CYCCNT;
    uint32_t currentTick;
    uint32_t elapsedTicks = 0;

    while (m_ticksLeft > 0)
    {
        currentTick = DWT->CYCCNT;
        if (currentTick < lastTick) // DWT counter was re-loaded
            elapsedTicks = max32 - lastTick + 1 + currentTick;
        else
            elapsedTicks = currentTick - lastTick;
        lastTick = currentTick;
        m_ticksLeft -= ((m_ticksLeft >= elapsedTicks) ? elapsedTicks : m_ticksLeft);
    }
}

void DwtTimer::delayMs(uint32_t ms)
{
    m_ticksLeft = ms * (SystemCoreClock / 1000);
    ticksDelay();
}

void DwtTimer::delayUs(uint32_t us)
{
    m_ticksLeft = us * (SystemCoreClock / 1000000);
    ticksDelay();
}

}   // namespace driver
