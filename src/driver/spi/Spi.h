#pragma once
#include <stdint.h>
#include "stm32h7xx.h"
#include "TransceiverDma.h"
#include "Signal.h"
#include "RingBuffer.h"
#include "driver/nvic/Nvic.h"
#include "DataTypes.h"

namespace driver {
namespace spi {

/**
 * @brief   SPI driver
 * @details Slot for IRQs signals
 */
class Spi : public SlotInterface <Irq>
{
private:
    friend class DmaTransceiver;
    SPI_TypeDef* const m_spi;           // Periphery device pointer
    DeviceMode m_mode;                  // Working mode (master/slave)
    TransferMethod m_transferMode;      // Transfer method (General/DMA)
    DmaTransceiver m_dmaTransceiver;    // DMA transceiver module
    #ifdef WITH_RTOS
    Signal<uint8_t> m_XferComplete;         // Transaction (via DMA) complete signal: RTOS version (subscribed slot shall use a Semaphore or something else..)
    #else
    SignalMainLoop<uint8_t> m_XferComplete; // Transaction (via DMA) complete signal: for Main Loop handling
    #endif

    /**
     * @brief SPI interruptions handler
     */
    void run(uint8_t, uint32_t) override;

    /**
     * @brief Stop SPI periphery
     * @param [in] timeoutParams saving timeout counter for DWT
     * @return True = Ok, it's stopped; False = timeout
     */
    bool stopSpi(DwtTimer::TimeoutData& timeoutParams);

    /**
     * @brief Transfer finishing
     */
    void stopTransfer();

    /**
     * @brief Data transmitting
     * @param [in] data Data
     * @param [in] datasize Count of data units
     * @param [in] options Option flags
     */
    template<typename Type>
    void sendData(Type* data, uint32_t datasize, uint32_t options);

    /**
     * @brief Data receiving
     * @details Receiving is always in blocks of 32 bits, because pointers operations ((__IO uint8_t*)&spi->RXDR) take much time,
     * and at high SPI frequencies (10 MHz) it can overflows FIFO with unread data. Reading in 32-bit words and layout in bytes (half-words) is faster.
     * @param [in] destPtr Address for data saving
     * @param [in] datasize Count of data units
     */
    template<typename Type>
    void readData(Type* destPtr, uint32_t datasize);

    /**
     * @brief Data transceiving (parallel read and write)
     * @param [in] dataTx Data for transmitting
     * @param [in] bufferRx Buffer for receiving
     * @param [in] datasize Exchanging data size
     */
    template<typename Type>
    void sendReadData(Type* dataTx, Type* bufferRx, uint16_t datasize);

public:
    /**
     * @brief Constructor
     */
    Spi(SPI_TypeDef* spi);

    /**
     * @brief SPI initialisation
     */
    void init(Config config);

    /**
     * @brief Subscribe to Transfer Complete Slot (for DMA receiving)
     * @details Connected functional will be a part of ISR, so it should be short and fast! 
     */
    void xferCmpltSubscribe(SlotInterface<uint8_t>*slot);

    /**
     * @brief Connect DMA transmitting/receiving streams
     */
    void onDmaTxStream(driver::dma::Stream* stream);
    void onDmaRxStream(driver::dma::Stream* stream);

    /**
     * @brief Get SCK frequency
     * @details Actual for Master
     */
    uint32_t getSckFreq();

    /**
     * @brief Transmit data to SPI in PIO mode (for 1-, 2- and 4-bytes packs)
     * @param [in] data Data
     * @param [in] datasize Count of data units
     * @param [in] options Additional options (driver::spi::OptionFlags)
     */
    void transmit8(uint8_t* data, uint32_t size = 1, uint32_t options = 0);
    void transmit16(uint16_t* data, uint32_t size = 1, uint32_t options = 0);
    void transmit32(uint32_t* data, uint32_t size = 1, uint32_t options = 0);

    /**
     * @brief Transmit data to SPI in DMA mode (for 1-, 2- and 4-bytes packs)
     * @param [in] data Data
     * @param [in] datasize Count of data units
     * @param [in] options Additional options (driver::spi::OptionFlags)
     */
    void transmit8viaDMA(uint8_t* data, uint32_t size = 1, uint32_t options = 0);
    void transmit16viaDMA(uint16_t* data, uint32_t size = 1, uint32_t options = 0);
    void transmit32viaDMA(uint32_t* data, uint32_t size = 1, uint32_t options = 0);

    /**
     * @brief Receive data from SPI in PIO mode (for 1-, 2- and 4-bytes packs)
     * @param [in] rxBuffer Address for received data saving
     * @param [in] size Count of data units
     */
    void receive8(uint8_t *rxBuffer, uint32_t size);
    void receive16(uint16_t *rxBuffer, uint32_t size);
    void receive32(uint32_t *rxBuffer, uint32_t size);

    /**
     * @brief Receive data from SPI in DMA mode (for 1-, 2- and 4-bytes packs)
     * @param [in] rxBuffer Address for received data saving
     * @param [in] size Count of data units
     */
    void receive8viaDMA(uint8_t *rxBuffer, uint32_t size);
    void receive16viaDMA(uint16_t *rxBuffer, uint32_t size);
    void receive32viaDMA(uint32_t *rxBuffer, uint32_t size);

    /**
     * @brief Data transceiving (parallel receiving and transmitting) via SPI in PIO mode
     * @param [in] txData Data for transmitting
     * @param [in] rxBuffer Buffer for receiving
     * @param [in] size Exchanging data size (in transaction units)
     */
    void transceive8(uint8_t *txData, uint8_t *rxBuffer, uint16_t size = 1);
    void transceive16(uint16_t *txData, uint16_t *rxBuffer, uint16_t size = 1);
    void transceive32(uint32_t *txData, uint32_t *rxBuffer, uint16_t size = 1);

    /**
     * @brief Data transceiving (parallel receiving and transmitting) via SPI in DMA mode
     * @param [in] txData Data for transmitting
     * @param [in] rxBuffer Buffer for receiving
     * @param [in] size Exchanging data size (in transaction units)
     */
    void transceive8viaDMA(uint8_t *txData, uint8_t *rxBuffer, uint16_t size = 1);
    void transceive16viaDMA(uint16_t *txData, uint16_t *rxBuffer, uint16_t size = 1);
    void transceive32viaDMA(uint32_t *txData, uint32_t *rxBuffer, uint16_t size = 1);
};

}   // namespace spi
}   // namespace driver
