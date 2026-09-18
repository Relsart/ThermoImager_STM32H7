#pragma once

#include <stdint.h>
#include "Signal.h"
#include "DataTypes.h"

namespace driver {
namespace can {

class Receiver;
class Bus;

class Filter
{
private:
    friend class Receiver;
    friend class Bus;

    const uint32_t m_id;        // CAN id
    const uint32_t m_mask;      // Mask
    Signal<Message*> m_signal;  // Receiver data signal

public:
    /**
     * @brief Constructor
     */
    Filter(uint32_t id, uint32_t mask) : m_id(id), m_mask(mask)
    {}

    /**
     * @brief Checking the filter activation
     * @param id Can ID
     * @return True == this filter activated
     */
    bool check(uint32_t id)
    {
        return (id & m_mask) == m_id;
    }

    void connect(SlotInterface<Message*>*slot)
    {
        if (slot)
            m_signal.connect(slot);
    }
};

}   // namespace uart
}   // namespace can
