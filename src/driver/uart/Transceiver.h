#pragma once
#include <stdint.h>
#include "RingBuffer.h"
#include "Signal.h"
#include "stm32h7xx.h"

namespace driver {
namespace uart {

class Port;

/**
 * @brief Uart transmitter-receiver
 * @details Slot for UART interruptions
 */
class Transceiver : public SlotInterface<uint8_t>
{
private:
    friend class Port;
    Port& m_holder;                 // Link to holder (Uart) class
    USART_TypeDef* const m_uart;    // Periphery device address
    uint8_t* m_txData = nullptr;    // Saved pointer to transmitting data
    uint32_t m_txDataSize = 0;      // Saved transmitting data size
    
    /**
     * @brief UART interruptions handler
     */
    void run(uint8_t data, uint32_t size) override;

public:
    static constexpr uint32_t icrMask = 0x123BFF;   // Resetting all interruption flags mask

    /**
     * @brief Constructor
     * @param Link to holder class
     */
    Transceiver(Port& holder);

    /**
     * @brief Send data to UART
     */
    void transmit(uint8_t* data, uint32_t size);
};

}   // namespace uart
}   // namespace driver
