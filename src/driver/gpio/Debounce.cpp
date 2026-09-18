#include "Debounce.h"

namespace driver {
namespace gpio {

Contact::Contact(GpioPort GPIOx, uint8_t pin) : m_GPIOx(GPIOx), m_pinPos(1 << pin) 
{}

void Contact::debounceScan() 
{
    if ((m_flagLow == 0) == ((m_GPIOx->IDR & m_pinPos) != 0)) // state not changed
    {
        if (m_filterCounter != 0) 
            m_filterCounter--;
    }
    else    // state changed
    {
        m_filterCounter++; 
        if (m_filterCounter >= m_filterSetpoint) 
        {
            m_flagLow = !m_flagLow;   // state inversion
            m_filterCounter = 0;  

            if (m_flagLow) 
                m_flagFalling = true;
            else 
                m_flagRising = true;
        }
    }
}

bool Contact::getLowFront()
{
    if (m_flagFalling)
    {
        m_flagFalling = false;
        return true;
    }
    return false;
}


bool Contact::getHighFront()
{
    if (m_flagRising)
    {
        m_flagRising = false;
        return true;
    }
    return false;
}


bool Contact::getFilteredState()
{
    return (!m_flagLow);
}

}   // namespace gpio
}   // namespace driver
