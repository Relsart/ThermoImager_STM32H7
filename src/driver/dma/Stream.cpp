#include "Stream.h"
#include "driver/Dwt.h"
#include "driver/nvic/NvicManager.h"

namespace driver {
namespace dma {

Stream::Stream() : m_stream(Manager::getInstance().getStream())
{
    if (!m_stream)
        return; // No more available streams!

    uint32_t dmaNumber = ((uint32_t)m_stream & DMA2_BASE) == DMA2_BASE? 2 : 1;  // DMA number identification (1 or 2)
    uint32_t streamNumber = (((uint32_t)m_stream & 0xFF) - 16) / 24;            // Stream number definition (0...7)

    DMA_TypeDef* dmaReg;  // DMA controller address (DMA1/DMA2)
    if (dmaNumber == 1) // DMA1
    {
        dmaReg = DMA1;
        m_muxNumber = streamNumber;       // channels 0..7
        SET_BIT (RCC->AHB1ENR, RCC_AHB1ENR_DMA1EN); // DMA1 clock enable
    }
    else                // DMA2
    {
        dmaReg = DMA2;
        m_muxNumber = streamNumber + 8;   // channels 8..15
        SET_BIT (RCC->AHB1ENR, RCC_AHB1ENR_DMA2EN); // DMA2 clock enable
    }

    m_mux = (DMAMUX_Channel_TypeDef*)(DMAMUX1_Channel0_BASE + m_muxNumber * 4);

    // ISR/IFCR registers identification:
    m_ifcr = (streamNumber < 4) ? (uint32_t*)&dmaReg->LIFCR : (uint32_t*)&dmaReg->HIFCR;
    m_isr = (streamNumber < 4) ? (uint32_t*)&dmaReg->LISR : (uint32_t*)&dmaReg->HISR;

    // ISR/IFCR bits positions and mask Identification for current stream: 
    uint8_t bitsPos[4] = {0, 6, 16, 22};
    uint8_t index = (streamNumber < 4) ? streamNumber : (streamNumber - 4);
    m_irPos.mask = 0x3D << bitsPos[index];
    m_irPos.tcif = bitsPos[index] + 5;
    m_irPos.htif = bitsPos[index] + 4;
    m_irPos.teif = bitsPos[index] + 3;
    m_irPos.dmeif = bitsPos[index] + 2;
    m_irPos.feif = bitsPos[index];

    nvic = NvicManager::getInstance().getDmaNvic(m_stream);
}


void Stream::run(Irq, uint32_t)
{
    // Transfer Complete Interruption:
    if ((READ_BIT(*m_isr, 1 << m_irPos.tcif)) && (READ_BIT(m_stream->CR, DMA_SxCR_TCIE)))
    {
        SET_BIT(*m_ifcr, (1 << m_irPos.tcif));       // Reset interrruption
        if (!READ_BIT(m_stream->CR, DMA_SxCR_CIRC))  // If no Circle mode
        {
            CLEAR_BIT(m_stream->CR, DMA_SxCR_TCIE);  // Disable interruption
            m_busy.store(false);                    // Mark stream as released
        }

        if (m_transComplete.isConnected())
        {
            SignalPack pack{0};
            if (READ_BIT(m_stream->CR, DMA_SxCR_DBM))    // Double-buffered mode
                pack.data = reinterpret_cast<uint8_t*>(READ_BIT(m_stream->CR, DMA_SxCR_CT) ? m_stream->M0AR : m_stream->M1AR);
            else
                pack.data = reinterpret_cast<uint8_t*>(m_stream->M0AR);
            pack.streamAddr = reinterpret_cast<uint32_t*>(this);
            pack.size = m_dataSize;
            m_transComplete.activ(pack);
        }

        if (!READ_BIT(m_stream->CR, DMA_SxCR_CIRC))  // If no Circle mode - reset size
            m_dataSize = 0;
    }

    // Half transfer complete interruption (has not made complete yet):
    if ((READ_BIT(*m_isr, 1 << m_irPos.htif)) && (READ_BIT(m_stream->CR, DMA_SxCR_HTIE)))
    {
        // Reset and disable:
        SET_BIT(*m_ifcr, 1 << m_irPos.htif);
        CLEAR_BIT (m_stream->CR, DMA_SxCR_HTIE);
    }

    // Transfer Error Interruption (common yet):
    if (READ_BIT (*m_isr, (1 << m_irPos.teif) | (1 << m_irPos.dmeif) | (1 << m_irPos.feif)))
    {
        // Reset and disable:
        SET_BIT(*m_ifcr, (1 << m_irPos.teif) | (1 << m_irPos.dmeif) | (1 << m_irPos.feif));
        CLEAR_BIT (m_stream->CR, DMA_SxCR_DMEIE | DMA_SxCR_TEIE);
        CLEAR_BIT (m_stream->FCR, DMA_SxFCR_FEIE);
    }
}


void Stream::transferData (uint32_t memaddr1, uint32_t memaddr2, uint16_t dataSize, uint8_t unitSize, bool memInc)
{
    stopStream ();
    m_dataSize = dataSize;
    WRITE_REG (m_stream->NDTR, m_dataSize);   // Set transfered data size

    // Set increment (data unit) size:
    IncSize inc;
    switch (unitSize)
    {
    case 1: inc = IncSize::Byte; break;
    case 2: inc = IncSize::HalfWord; break;
    case 4: inc = IncSize::Word; break;
    default: return;
    }

    MODIFY_REG (m_stream->CR, DMA_SxCR_PSIZE, static_cast<uint8_t>(inc) << DMA_SxCR_PSIZE_Pos);
    MODIFY_REG (m_stream->CR, DMA_SxCR_MSIZE, static_cast<uint8_t>(inc) << DMA_SxCR_MSIZE_Pos);
    MODIFY_REG (m_stream->CR, DMA_SxCR_MINC, static_cast<uint8_t>(memInc) << DMA_SxCR_MINC_Pos);

    // For Mem-To-Mem transactions:
    if ((m_mux->CCR & 0x7F) == static_cast<uint32_t>(MuxRequestIDs::MemToMem))
    {
        WRITE_REG (m_stream->PAR, memaddr1);    // Source address
        WRITE_REG (m_stream->M0AR, memaddr2);   // Destination address
    }
    else  // For exchange with periphery
    {
        WRITE_REG (m_stream->M0AR, memaddr1);         // Memory address
        if (READ_BIT (m_stream->CR, DMA_SxCR_DBM))    // Buffer 2 for double-buffered mode
            WRITE_REG (m_stream->M1AR, memaddr2);
    }

    MODIFY_REG(m_stream->CR, 0x1E, DMA_SxCR_TCIE | DMA_SxCR_TEIE);  // Enable interruptions (transfer complete & errors)
    m_busy.store(true);
    SET_BIT (m_stream->CR, DMA_SxCR_EN);  // Stream enable
}


bool Stream::configurate(const StreamConfig* config)
{
    if (!m_stream || !config)
        return false;   // No available

    memcpy(&m_config, config, sizeof(m_config));   // Save configuration (for possible reinitialization)
    
    // Init stream NVIC:
    IRQn_Type irq;
    if (!NvicManager::getInstance().getIrqNumber(reinterpret_cast<uint32_t>(m_stream), irq))
        return false;

    if (!nvic)
        return false;

    nvic->init(irq, m_config.nvicPriority, *this);
    nvic->disable();    // Disable IRQ before stream initialization...
    stopStream();       // Stop stream and wait for disabling

    // If used FIFO check its parameters for correctness:
    if (m_config.fifoMode && !checkFifoParams(&m_config))
        return false;

    // Stream configuration:
    CLEAR_REG (m_stream->CR);
    uint32_t tempCR = 0;
    tempCR |= static_cast<uint32_t>(m_config.mode);                                        // Set stream mode flags (CIRC, PFCTRL, DBM, CT)
    if (m_config.periphInc) SET_BIT (tempCR, DMA_SxCR_PINC);                               // Periphery address increment
    if (m_config.memInc) SET_BIT (tempCR, DMA_SxCR_MINC);                                  // Memory address increment
    SET_BIT (tempCR, static_cast<uint8_t>(m_config.periphBurst) << DMA_SxCR_PBURST_Pos);   // Increment size for Periphery pack (burst mode)
    SET_BIT (tempCR, static_cast<uint8_t>(m_config.memBurst) << DMA_SxCR_MBURST_Pos);      // Increment size for Memory pack (burst mode)
    SET_BIT (tempCR, static_cast<uint8_t>(m_config.priority) << DMA_SxCR_PL_Pos);          // Stream priority

    if ((m_config.request == MuxRequestIDs::Usart1RX) || (m_config.request == MuxRequestIDs::Usart1TX) ||
        (m_config.request == MuxRequestIDs::Usart2RX) || (m_config.request == MuxRequestIDs::Usart2TX) ||
        (m_config.request == MuxRequestIDs::Usart3RX) || (m_config.request == MuxRequestIDs::Usart3TX) ||
        (m_config.request == MuxRequestIDs::Uart4RX)  || (m_config.request == MuxRequestIDs::Uart4TX)  ||
        (m_config.request == MuxRequestIDs::Uart5RX)  || (m_config.request == MuxRequestIDs::Uart5TX)  ||
        (m_config.request == MuxRequestIDs::Usart6RX) || (m_config.request == MuxRequestIDs::Usart6TX) ||
        (m_config.request == MuxRequestIDs::Uart7RX)  || (m_config.request == MuxRequestIDs::Uart7TX)  ||
        (m_config.request == MuxRequestIDs::Uart8RX)  || (m_config.request == MuxRequestIDs::Uart8TX))
    {
        SET_BIT (tempCR, DMA_SxCR_TRBUFF); // For UART streams enable bufferization data handling (Errata 2.22)
    }

    WRITE_REG (m_stream->CR, tempCR);

    // Set periphery address & transmission direction
    if (!setPeriphAddr (m_config.request))  
        return false;

    // FIFO settings:
    tempCR = 0;
    if (m_config.fifoMode)
    {
        SET_BIT (tempCR, static_cast<uint8_t>(m_config.fifoThresh) << DMA_SxFCR_FTH_Pos);  // FIFO threshold
        SET_BIT (tempCR, DMA_SxFCR_DMDIS);                                                 // Disable Direct Mode
        WRITE_REG (m_stream->FCR, tempCR);
    }

    nvic->enable(); // Enable interruptions and finish initialization
    m_initOk = true;
    m_errorFlag = false; 
    m_busy.store(false);
    return true;
}

StreamConfig Stream::getConfig()
{
    return m_config;
}

bool Stream::reInit()
{
    return (m_initOk ? configurate(&m_config) : false);
}

// TODO: Make FIFO checking at after transaction- unit size is different!
bool Stream::checkFifoParams(const StreamConfig* configs)
{
    if (configs->memIncSize == IncSize::Byte)           // For 1-byte incrementation
    {
        if ((configs->fifoThresh == FifoThreshold::Quater) || (configs->fifoThresh == FifoThreshold::ThreeQuaters))
        {
            if ((configs->memBurst == Burst::Inc8) || (configs->memBurst == Burst::Inc16))
                return false;
        }
        if ((configs->fifoThresh == FifoThreshold::Half) && (configs->memBurst == Burst::Inc16))
        {
            return false;
        }
    }
    else if (configs->memIncSize == IncSize::HalfWord)  // For 1-bytes incrementation
    {
        if ((configs->fifoThresh == FifoThreshold::Quater) || (configs->fifoThresh == FifoThreshold::ThreeQuaters))
            return false;

        if (configs->fifoThresh == FifoThreshold::Half)
        {
            if ((configs->memBurst == Burst::Inc8) || (configs->memBurst == Burst::Inc16))
                return false;
        }
        if ((configs->fifoThresh == FifoThreshold::Full) || (configs->memBurst == Burst::Inc16))
        {
            return false;
        }
    }
    else if (configs->memIncSize == IncSize::Word)   // For 4-bytes incrementation
    {
        if ((configs->fifoThresh == FifoThreshold::Quater) || 
            (configs->fifoThresh == FifoThreshold::ThreeQuaters) || 
            (configs->fifoThresh == FifoThreshold::Half))
            return false;

        if (configs->fifoThresh == FifoThreshold::Full)
        {
            if ((configs->memBurst == Burst::Inc8) || (configs->memBurst == Burst::Inc16))
                return false;
        }
    }
    return true;
}

bool Stream::checkMemAddr(uint32_t addr)
{
    if ((addr >= 0x24000000) && (addr <= 0x2407FFFF))       // AXI SRAM memory
        return true;
    else if ((addr >= 0x30000000) && (addr <= 0x3800FFFF))  // SRAM1, SRAM2, SRAM3, SRAM4 memory
        return true;
    else if ((addr >= 0x38800000) && (addr <= 0x38800FFF))  // Backup SRAM memory
        return true;
    else if ((addr >= 0x60000000) && (addr <= 0x8FFFFFFF))  // FMC memory
        return true;
    else if ((addr >= 0x90000000) && (addr <= 0x9FFFFFFF))  // QUADSPI memory
        return true;
    else if ((addr >= 0x8000000) && (addr <= 0x81FFFFF))    // Flash memory banks 1, 2
        return true;
    else
        return false;   // Not supported by DMA1, DMA2.
}

void Stream::stopStream()
{
    // Disable stream and wait for its disabling:
    CLEAR_BIT (m_stream->CR, DMA_SxCR_EN);
    driver::DwtTimer::TimeoutData param = driver::DwtTimer::getInstance().timeoutInit(300);
    while (READ_BIT(m_stream->CR, DMA_SxCR_EN))
    {
        if (driver::DwtTimer::getInstance().checkTimeout(param))
        {
            m_errorFlag = true;
            return;     
        }
    }
    SET_BIT(DMAMUX1_ChannelStatus->CFR, 1 << m_muxNumber);    // DMAMUX synchro overrun flag reset
    SET_BIT(*m_ifcr, m_irPos.mask);                           // Stream interruptions flags reset
    m_busy.store(false);                                     // Release stream
}


bool Stream::setPeriphAddr(MuxRequestIDs requestId)
{
    uint32_t periphAddr = 0;
    Direction direction;

    switch (requestId)
    {
    case MuxRequestIDs::MemToMem:
        direction = Direction::Mem2Mem;
        break;
    case MuxRequestIDs::Usart1RX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&USART1->RDR);
        direction = Direction::Periph2Mem;
        break;
    case MuxRequestIDs::Usart1TX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&USART1->TDR);
        direction = Direction::Mem2Periph;
        break;
    case MuxRequestIDs::Usart2RX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&USART2->RDR);
        direction = Direction::Periph2Mem;
        break;
    case MuxRequestIDs::Usart2TX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&USART2->TDR);
        direction = Direction::Mem2Periph;
        break;
    case MuxRequestIDs::Usart3RX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&USART3->RDR);
        direction = Direction::Periph2Mem;
        break;
    case MuxRequestIDs::Usart3TX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&USART3->TDR);
        direction = Direction::Mem2Periph;
        break;
    case MuxRequestIDs::Uart4RX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&UART4->RDR);
        direction = Direction::Periph2Mem;
        break;
    case MuxRequestIDs::Uart4TX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&UART4->TDR);
        direction = Direction::Mem2Periph;
        break;
    case MuxRequestIDs::Uart5RX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&UART5->RDR);
        direction = Direction::Periph2Mem;
        break;
    case MuxRequestIDs::Uart5TX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&UART5->TDR);
        direction = Direction::Mem2Periph;
        break;
    case MuxRequestIDs::Usart6RX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&USART6->RDR);
        direction = Direction::Periph2Mem;
        break;
    case MuxRequestIDs::Usart6TX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&USART6->TDR);
        direction = Direction::Mem2Periph;
        break;
    case MuxRequestIDs::Uart7RX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&UART7->RDR);
        direction = Direction::Periph2Mem;
        break;
    case MuxRequestIDs::Uart7TX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&UART7->TDR);
        direction = Direction::Mem2Periph;
        break;
    case MuxRequestIDs::Uart8RX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&UART8->RDR);
        direction = Direction::Periph2Mem;
        break;
    case MuxRequestIDs::Uart8TX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&UART8->TDR);
        direction = Direction::Mem2Periph;
        break;
    case MuxRequestIDs::Spi1RX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&SPI1->RXDR);
        direction = Direction::Periph2Mem;
        break;
    case MuxRequestIDs::Spi1TX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&SPI1->TXDR);
        direction = Direction::Mem2Periph;
        break;
    case MuxRequestIDs::Spi2RX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&SPI2->RXDR);
        direction = Direction::Periph2Mem;
        break;
    case MuxRequestIDs::Spi2TX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&SPI2->TXDR);
        direction = Direction::Mem2Periph;
        break;
    case MuxRequestIDs::Spi3RX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&SPI3->RXDR);
        direction = Direction::Periph2Mem;
        break;
    case MuxRequestIDs::Spi3TX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&SPI3->TXDR);
        direction = Direction::Mem2Periph;
        break;
    case MuxRequestIDs::Spi4RX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&SPI4->RXDR);
        direction = Direction::Periph2Mem;
        break;
    case MuxRequestIDs::Spi4TX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&SPI4->TXDR);
        direction = Direction::Mem2Periph;
        break;
    case MuxRequestIDs::Spi5RX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&SPI5->RXDR);
        direction = Direction::Periph2Mem;
        break;
    case MuxRequestIDs::Spi5TX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&SPI5->TXDR);
        direction = Direction::Mem2Periph;
        break;
    case MuxRequestIDs::I2c1RX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&I2C1->RXDR);
        direction = Direction::Periph2Mem;
        break;
    case MuxRequestIDs::I2c1TX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&I2C1->TXDR);
        direction = Direction::Mem2Periph;
        break;
    case MuxRequestIDs::I2c2RX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&I2C2->RXDR);
        direction = Direction::Periph2Mem;
        break;
    case MuxRequestIDs::I2c2TX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&I2C2->TXDR);
        direction = Direction::Mem2Periph;
        break;
    case MuxRequestIDs::I2c3RX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&I2C3->RXDR);
        direction = Direction::Periph2Mem;
        break;
    case MuxRequestIDs::I2c3TX:
        m_stream->PAR = reinterpret_cast<uint32_t>(&I2C3->TXDR);
        direction = Direction::Mem2Periph;
        break;
    default:
        return false;
    }

    SET_BIT (m_stream->CR, static_cast<uint8_t>(direction) << DMA_SxCR_DIR_Pos);
    m_mux->CCR = static_cast<uint8_t>(m_config.request);
    return true;
}

bool Stream::periphTransfer(void* memAddr, uint16_t dataSize, uint8_t incSize, bool memInc)
{
    if (dataSize == 0 || !m_initOk || m_errorFlag || m_busy.load())
        return false;     // Stream not ready

    if (READ_BIT(m_stream->CR, DMA_SxCR_DBM))
        return false;     // Stream configured to double-buffered mode

    if ((m_mux->CCR & 0x7F) == static_cast<uint32_t>(MuxRequestIDs::MemToMem))
        return false;     // Stream configured to Mem2Mem transactions (not periphery)

    if (!checkMemAddr(reinterpret_cast<uint32_t>(memAddr)))
        return false;     // Address not supported by DMA1,2

    transferData((uint32_t)memAddr, 0, dataSize, incSize, memInc);
    return true;
}

bool Stream::periphTransfer(void* memBuff1, void* memBuff2, uint16_t dataSize, uint8_t incSize)
{
    if (dataSize == 0 || !m_initOk || m_errorFlag || m_busy.load())
        return false;     // Stream not ready

    if (!READ_BIT(m_stream->CR, DMA_SxCR_DBM))
        return false;     // Stream NOT configured to double-buffered mode

    if ((m_mux->CCR & 0x7F) == static_cast<uint32_t>(MuxRequestIDs::MemToMem))
        return false;     // Stream configured to Mem2Mem transactions (not periphery)

    if (!checkMemAddr(reinterpret_cast<uint32_t>(memBuff1)) || !checkMemAddr(reinterpret_cast<uint32_t>(memBuff2)))
        return false;     // Addresses not supported by DMA1,2

    transferData((uint32_t)memBuff1, (uint32_t)memBuff2, dataSize, incSize);
    return true;
}

bool Stream::mem2memTransfer(uint8_t* sourceAddr, uint8_t* destAddr, uint16_t dataSize)
{
    if (dataSize == 0 || !m_initOk || m_errorFlag || m_busy.load())
        return false;     // Stream not ready

    if ((m_mux->CCR & 0x7F) != static_cast<uint32_t>(MuxRequestIDs::MemToMem))
        return false;     // Stream configuration is not MemToMem

    if ((!checkMemAddr((uint32_t)sourceAddr)) || (!checkMemAddr((uint32_t)destAddr)))
        return false;     // Addresses not supported by DMA

    transferData((uint32_t)sourceAddr, (uint32_t)destAddr, dataSize, sizeof(uint8_t));
    return true;
}

bool Stream::mem2memTransfer(uint16_t* sourceAddr, uint8_t* destAddr, uint16_t dataSize)
{
    if (dataSize == 0 || !m_initOk || m_errorFlag || m_busy.load())
        return false;     // Stream not ready

    if ((m_mux->CCR & 0x7F) != static_cast<uint32_t>(MuxRequestIDs::MemToMem))
        return false;     // Stream configuration is not MemToMem

    if ((!checkMemAddr((uint32_t)sourceAddr)) || (!checkMemAddr((uint32_t)destAddr)))
        return false;     // Addresses not supported by DMA

    transferData((uint32_t)sourceAddr, (uint32_t)destAddr, dataSize, sizeof(uint16_t));
    return true;
}

bool Stream::mem2memTransfer(uint32_t* sourceAddr, uint8_t* destAddr, uint16_t dataSize)
{
    if (dataSize == 0 || !m_initOk || m_errorFlag || m_busy.load())
        return false;     // Stream not ready

    if ((m_mux->CCR & 0x7F) != static_cast<uint32_t>(MuxRequestIDs::MemToMem))
        return false;     // Stream configuration is not MemToMem

    if ((!checkMemAddr((uint32_t)sourceAddr)) || (!checkMemAddr((uint32_t)destAddr)))
        return false;     // Addresses not supported by DMA

    transferData((uint32_t)sourceAddr, (uint32_t)destAddr, dataSize, sizeof(uint32_t));
    return true;
}

bool Stream::isInitialised()
{
    return m_initOk;
}

bool Stream::isBusy()
{
    return m_busy.load();
}

bool Stream::isErrorState()
{
    return m_errorFlag;
}

void Stream::onTransferCompleteSlot(SlotInterface<SignalPack>*slot)
{
    m_transComplete.connect(slot);
}

}   // namespace dma
}   // namespace driver
