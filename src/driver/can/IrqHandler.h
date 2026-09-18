#pragma once

#include <stdint.h>
#include "DataTypes.h"
#include "driver/nvic/NvicManager.h"
#include "RingBuffer.h"

namespace driver {
namespace can {

class Receiver;
class Bus;

class IrqHandler : public SlotInterface<Irq>
{
private:
    friend class Receiver;
    friend class Bus;
    
    const uint32_t m_RxFifoAddr;                                // Address of Rx FIFO start in memory
    MessageRingBuffer<Message> m_buffer;                        // Buffer for incoming CAN messages
    volatile FDCAN_GlobalTypeDef* const m_can;                  // Address of CAN periphery
    #ifndef WITH_RTOS
    SignalMainLoop<MessageRingBuffer<Message>*> m_rxDataSignal; // Signal for receiving a new message
    SignalMainLoop<Message*> m_txFinishedSignal;                // Signal for finished transmitting 
    #endif
    void run (uint8_t, uint32_t) override;

    /**
     * @brief Re-init the bus in case of Bus-off status
     */
    void resetBus();

public:
    /**
     * @brief Constructor
     * @param [in] can pointer to periphery device
     * @param [in] fifoAddr address of Rx FIFO start in memory
     */
    IrqHandler(FDCAN_GlobalTypeDef* can, uint32_t fifoAddr);
};

}   // namespace uart
}   // namespace can
