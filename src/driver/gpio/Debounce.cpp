#include "Debounce.h"

namespace driver {
namespace gpio {

Contact::Contact(GpioPort GPIOx, uint8_t pin) : 
    m_GPIOx(GPIOx), 
    m_pinPos(1U << pin),
    m_FilterSetpoint(200),
    m_filterCounter(m_FilterSetpoint / 2),
    m_flagRising(false),
    m_flagFalling(false)
{}

void Contact::scan() 
{
    bool currentRawState = ((m_GPIOx->IDR & m_pinPos) != 0);    // Get physical raw level

    // Integrating: if HIGH move the counter up, if LOW, move it down:
    if (currentRawState) 
        if (m_filterCounter < m_FilterSetpoint) m_filterCounter++;
    else 
        if (m_filterCounter > 0) m_filterCounter--;

    // Thresholds checking:
    if (m_filterCounter == m_FilterSetpoint && !m_debouncedState) // Stable HIGH achieved
    {
        m_debouncedState = true;
        m_flagRising = true;    // Rising front fixed
    }
    else if (m_filterCounter == 0 && m_debouncedState)  // Stable LOW achieved
    {
        m_debouncedState = false;
        m_flagFalling = true;   // Falling front fixed
    }
}

bool Contact::getHighFront()
{
    // __disable_irq(); 
    if (m_flagRising.load())
    {
        m_flagRising.store(false);
        // __enable_irq();
        return true;
    }
    // __enable_irq();
    return false;
}

bool Contact::getLowFront()
{
    if (m_flagFalling.load())
    {
        m_flagFalling.store(false);
        return true;
    }
    return false;
}

bool Contact::getFilteredState()
{
    return m_debouncedState;
}

}   // namespace gpio
}   // namespace driver
