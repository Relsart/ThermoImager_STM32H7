#include "Timer.h"

namespace driver {
namespace timer {

Timer::Timer(TIM_TypeDef* timerAddr) : m_timerPeriph(timerAddr)
{

}

void Timer::run(Irq, uint32_t)
{
    
}

}   // namespace timer
}   // namespace driver
