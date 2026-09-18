#pragma once
#include <stdint.h>
#include "driver/dma/Stream.h"
#include "Signal.h"
#include "stm32h7xx.h"

namespace driver {

class I2C;

/**
 * @brief DMA transceiver module
 * @details Slot for DMA Stream IRQ handling (Transfer Complete interrupt)
 */
class DmaTransceiver : public SlotInterface <driver::dma::SignalPack>
{
private:
    I2C& m_holder;              // Link to holder (I2C) class
    I2C_TypeDef* const m_i2c;   // I2C periphery address
    dma::Stream* m_stream;      // DMA stream pointer

    /**
     * @brief DMA signals handler
     */
    void run(dma::SignalPack, uint32_t) override;

public:
    /**
     * @brief Constructor
     */
    DmaTransceiver(I2C& holder);

    /**
     * @brief Prepare the DMA receiving request
     * @param [in] dest receiver buffer address
     * @param [in] destSize size of reading data in bytes
     * @return True = bit condition OK
     */
    bool dmaRxRequest(uint8_t *dest, uint16_t destSize);

    /**
     * @brief Subscribe to DMA Stream instance for data receiving
     */
    void onRxStream(dma::Stream* stream);
};

}   // namespace driver
