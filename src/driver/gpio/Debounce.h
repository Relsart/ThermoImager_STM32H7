#pragma once
#include <stdint.h>
#include "DataTypes.h"

namespace driver {
namespace gpio {

/**
 * @brief Debounce for electrical contact input
 */
class Contact
{
private:
    const GpioPort m_GPIOx;           // Port (GPIOA, GPIOB...)  
    const uint32_t m_pinPos;          // Pin mask (1 << pin)
    uint32_t m_filterSetpoint = 200;  // Switching setpoint for filter
    uint32_t m_filterCounter = 100;   // Filter counter
    bool m_flagLow = false;           // low-lewel flag
    bool m_flagRising = false;        // Rising front trigger flag
    bool m_flagFalling = false;       // Falling front trigger flag

public:
    /**
     * @brief Constructor
     */
    Contact(GpioPort GPIOx, uint8_t pin);

    /**
     * @brief Scan pin state and calculate debounced value
     * @details Invoked periodical, in main loop, timer handler or somewhere else
     */
    void debounceScan();

    /**
     * @brief Get rising-front (falling-front) trigger values
     * @details Once-working. Invoking this method resets trigger flag (until it's changes state again)
     */
    bool getHighFront();
    bool getLowFront();

    /**
     * @brief Get filtered pin state
     */
    bool getFilteredState();
};

}   // namespace gpio
}   // namespace driver
