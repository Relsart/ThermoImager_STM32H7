#pragma once

#include "stm32h7xx.h"
#include "Interface.h"
#include "driver/nvic/NvicManager.h"
#include "Transceiver.h"
#include "TransceiverDma.h"
#include "RingBuffer.h"
#include "driver/Rcc.h"
#include "Signal.h"
#include "etl/map.h"

namespace driver {
namespace uart {

/**
 * @brief UART base driver class
 * @details Slot for subscribers signals sending data to UART
 */
class Port : public i_face::I_Uart, public SlotInterface <uint8_t*>
{
public:
    friend class DmaTransceiver;
    friend class Transceiver;

    /**
     * @brief Constructor
     * @param [in] uart pointer to periphery device
     * @param [in] rxSize size of receiver ring buffer in bytes
     */
    Port(USART_TypeDef* uart, i_face::I_RingBuffer& rxBuff);

    /**
     * @brief UART initialization
     * @param [in] speed Baudrate
     * @param [in] parity Parity checking
     * @todo initialization for other configs: bits count, stop-bit etc...
     */
    void init(uint32_t speed, Parity parity = Parity::None);

    /**
     * @brief Send data to port
     * @details Depends on mode: general or DMA
     */
    void transmit(void* data, uint32_t size) override;

    /**
     * @brief Start receiving data via DMA stream
     * @param [in] size size of reading data
     * @param [in] circular Circular (endless) or single-pack receive transaction
     */
    void receiveViaDma(uint16_t size, bool circular) override;

    /**
     * @brief Subscribing client to reseiving Callback or Signals (Main Loop handler or Immediately activating)
     * @note  ACHTUNG: It can be connected only one signal or callback: Real or Offline (Deferred Interrupt Processing)! 
     *                 Otherwise the second signal handler may get empty (or changed) data in ring buffer
     * @param [in] slot Target clients slot
     */
    void setSlotRx(SlotInterface<i_face::I_RingBuffer*>*slot);
    void setSlotInstantRx(SlotInterface<i_face::I_RingBuffer*>*slot);
    void setRxCallback(RxCallback h) override;

    /**
     * @brief Set receiving mode (PIO or DMA)
     */
    void setRxMode(Mode mode) override;

    /**
     * @brief Set transmitting mode (PIO or DMA)
     */
    void setTxMode(Mode mode) override;

    /**
     * @brief Connect DMA Tx stream
     */
    void onTxStream(driver::dma::Stream* stream);

    /**
     * @brief Connect DMA Rx stream
     */
    void onRxStream(driver::dma::Stream* stream);

    /**
     * @brief Set DMA receiving parameters: allocation size for Rx buffer
     * @param [in] bufferSize buffer size in bytes
     * @param [in] doubleBufMode flag for allocation 2 buffers * size (double-buffered receiving mode)
     */
    void setDmaRxBufferSize (uint32_t bufferSize, bool doubleBufMode = false);

    /**
     * @brief Get busy flag of DMA transmitter
     * @details If timeout sets to 0 it's simple busy flag getter. 
     *          Otherwise (timeoutMs > 0) function can wait for DMA Tx stream to be released.
     * @param [in] timeoutMs Timeout in milliseconds
     * @return True == Busy
     */
    bool dmaTxIsBusy(uint32_t timeoutMs = 0);

    /**
     * @brief Set NVIC priority
     */
    void setNvicPriority(uint16_t priority);

    /**
     * @brief Get link to Receiver ring buffer
     */
    i_face::I_RingBuffer& getBuffer() override;

private:
    USART_TypeDef* const m_uart;            // UART base pointer
    const uint32_t rxRingBufferSize = 0;    // Size of receiving ring buffer
    i_face::I_RingBuffer& m_rxRingBuffer;   // Receiving data ring buffer
    uint32_t m_baudrate = 0;                // Port speed
    Mode m_rxMode = Mode::Pio;              // Receiving mode (general/DMA)
    Mode m_txMode = Mode::Pio;              // Transmitting mode (general/DMA)
    DmaTransceiver m_dmaTransceiver;        // DMA transceiver
    Transceiver m_transceiver;              // General tranceiver
    Nvic* nvic;

    #ifndef WITH_RTOS
    SignalMainLoop <i_face::I_RingBuffer*> m_signalRx;    // Received data signal (handling in Main Loop, outside the interrupt handler)
    #endif
    Signal <i_face::I_RingBuffer*> m_signalInstantRx;     // Received data signal (immediately handling)
    RxCallback m_rxCallback;                    // Received data callback

    static etl::map<uint8_t, Port*, 10> m_registeredUarts;    // Info about all registered ports

    /**
     * @brief Data transmitting signals handler
     */
    void run(uint8_t* data, uint32_t size) override;

    /**
     * @brief Count and set configuration for target baudrate
     * @details Only for initialization internal usage!
     */
    void configBaudrate(uint32_t baudrate);
};

}   // namespace uart
}   // namespace driver
