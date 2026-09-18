#include "Dma.h"

namespace driver {
namespace dma {

Manager& Manager::getInstance()
{
    static Manager self;
    return self;
}

DMA_Stream_TypeDef* Manager::getStream()
{
    return (m_count >= m_steamNumber) ? nullptr : m_streamArray[m_count++];
}

MuxRequestIDs Manager::getUartMuxRequestId(uint32_t uartAddr, Direction direct)
{
    MuxRequestIDs id = MuxRequestIDs::MemToMem;
    if (direct == Direction::Mem2Mem)
        return id;  // failed!

    switch (uartAddr)
    {
    case USART1_BASE : id = (direct == Direction::Mem2Periph) ? MuxRequestIDs::Usart1TX : MuxRequestIDs::Usart1RX; break;
    case USART2_BASE : id = (direct == Direction::Mem2Periph) ? MuxRequestIDs::Usart2TX : MuxRequestIDs::Usart2RX; break;
    case USART3_BASE : id = (direct == Direction::Mem2Periph) ? MuxRequestIDs::Usart3TX : MuxRequestIDs::Usart3RX; break;
    case UART4_BASE  : id = (direct == Direction::Mem2Periph) ? MuxRequestIDs::Uart4TX : MuxRequestIDs::Uart4RX;   break;
    case UART5_BASE  : id = (direct == Direction::Mem2Periph) ? MuxRequestIDs::Uart5TX : MuxRequestIDs::Uart5RX;   break;
    case USART6_BASE : id = (direct == Direction::Mem2Periph) ? MuxRequestIDs::Usart6TX : MuxRequestIDs::Usart6RX; break;
    case UART7_BASE  : id = (direct == Direction::Mem2Periph) ? MuxRequestIDs::Uart7TX : MuxRequestIDs::Uart7RX;   break;
    case UART8_BASE  : id = (direct == Direction::Mem2Periph) ? MuxRequestIDs::Uart8TX : MuxRequestIDs::Uart8RX;   break;        
    }
    return id;
}

MuxRequestIDs Manager::getSpiMuxRequestId(uint32_t spiAddr, Direction direct)
{
    MuxRequestIDs id = MuxRequestIDs::MemToMem;
    if (direct == Direction::Mem2Mem)
        return id;  // failed!

    switch (spiAddr)
    {
    case SPI1_BASE : id = (direct == Direction::Mem2Periph) ? dma::MuxRequestIDs::Spi1TX : dma::MuxRequestIDs::Spi1RX; break;
    case SPI2_BASE : id = (direct == Direction::Mem2Periph) ? dma::MuxRequestIDs::Spi2TX : dma::MuxRequestIDs::Spi2RX; break;
    case SPI3_BASE : id = (direct == Direction::Mem2Periph) ? dma::MuxRequestIDs::Spi3TX : dma::MuxRequestIDs::Spi3RX; break;
    case SPI4_BASE : id = (direct == Direction::Mem2Periph) ? dma::MuxRequestIDs::Spi4TX : dma::MuxRequestIDs::Spi4RX; break;
    case SPI5_BASE : id = (direct == Direction::Mem2Periph) ? dma::MuxRequestIDs::Spi5TX : dma::MuxRequestIDs::Spi5RX; break;
    // ACHTUNG: SPI6 not supported DMA! Only BDMA!
    }
    return id;
}

MuxRequestIDs Manager::getI2CMuxRequestId(uint32_t i2cAddr, Direction direct)
{
    MuxRequestIDs id = MuxRequestIDs::MemToMem;
    if (direct == Direction::Mem2Mem)
        return id;  // failed!

    switch (i2cAddr)
    {
        case I2C1_BASE : id = (direct == Direction::Mem2Periph) ? dma::MuxRequestIDs::I2c1TX : dma::MuxRequestIDs::I2c1RX; break;
        case I2C2_BASE : id = (direct == Direction::Mem2Periph) ? dma::MuxRequestIDs::I2c2TX : dma::MuxRequestIDs::I2c2RX; break;
        case I2C3_BASE : id = (direct == Direction::Mem2Periph) ? dma::MuxRequestIDs::I2c3TX : dma::MuxRequestIDs::I2c3RX; break;
    }
    return id;
}

}   // namespace dma
}   // namespace driver
