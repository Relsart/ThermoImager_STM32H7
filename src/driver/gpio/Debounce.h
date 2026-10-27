#pragma once
#include <stdint.h>
#include <etl/atomic.h>
#include "DataTypes.h"

namespace driver {
namespace gpio {

/**
 * @brief Debounce for electrical contact input
 * @details Based on integral filter (reversible counter)
 */
class Contact
{
private:
    const GpioPort m_GPIOx;             // Port (GPIOA, GPIOB...)  
    const uint32_t m_pinPos;            // Pin mask (1 << pin)
    const uint32_t m_FilterSetpoint;    // Switching setpoint for filter
    uint32_t m_filterCounter;           // The integrator ranges from 0 (pure LOW) to m_filterSetpoint (pure HIGH), starts strictly from the middle
    bool m_debouncedState = false;      // Current (filtered) pin state
    etl::atomic<bool> m_flagRising;     // Rising front trigger flag
    etl::atomic<bool> m_flagFalling;    // Falling front trigger flag

public:
    /**
     * @brief Constructor
     */
    Contact(GpioPort GPIOx, uint8_t pin);

    /**
     * @brief Scan pin state and calculate debounced value
     * @details Invoked periodical, in main loop, timer handler or somewhere else
     */
    void scan();

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
