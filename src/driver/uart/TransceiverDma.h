#pragma once
#include <stdint.h>
#include "driver/dma/Stream.h"
#include "RingBuffer.h"
#include "Signal.h"
#include "stm32h7xx.h"

namespace driver {
namespace uart {

class Port;

/**
 * @brief Uart transmitter-receiver for DMA streams
 * @details Slot for DMA interruptions
 */
class DmaTransceiver : public SlotInterface <driver::dma::SignalPack>
{
private:
    friend class Port;
    USART_TypeDef* const m_uart;      // Uart periphery address
    Port& m_holder;                   // Link to holder (Uart) class
    dma::Stream* m_txStream;          // DMA transmiter stream
    dma::Stream* m_rxStream;          // DMA receiver stream
    dma::StreamConfig m_txConfig;     // Transmitter stream saved configs (maybe not needed?)
    dma::StreamConfig m_rxConfig;     // Receiver stream saved configs
    uint8_t* m_rxBuffer = nullptr;    // Dynamic buffer for receiving data
    uint8_t* m_rxBuffer2 = nullptr;   // Pointer to receiving buffer2, using only in cycle double-buffered mode
    uint32_t m_bufferSize = 0;        // Allocated size of buffer (in bytes)
    uint32_t m_nextTxSize = 0;        // Saved counter of transmitting data
    uint8_t* m_nextTxData = nullptr;  // Saved address of transmitting data
    bool m_circRxMode = false;        // Circular receiving mode flag
    const uint32_t m_timeoutMs = 500; // Timeout for stream free

    /**
     * @brief DMA interruptions handler
     */
    void run(dma::SignalPack, uint32_t) override;

public:
    /**
     * @brief Constructor
     * @param Link to holder class
     */
    DmaTransceiver(Port& holder);

    /**
     * @brief Destructor
     */
    ~DmaTransceiver();

    /**
     * @brief Connect and configurate Rx/Tx DMA streams instances
     * @details Streams MAY NOT be configured! initialization will made in this class.
     */
    void onTxStream(dma::Stream* stream);
    void onRxStream(dma::Stream* stream);

    /**
     * @brief Allocate memory for receiving buffer
     * @param [in] size Buffer size in bytes
     * @param [in] doubleBufMode If true- sets to double buffered mode, and allocates size * 2 bytes in memory
     */
    void allocateRxBuffer(uint32_t size, bool doubleBufMode);

    /**
     * @brief Send data to UART via DMA stream
     */
    void transmit(uint8_t* data, uint32_t size);

    /**
     * @brief Start receiving data from UART via DMA stream
     * @param [in] size Data size to read from port (in bytes)
     * @param [in] circular Circular (endless) or single request 
     */
    void startReading(uint16_t size, bool circular);

    /**
     * @brief is transmitter busy
     */
    bool isTxBusy();
};

}   // namespace uart
}   // namespace driver
