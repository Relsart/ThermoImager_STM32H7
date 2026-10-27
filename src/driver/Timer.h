#pragma once

#include "stm32h7xx.h"
#include "nvic/NvicManager.h"
#include "Signal.h"

namespace driver {
namespace timer {

class Timer : public SlotInterface<Irq>
{
public:
    Timer(TIM_TypeDef* timerAddr);

private:
    TIM_TypeDef* const m_timerPeriph;

    /**
     * @brief Timer/Counter interruptions handler
     */
    void run(Irq, uint32_t) override;

};

}   // namespace timer
}   // namespace driver
