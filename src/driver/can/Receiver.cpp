#include "Receiver.h"

namespace driver {
namespace can {

Receiver::Receiver(FDCAN_GlobalTypeDef* can, uint32_t fifoAddr) : m_irqHandler(can, fifoAddr)
{
    #ifndef WITH_RTOS
    m_irqHandler.m_rxDataSignal.connect(this);
    #endif
}

void Receiver::run(MessageRingBuffer<Message>* buffer, uint32_t)
{
    if (!buffer)
        return;

    Message msg{0};
    while (buffer->read(msg))
    {
        for (auto filter = m_filtersList.begin(); filter != m_filtersList.end(); filter++)
        {
            if (filter->check(msg.id))
                filter->m_signal.activ(&msg);
        }
    }
}

}   // namespace uart
}   // namespace can
