#include "Exti.h"
#include "stm32h7xx.h"
#include "Gpio.h"
#include "driver/nvic/NvicManager.h"

namespace driver {
namespace gpio {

void Exti::init (ExtiFront front)
{
    // initialization Nvic
    Nvic* nvic = NvicManager::getInstance().getGpioNvic(m_pinClass.m_pin);
    
    if (nvic)
    {
        IRQn_Type irq;
        if (NvicManager::getInstance().getIrqNumber(m_pinClass.m_pin, irq))
            nvic->init (irq, 10, *this);    // It has low priority
    }

    SET_BIT (RCC->APB4ENR, RCC_APB4ENR_SYSCFGEN);   // Enable clocking SysCfgController

    
    uint8_t setPort;    // Port value
    switch (reinterpret_cast<uint32_t>(m_pinClass.m_GPIOx))
    {
    case GPIOA_BASE: setPort = 0; break;
    case GPIOB_BASE: setPort = 1; break;
    case GPIOC_BASE: setPort = 2; break;
    case GPIOD_BASE: setPort = 3; break;
    case GPIOE_BASE: setPort = 4; break;
    case GPIOF_BASE: setPort = 5; break;    
    case GPIOG_BASE: setPort = 6; break;    
    }

    // Offset in EXTICR registers
    uint8_t offset;
    switch (m_pinClass.m_pin)
    {
    case 0:
    case 4:
    case 8:
    case 12:
        offset = 0; break;
    case 1:
    case 5:
    case 9:
    case 13:
        offset = 4; break;
    case 2:
    case 6:
    case 10:
    case 14:
        offset = 8; break;
    case 3:
    case 7:
    case 11:
    case 15:
        offset = 12; break;
    }

    // Port configuration in EXTICR multiplexor:
    if (m_pinClass.m_pin <= 3)           /* Pins 0..3 */
    {
        SET_BIT (SYSCFG->EXTICR[0], setPort << offset);
    }
    else if (m_pinClass.m_pin <= 7)      /* Pins 4..7 */
    {
        SET_BIT (SYSCFG->EXTICR[1], setPort << offset);
    }
    else if (m_pinClass.m_pin <= 11)     /* Pins 8..11 */
    {
        SET_BIT (SYSCFG->EXTICR[2], setPort << offset);
    }
    else                             /* Pins 12..15 */
    {
        SET_BIT (SYSCFG->EXTICR[3], setPort << offset);
    }

    // Interruptions enable for target channel:
    SET_BIT (EXTI_D1->IMR1, m_pinClass.m_pinPos);

    // Front detection configuration:
    if (front == ExtiFront::RisingFront)
    {
        SET_BIT (EXTI->RTSR1, m_pinClass.m_pinPos);
    }
    else if (front == ExtiFront::FailingFront)
    {
        SET_BIT (EXTI->FTSR1, m_pinClass.m_pinPos);
    }
    else if (front == ExtiFront::BothFronts)
    {
        SET_BIT (EXTI->RTSR1, m_pinClass.m_pinPos);
        SET_BIT (EXTI->FTSR1, m_pinClass.m_pinPos);
    }
    else if (front == ExtiFront::None)
    {
        CLEAR_BIT (EXTI->RTSR1, m_pinClass.m_pinPos);
        CLEAR_BIT (EXTI->FTSR1, m_pinClass.m_pinPos);
    }

    // Interruption reset:
    SET_BIT (EXTI_D1->PR1, m_pinClass.m_pinPos);
}

void Exti::run (uint8_t, uint32_t)
{
    // Checking for particular pin interruption activation (some pins have common interruption handlers):
    if (READ_BIT(EXTI_D1->PR1, m_pinClass.m_pinPos))
    {
        m_pinClass.extiSignal.activ (1);
        SET_BIT (EXTI_D1->PR1, m_pinClass.m_pinPos);    // Interruption reset
    }
}

}   // namespace gpio
}   // namespace driver
