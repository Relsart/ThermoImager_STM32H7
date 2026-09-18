#pragma once

#include <stdint.h>
#include "Filter.h"
#include "IrqHandler.h"
#include "Signal.h"
#include "RingBuffer.h"
#ifdef WITH_ETL
#include "etl/list.h"
#else
#include <list>
#endif

namespace driver {
namespace can {

class Bus;

class Receiver: public SlotInterface<MessageRingBuffer<Message>*>
{
public:
    friend class Bus;
    /**
     * @brief Constructor
     * @param [in] can pointer to periphery device
     * @param [in] fifoAddr address of FIFO start in memory
     */
    Receiver(FDCAN_GlobalTypeDef* can, uint32_t fifoAddr);

private:
    #ifdef WITH_ETL 
    etl::list<Filter, 100> m_filtersList;  // List of subscribed filters
    #else
    std::list<Filter> m_filtersList;
    #endif

    IrqHandler m_irqHandler;

    /**
     * @brief Incoming frames handler
     */
    void run(MessageRingBuffer<Message>* buffer, uint32_t) override;
};

}   // namespace uart
}   // namespace can
