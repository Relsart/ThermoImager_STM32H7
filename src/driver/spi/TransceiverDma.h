#pragma once
#include <stdint.h>
#include "etl/atomic.h"

#include "driver/dma/Stream.h"
#include "Signal.h"
#include "stm32h7xx.h"
#include "DataTypes.h"

#include "Loger.h"
#include "driver/Dwt.h"

namespace driver {
namespace spi {

class Spi;

/**
 * @brief SPI transmitter-receiver for DMA streams
 * @details Slot for DMA interruptions (DEPRECATED)
 */
class DmaTransceiver : public SlotInterface <driver::dma::SignalPack>
{
private:
    Spi& m_maintainer;                  // Link to maintainer class
    SPI_TypeDef* const m_spi;           // SPI periphery
    dma::Stream* m_txStream;            // DMA transmitter stream
    dma::Stream* m_rxStream;            // DMA receiver stream
    etl::atomic<bool> m_busy{false};    // Busy stream flag

    /**
     * @brief Saved state of tx/rx transaction
     */
    struct StreamInfo
    {
        uint32_t remainSize = 0;    // Size of remained data
        uint32_t nextAddr = 0;      // Next address
        uint8_t incSize = 0;        // Increment size in bytes
        uint32_t options = 0;       // Additional option bits (see OptionFlags)
    } m_txState, m_rxState;

    /**
     * @brief Type of current operation
     */
    enum class OperType {Idle, Receive, Transmit, Duplex};
    OperType m_currentTask = OperType::Idle;

    /**
     * @brief One TX transaction (max 65535 data units)
     * @param [in] dataAddr Transmitting data address
     * @param [in] dataSize Transmitting units count
     * @param [in] inc DMA increment
     */
    void txTransaction(void* dataAddr, uint16_t dataSize, uint8_t inc);

    /**
     * @brief One RX transaction (max 65535 data units)
     * @param [in] buffAddr Receiving buffer address
     * @param [in] dataSize Receiving units count
     * @param [in] inc DMA increment
     */
    void rxTransaction(void* buffAddr, uint16_t dataSize, uint8_t inc);

    /**
     * @brief One TX-RX transaction (max 65535 data units)
     * @param [in] dataAddr Transmitting data address
     * @param [in] buffAddr Receiving buffer address
     * @param [in] dataSize Transmitting units count
     * @param [in] inc DMA increment
     */
    void txRxTransaction(void* dataAddr, void* buffAddr, uint16_t dataSize, uint8_t inc);

    /**
     * @brief DMA interruptions handler
     */
    void run(dma::SignalPack, uint32_t) override;

    /**
     * @brief Get frame size code for Spi configuration register from increment size
     * @details Only for 1/2/4-byte frames yet
     * @param [in] increment data increment value in bytes (1/2/4)
     * @return DSIZE value for SPI_CFG1 register
     */
    inline uint8_t calcFrameSize(uint8_t increment);

    inline bool checkUnitSize(uint8_t unitSize);

    inline bool waitForTransceiverRelease(DwtTimer::TimeoutData& timeoutParams);

public:
    /**
     * @brief Constructor
     * @param Link to holder class
     */
    DmaTransceiver(Spi& maintainer);

    /**
     * @brief Connect and configurate DMA transmitting/recceiving streams
     * @details Streams MAY NOT be configured. initialization will made in this class
     * @param [in] stream Dma stream pointer
     */
    void onTxStream(dma::Stream* stream);
    void onRxStream(dma::Stream* stream);

    /**
     * @brief Sends next data pack (if data not fit in one transaction) or finishes session
     * @details Handler for SPI EndOfTransfer interruption (in DMA mode)
     */
    void nextTransaction();

    /**
     * @brief Data transmitting via stream
     * @param [in] data Tx data pointer
     * @param [in] unitSize bytesize of one unit (1/2/4 bytes)
     * @param [in] dataCount Data count (in 8/16/32-bit units)
     * @param [in] options Option flags (see driver::spi::OptionFlags)
     */
    void transmit(void* data, uint8_t unitSize, uint32_t dataCount, uint32_t options);

    /**
     * @brief Data receiving via stream
     * @param [in] buffer Pointer for received data saving
     * @param [in] unitSize bytesize of one unit (1/2/4 bytes)
     * @param [in] dataCount Data count (in units) - max 0xFFFF data units (is multitransaction receiving necessary?)
     */
    void receive(void* buffer, uint8_t unitSize, uint32_t dataCount);

    /**
     * @brief Parallel data receiving/transmitting via stream
     * @param [in] data Tx data pointer
     * @param [in] buffer Pointer for received data saving
     * @param [in] unitSize bytesize of one unit (1/2/4 bytes)
     * @param [in] dataCount Data count (in units)
     */
    void transmitReceive(void* data, void* buffer, uint8_t unitSize, uint32_t dataCount);

    /**
     * @brief Is stream busy?
     */
    bool isBusy();
};

}   // namespace spi
}   // namespace driver
