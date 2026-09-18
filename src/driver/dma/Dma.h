#pragma once

#include <stdint.h>
#include "stm32h7xx.h"
#include "driver/nvic/Nvic.h"
#include "DataTypes.h"

namespace driver {
namespace dma {

/**
 * @brief Manager for DMA streams
 */
class Manager
{
public:
    static Manager& getInstance();

    /**
     * @brief Get free stream
     * @return Pointer to next free stream or NULL if all 16 streams are busy
     */
    DMA_Stream_TypeDef* getStream();

    /**
     * @brief Get DMAMUX request ID for UART periphery 
     * @param [in] uartAddr base address of UART
     * @param [in] direction data stream direction (mem->periph or periph->mem, mem->mem not handled)
     * @return DMAMUX request ID or 0 (id of mem2mem) if failed
     */
    MuxRequestIDs getUartMuxRequestId(uint32_t uartAddr, Direction direct);

    /**
     * @brief Get DMAMUX request ID for SPI periphery 
     * @param [in] spiAddr base address of SPI
     * @param [in] direction data stream direction (mem->periph or periph->mem, mem->mem not handled)
     * @return DMAMUX request ID or 0 (id of mem2mem) if failed
     */
    MuxRequestIDs getSpiMuxRequestId(uint32_t spiAddr, Direction direct);

    /**
     * @brief Get DMAMUX request ID for I2C periphery 
     * @param [in] i2cAddr base address of I2C
     * @param [in] direction data stream direction (mem->periph or periph->mem, mem->mem not handled)
     * @return DMAMUX request ID or 0 (id of mem2mem) if failed
     */
    MuxRequestIDs getI2CMuxRequestId(uint32_t i2cAddr, Direction direct);

private:
    /**
     * @brief Streams DMA1, DMA2 array
     */
    static constexpr DMA_Stream_TypeDef* m_streamArray[] = 
    {
        DMA1_Stream0,
        DMA1_Stream1,
        DMA1_Stream2,
        DMA1_Stream3,
        DMA1_Stream4,
        DMA1_Stream5,
        DMA1_Stream6,
        DMA1_Stream7,
        DMA2_Stream0,
        DMA2_Stream1,
        DMA2_Stream2,
        DMA2_Stream3,
        DMA2_Stream4,
        DMA2_Stream5,
        DMA2_Stream6,
        DMA2_Stream7
    };

    uint32_t m_count = 0;
    static constexpr uint32_t m_steamNumber = sizeof(m_streamArray) / sizeof(m_streamArray[0]);

    /**
     * @brief Singleton
     */
    Manager() {};
    Manager(Manager&) = delete;
};

}   // namespace dma
}   // namespace driver
