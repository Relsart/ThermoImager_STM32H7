#pragma once
#include "Nvic.h"

namespace driver {

/**
 * @brief Nvic (interruptions) manager
 * @details Singleton class
 */
class NvicManager
{
private:
    NvicManager() {};
    NvicManager(NvicManager&) = delete;
    
    Nvic m_uart_nvics[8];   // USART/UART nvics instances
    Nvic m_spi_nvics[6];    // SPI nvics instances
    Nvic m_gpio_nvics[7];   // GPIO EXTI nvics instances
    Nvic m_can_nvics[2];    // CAN nvics instances
    Nvic m_dma_nvics[16];   // DMA streams nvics instances
    Nvic m_sdmmc_nvics[2];  // SDMMC nvics instances
    Nvic m_i2c_nvics[4];    // I2C Events nvics instances
    
public:
    static NvicManager& getInstance();

    /**
     * @brief Get Irq number (IRQn_Type) of Periphery Device
     * @param [in] addr Periphery base address OR (0..15) GPIO Pins numbers (for GPIO EXTI)
     * @param [out] irq Saving result variable
     * @return Result. True = Ok
     */
    bool getIrqNumber(uint32_t addr, IRQn_Type& irq);

    /**
     * @brief Get pointer to Uart Nvic
     * @param [in] uart Periphery device
     * @return pointer to Nvic or nullptr if failed 
     */
    Nvic* getUartNvic(const USART_TypeDef* uart);

    /**
     * @brief Get pointer to Spi Nvic
     * @param [in] spi Periphery device
     * @return pointer to Nvic or nullptr if failed 
     */
    Nvic* getSpiNvic(const SPI_TypeDef* spi);

    /**
     * @brief Get pointer to CAN Nvic
     * @param [in] can Periphery device
     * @return pointer to Nvic or nullptr if failed 
     */
    Nvic* getCanNvic(const volatile FDCAN_GlobalTypeDef* can);

    Nvic* getI2CNvic(const I2C_TypeDef* i2c);

    /**
     * @brief Get pointer to DMA stream Nvic
     * @param [in] dma Stream
     * @return pointer to Nvic or nullptr if failed 
     */
    Nvic* getDmaNvic(const volatile DMA_Stream_TypeDef* dma);

    /**
     * @brief Get pointer to GPIO EXTI Nvic
     * @param [in] pin GPIO pin number
     * @return pointer to Nvic or nullptr if failed 
     */
    Nvic* getGpioNvic(uint8_t pin);

    /**
     * @brief Get pointer to SDMMC Nvic
     * @param [in] sdmmc Periphery device
     * @return pointer to Nvic or nullptr if failed 
     */
    Nvic* getSdmmcNvic(const SDMMC_TypeDef* sdmmc);
};

}   // namespace driver
