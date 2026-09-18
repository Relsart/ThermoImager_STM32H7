#pragma once
#include <stdint.h>
#include "Signal.h"
#include "DataTypes.h"

namespace driver {
namespace gpio {
    
class Pin;

/**
 * @brief Slot for GPIO external interruptions (EXTI)
 */
class Exti : public SlotInterface <uint8_t>
{
private:
    Pin& m_pinClass;  // Holder base GPIO class

    /**
     * @brief GPIO EXTI interruptions handler
     */
    void run (uint8_t, uint32_t) override;

public:
    /**
     * @brief Constructor.
     * @param _pinClass Link to base holder class
     */
    Exti (Pin& pinClass) : m_pinClass (pinClass)
    {}

    /**
     * @brief initialization and enabling EXTI interruptions
     * @param front Detecting front
     */
    void init (ExtiFront front);
};

}   // namespace gpio
}   // namespace driver
