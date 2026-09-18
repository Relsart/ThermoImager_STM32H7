#include "NvicManager.h"

namespace driver {

NvicManager& NvicManager::getInstance()
{
    static NvicManager self;
    return self;
}

bool NvicManager::getIrqNumber(uint32_t addr, IRQn_Type& irq)
{
    switch (addr)
    {
    /* GPIOs pins numbers (for EXTI interruptions) */
    case 0 : irq = EXTI0_IRQn;             break;
    case 1 : irq = EXTI1_IRQn;             break;
    case 2 : irq = EXTI2_IRQn;             break;
    case 3 : irq = EXTI3_IRQn;             break;
    case 4 : irq = EXTI4_IRQn;             break;
    case 5 :
    case 6 :
    case 7 :
    case 8 :
    case 9 : irq = EXTI9_5_IRQn;           break;
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15: irq = EXTI15_10_IRQn;         break;

    /* Periphery base addresses (for other interruptions) */
    case USART1_BASE : irq = USART1_IRQn;  break;
    case USART2_BASE : irq = USART2_IRQn;  break;
    case USART3_BASE : irq = USART3_IRQn;  break;
    case UART4_BASE :  irq = UART4_IRQn;   break;
    case UART5_BASE :  irq = UART5_IRQn;   break;
    case USART6_BASE : irq = USART6_IRQn;  break;
    case UART7_BASE :  irq = UART7_IRQn;   break;
    case UART8_BASE :  irq = UART8_IRQn;   break;

    case SPI1_BASE : irq = SPI1_IRQn;      break;
    case SPI2_BASE : irq = SPI2_IRQn;      break;
    case SPI3_BASE : irq = SPI3_IRQn;      break;
    case SPI4_BASE : irq = SPI4_IRQn;      break;
    case SPI5_BASE : irq = SPI5_IRQn;      break;
    case SPI6_BASE : irq = SPI6_IRQn;      break;

    case FDCAN1_BASE : irq = FDCAN1_IT0_IRQn; break;
    case FDCAN2_BASE : irq = FDCAN2_IT0_IRQn; break;

    case I2C1_BASE : irq = I2C1_EV_IRQn; break;
    case I2C2_BASE : irq = I2C2_EV_IRQn; break;
    case I2C3_BASE : irq = I2C3_EV_IRQn; break;
    case I2C4_BASE : irq = I2C4_EV_IRQn; break;

    case DMA1_Stream0_BASE : irq = DMA1_Stream0_IRQn; break;
    case DMA1_Stream1_BASE : irq = DMA1_Stream1_IRQn; break;
    case DMA1_Stream2_BASE : irq = DMA1_Stream2_IRQn; break;
    case DMA1_Stream3_BASE : irq = DMA1_Stream3_IRQn; break;
    case DMA1_Stream4_BASE : irq = DMA1_Stream4_IRQn; break;
    case DMA1_Stream5_BASE : irq = DMA1_Stream5_IRQn; break;
    case DMA1_Stream6_BASE : irq = DMA1_Stream6_IRQn; break;
    case DMA1_Stream7_BASE : irq = DMA1_Stream7_IRQn; break;
    case DMA2_Stream0_BASE : irq = DMA2_Stream0_IRQn; break;
    case DMA2_Stream1_BASE : irq = DMA2_Stream1_IRQn; break;
    case DMA2_Stream2_BASE : irq = DMA2_Stream2_IRQn; break;
    case DMA2_Stream3_BASE : irq = DMA2_Stream3_IRQn; break;
    case DMA2_Stream4_BASE : irq = DMA2_Stream4_IRQn; break;
    case DMA2_Stream5_BASE : irq = DMA2_Stream5_IRQn; break;
    case DMA2_Stream6_BASE : irq = DMA2_Stream6_IRQn; break;
    case DMA2_Stream7_BASE : irq = DMA2_Stream7_IRQn; break;

    case SDMMC1_BASE : irq = SDMMC1_IRQn; break;
    case SDMMC2_BASE : irq = SDMMC2_IRQn; break;

    default:  return false;
    }
    return true;
}

Nvic* NvicManager::getUartNvic(const USART_TypeDef* uart)
{
    switch (reinterpret_cast<uint32_t>(uart))
    {
    case USART1_BASE:   return &m_uart_nvics[0];
    case USART2_BASE:   return &m_uart_nvics[1];
    case USART3_BASE:   return &m_uart_nvics[2];
    case UART4_BASE:    return &m_uart_nvics[3];
    case UART5_BASE:    return &m_uart_nvics[4];
    case USART6_BASE:   return &m_uart_nvics[5];
    case UART7_BASE:    return &m_uart_nvics[6];
    case UART8_BASE:    return &m_uart_nvics[7];
    }
    return nullptr;
}

Nvic* NvicManager::getSpiNvic(const SPI_TypeDef* spi)
{
    switch (reinterpret_cast<uint32_t>(spi))
    {
    case SPI1_BASE: return &m_spi_nvics[0];
    case SPI2_BASE: return &m_spi_nvics[1];
    case SPI3_BASE: return &m_spi_nvics[2];
    case SPI4_BASE: return &m_spi_nvics[3];
    case SPI5_BASE: return &m_spi_nvics[4];
    case SPI6_BASE: return &m_spi_nvics[5];
    }
    return nullptr;
}

Nvic* NvicManager::getCanNvic(const volatile FDCAN_GlobalTypeDef* can)
{
    switch (reinterpret_cast<uint32_t>(can))
    {
    case FDCAN1_BASE: return &m_can_nvics[0];
    case FDCAN2_BASE: return &m_can_nvics[1];
    }
    return nullptr;
}

Nvic* NvicManager::getI2CNvic(const I2C_TypeDef* i2c)
{
    switch (reinterpret_cast<uint32_t>(i2c))
    {
    case I2C1_BASE: return &m_i2c_nvics[0];
    case I2C2_BASE: return &m_i2c_nvics[1];
    case I2C3_BASE: return &m_i2c_nvics[2];
    case I2C4_BASE: return &m_i2c_nvics[3];
    }
    return nullptr;
}

Nvic* NvicManager::getDmaNvic(const volatile DMA_Stream_TypeDef* dma)
{
    switch (reinterpret_cast<uint32_t>(dma))
    {
    case DMA1_Stream0_BASE: return &m_dma_nvics[0];
    case DMA1_Stream1_BASE: return &m_dma_nvics[1];
    case DMA1_Stream2_BASE: return &m_dma_nvics[2];
    case DMA1_Stream3_BASE: return &m_dma_nvics[3];
    case DMA1_Stream4_BASE: return &m_dma_nvics[4];
    case DMA1_Stream5_BASE: return &m_dma_nvics[5];
    case DMA1_Stream6_BASE: return &m_dma_nvics[6];
    case DMA1_Stream7_BASE: return &m_dma_nvics[7];
    case DMA2_Stream0_BASE: return &m_dma_nvics[8];
    case DMA2_Stream1_BASE: return &m_dma_nvics[9];
    case DMA2_Stream2_BASE: return &m_dma_nvics[10];
    case DMA2_Stream3_BASE: return &m_dma_nvics[11];
    case DMA2_Stream4_BASE: return &m_dma_nvics[12];
    case DMA2_Stream5_BASE: return &m_dma_nvics[13];
    case DMA2_Stream6_BASE: return &m_dma_nvics[14];
    case DMA2_Stream7_BASE: return &m_dma_nvics[15];
    }
    return nullptr;
}

Nvic* NvicManager::getGpioNvic(uint8_t pin)
{
    if (pin <= 4)
        return &m_gpio_nvics[pin];
    else if ((pin >= 5) && (pin <= 9))
        return &m_gpio_nvics[5];
    else if ((pin >= 10) && (pin <= 15))
        return &m_gpio_nvics[6];
    else
        return nullptr;
}

Nvic* NvicManager::getSdmmcNvic(const SDMMC_TypeDef* sdmmc)
{
    switch (reinterpret_cast<uint32_t>(sdmmc))
    {
    case SDMMC1_BASE: return &m_sdmmc_nvics[0];
    case SDMMC2_BASE: return &m_sdmmc_nvics[1];
    }
    return nullptr;
}

}   // namespace driver
