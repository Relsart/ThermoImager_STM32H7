#include "TransceiverDma.h"
#include "Loger.h"
#include "driver/Dwt.h"
#include "Spi.h"

namespace driver {
namespace spi {

DmaTransceiver::DmaTransceiver(Spi& maintainer) : m_spi(maintainer.m_spi), m_maintainer(maintainer)
{}

void DmaTransceiver::onTxStream(dma::Stream* stream)
{
    if (stream)
        m_txStream = stream;
    //m_stream->onTransferCompleteSlot(this);   // Not used
}

void DmaTransceiver::onRxStream(dma::Stream* stream)
{
    if (stream)
        m_rxStream = stream;
    //m_stream->onTransferCompleteSlot(this);   // Not used
}

bool DmaTransceiver::checkUnitSize(uint8_t unitSize)
{
    return ((unitSize == 1) || (unitSize == 2) || (unitSize == 4));
}

uint8_t DmaTransceiver::calcFrameSize(uint8_t increment)
{
    FrameSize framesize;
    switch (increment)
    {
    case 1:  framesize = FrameSize::Frame_8bit;  break;
    case 2:  framesize = FrameSize::Frame_16bit; break;
    case 4:  framesize = FrameSize::Frame_32bit; break;
    default: framesize = FrameSize::Frame_8bit;  break;
    }
    return static_cast<uint8_t>(framesize);
}

bool DmaTransceiver::waitForTransceiverRelease(DwtTimer::TimeoutData& timeoutParams)
{
    while (m_busy.load())
    {
        if (driver::DwtTimer::getInstance().checkTimeout(timeoutParams))
            return false;
    }
    return true;
}

void DmaTransceiver::transmit(void* data, uint8_t unitSize, uint32_t dataCount, uint32_t options)
{
    if (!data || dataCount == 0 || !checkUnitSize(unitSize))
        return;

    if (!m_txStream)
    {
        Log(lmSystem, Error) << "SPI Tx DMA: no stream connected!";
        return;
    }
    
    // Wait for the transceiver to be released (from previous transaction):
    driver::DwtTimer::TimeoutData dwtParam = driver::DwtTimer::getInstance().timeoutInit(500);
    if (!waitForTransceiverRelease(dwtParam))
        return; // Timeout!

    // Config stream to transmit data:
    dma::StreamConfig streamConfig = m_txStream->getConfig();
    streamConfig.request = dma::Manager::getInstance().getSpiMuxRequestId((uint32_t)m_spi, dma::Direction::Mem2Periph);
    streamConfig.periphInc = false;
    streamConfig.memInc = ((options & RepeatedTx) == 0);
    if ((options & RepeatedTx))
    {
        streamConfig.fifoMode = 0;  // For sending without memaddr incrementing FIFO mode must be switched off!
    }
    if (!m_txStream->configurate(&streamConfig))
    {
        return;
    }
    m_currentTask = OperType::Transmit;
    m_txState.incSize = unitSize;
    m_txState.options = options;
    m_busy.store(true);

    // Calculate current transfer and remained data size:
    uint32_t currentTxSize = (dataCount > transferMaxsize) ? transferMaxsize : dataCount;
    m_txState.remainSize = dataCount - currentTxSize;

    // Calculate next address (if there are more than one transaction):
    if ((m_txState.options & RepeatedTx) == 0)
        m_txState.nextAddr = (m_txState.remainSize != 0) ? (reinterpret_cast<uint32_t>(data) + (currentTxSize * m_txState.incSize)) : 0;
    else
        m_txState.nextAddr = reinterpret_cast<uint32_t>(data);

    // First transaction:
    txTransaction(data, currentTxSize, m_txState.incSize);
}

void DmaTransceiver::receive(void* buffer, uint8_t unitSize, uint32_t dataCount)
{
    if (!buffer || dataCount == 0 || !checkUnitSize(unitSize))
        return;

    if (!m_rxStream)
    {
        Log(lmSystem, Error) << "SPI Rx DMA: no stream connected!";
        return;
    }

    // Wait for the transceiver to be released:
    driver::DwtTimer::TimeoutData dwtParam = driver::DwtTimer::getInstance().timeoutInit(500);  
    if (!waitForTransceiverRelease(dwtParam))
        return; // Timeout!

    // Config stream to receive data:
    dma::StreamConfig streamConfig = m_rxStream->getConfig();
    streamConfig.request = dma::Manager::getInstance().getSpiMuxRequestId((uint32_t)m_spi, dma::Direction::Periph2Mem);
    streamConfig.periphInc = false;
    streamConfig.memInc = true;
    if (!m_rxStream->configurate(&streamConfig))
        return;

    m_currentTask = OperType::Receive;
    m_rxState.incSize = unitSize;
    m_busy.store(true);    // Marks transceiver as busy

    // Calculate current transfer and remained data size:
    uint32_t currentRxSize = (dataCount > transferMaxsize) ? transferMaxsize : dataCount;
    m_rxState.remainSize = dataCount - currentRxSize;

    // Calculate next address (if there are more than one transaction):
    m_rxState.nextAddr = (m_rxState.remainSize != 0) ? (reinterpret_cast<uint32_t>(buffer) + (currentRxSize * m_rxState.incSize)) : 0;

    // First transaction:
    rxTransaction(buffer, currentRxSize, m_rxState.incSize);
}

void DmaTransceiver::transmitReceive(void* data, void* buffer, uint8_t unitSize, uint32_t dataCount)
{ 
    if (!data || !buffer || dataCount == 0 || !checkUnitSize(unitSize))
        return;
    
    if (!m_rxStream || !m_txStream)
    {
        Log(lmSystem, Error) << "SPI Tx/Rx DMA: not both stream connected!";
        return;
    }

    // Wait for the transceiver to be released:
    driver::DwtTimer::TimeoutData dwtParam = driver::DwtTimer::getInstance().timeoutInit(500);  
    if (!waitForTransceiverRelease(dwtParam))
        return; // Timeout!

    // Config streams:
    dma::StreamConfig streamConfig = m_txStream->getConfig();
    streamConfig.request = dma::Manager::getInstance().getSpiMuxRequestId((uint32_t)m_spi, dma::Direction::Mem2Periph);
    streamConfig.periphInc = false;
    streamConfig.memInc = true;
    if (!m_txStream->configurate(&streamConfig))
        return;

    streamConfig = m_rxStream->getConfig();
    streamConfig.request = dma::Manager::getInstance().getSpiMuxRequestId((uint32_t)m_spi, dma::Direction::Periph2Mem);
    streamConfig.periphInc = false;
    streamConfig.memInc = true;
    if (!m_rxStream->configurate(&streamConfig))
        return;

    m_currentTask = OperType::Duplex;
    m_txState.incSize = unitSize;
    m_rxState.incSize = m_txState.incSize;
    m_busy.store(true);

    // Calculate current transfer and remained data size:
    uint32_t currExchangeSize = (dataCount > transferMaxsize) ? transferMaxsize : dataCount;
    m_txState.remainSize = dataCount - currExchangeSize;
    m_rxState.remainSize = m_txState.remainSize;

    // Calculate next data/buffer addresses (if there are more than one transaction):
    m_txState.nextAddr = (m_txState.remainSize != 0) ? (reinterpret_cast<uint32_t>(data) + (currExchangeSize * m_txState.incSize)) : 0;
    m_rxState.nextAddr = (m_rxState.remainSize != 0) ? (reinterpret_cast<uint32_t>(buffer) + (currExchangeSize * m_rxState.incSize)) : 0;

    // First transaction:
    txRxTransaction(data, buffer, currExchangeSize, m_txState.incSize);
}

void DmaTransceiver::txTransaction (void* dataAddr, uint16_t dataSize, uint8_t inc)
{
    // Wait for the stream to be released:
    driver::DwtTimer::TimeoutData dwtParam = driver::DwtTimer::getInstance().timeoutInit(500);  
    while (m_txStream->isBusy())
    {
        if (driver::DwtTimer::getInstance().checkTimeout(dwtParam))
            return;
    }

    // Disable SPI and wait until it disabled:                  
    dwtParam = driver::DwtTimer::getInstance().timeoutInit(200);
    if (!m_maintainer.stopSpi(dwtParam))
        return;

    bool memInc = ((m_txState.options & OptionFlags::RepeatedTx) == 0);

    MODIFY_REG(m_spi->CFG2, SPI_CFG2_COMM, SPI_CFG2_COMM_0);        // Set SPI mode = Simplex Transmitter
    CLEAR_BIT(m_spi->CFG1, SPI_CFG1_TXDMAEN);                       // Disable DMA transmitter
    MODIFY_REG(m_spi->CFG1, SPI_CFG1_DSIZE, calcFrameSize(inc) << SPI_CFG1_DSIZE_Pos);  // Set frame size
    m_txStream->periphTransfer(dataAddr, dataSize, inc, memInc);    // Prapare data in DMA stream
    MODIFY_REG(m_spi->CR2, SPI_CR2_TSIZE, dataSize);                // Set number of receiving data
    SET_BIT(m_spi->CFG1, SPI_CFG1_TXDMAEN);                         // Enable DMA transmitter
    SET_BIT (m_spi->IER, SPI_IER_EOTIE);                            // Enable SPI End-Of-Transfer interruption
    SET_BIT(m_spi->CR1, SPI_CR1_SPE);                               // Enable SPI
    SET_BIT(m_spi->CR1, SPI_CR1_CSTART);                            // Start SPI data transfer
}

void DmaTransceiver::rxTransaction (void* buffAddr, uint16_t dataSize, uint8_t inc)
{
    // Wait for the stream to be released:
    driver::DwtTimer::TimeoutData dwtParam = driver::DwtTimer::getInstance().timeoutInit(500);  
    while (m_rxStream->isBusy())
    {
        if (driver::DwtTimer::getInstance().checkTimeout(dwtParam))
            return;
    }

    // Disable SPI and wait until it disabled:                  
    dwtParam = driver::DwtTimer::getInstance().timeoutInit(200);
    if (!m_maintainer.stopSpi(dwtParam))
        return;

    MODIFY_REG(m_spi->CFG2, SPI_CFG2_COMM, SPI_CFG2_COMM_1);    // Set SPI mode = Simplex Receiver
    CLEAR_BIT (m_spi->CFG1, SPI_CFG1_RXDMAEN);                  // Disable DMA receiver
    MODIFY_REG(m_spi->CFG1, SPI_CFG1_DSIZE, calcFrameSize(inc) << SPI_CFG1_DSIZE_Pos);  // Set frame size
    m_rxStream->periphTransfer(buffAddr, dataSize, inc);        // Prapare data in DMA stream
    MODIFY_REG(m_spi->CR2, SPI_CR2_TSIZE, dataSize);            // Set number of receiving data
    SET_BIT (m_spi->CFG1, SPI_CFG1_RXDMAEN);                    // Enable DMA receiver
    SET_BIT (m_spi->IER, SPI_IER_EOTIE);                        // Enable SPI End-Of-Transfer interruption
    SET_BIT (m_spi->CR1, SPI_CR1_SPE);                          // Enable SPI
    SET_BIT(m_spi->CR1, SPI_CR1_CSTART);                        // Start SPI data transfer
}

void DmaTransceiver::txRxTransaction(void* dataAddr, void* buffAddr, uint16_t dataSize, uint8_t inc)
{
    // Wait for the stream to be released:
    driver::DwtTimer::TimeoutData dwtParam = driver::DwtTimer::getInstance().timeoutInit(500);  
    while (m_txStream->isBusy())
    {
        if (driver::DwtTimer::getInstance().checkTimeout(dwtParam))
            return;
    }

    // Disable SPI and wait until it disabled:                  
    dwtParam = driver::DwtTimer::getInstance().timeoutInit(200);
    if (!m_maintainer.stopSpi(dwtParam))
        return;

    CLEAR_BIT (m_spi->CFG2, SPI_CFG2_COMM);                         // Set Full-Duplex mode
    CLEAR_BIT(m_spi->CFG1, SPI_CFG1_TXDMAEN | SPI_CFG1_RXDMAEN);    // Disable DMA transmitter and receiver
    MODIFY_REG(m_spi->CFG1, SPI_CFG1_DSIZE, calcFrameSize(inc) << SPI_CFG1_DSIZE_Pos);  // Set frame size
    m_txStream->periphTransfer(dataAddr, dataSize, inc);            // Prapare data in DMA streams
    m_rxStream->periphTransfer(buffAddr, dataSize, inc);
    MODIFY_REG(m_spi->CR2, SPI_CR2_TSIZE, dataSize);                // Set number of receiving data
    SET_BIT (m_spi->CFG1, SPI_CFG1_RXDMAEN | SPI_CFG1_TXDMAEN);     // Enable DMA receiver and transmitter
    SET_BIT (m_spi->IER, SPI_IER_EOTIE);                            // Enable SPI End-Of-Transfer interruption
    SET_BIT (m_spi->CR1, SPI_CR1_SPE);                              // Enable SPI
    SET_BIT(m_spi->CR1, SPI_CR1_CSTART);                            // Start SPI data transfer
}

void DmaTransceiver::run(dma::SignalPack pack, uint32_t)  // DEPRECATED!
{
    // Handling DMA stream interrupts not used!
    // Transfer finish interruption (from DMA) appears a bit earlier then last data came to periphery, so "busy" flag may be disabled at wrong time!
} 

void DmaTransceiver::nextTransaction()
{
    if (m_currentTask == OperType::Transmit)
    {
        if (m_txState.remainSize > 0)   // There is data for transmitting
        {
            // Calculate current transfer and remained data size:
            uint32_t currentTxSize = (m_txState.remainSize > transferMaxsize) ? transferMaxsize : m_txState.remainSize;
            m_txState.remainSize -= currentTxSize;
            // Next transaction:
            txTransaction(reinterpret_cast<void*>(m_txState.nextAddr), currentTxSize, m_txState.incSize);
            // Calculate next address (if there are more than one transaction):
            if ((m_txState.options & RepeatedTx) == 0)
                m_txState.nextAddr = (m_txState.remainSize != 0) ? (m_txState.nextAddr + (currentTxSize * m_txState.incSize)) : 0;
        }
        else    // Finish transmitting
        {
            m_currentTask = OperType::Idle;
            m_busy.store(false);   // Release transmitter
        }
    }
    else if (m_currentTask == OperType::Receive)
    {
        if (m_rxState.remainSize > 0)   // There is data for receiving
        {
            // Calculate current transfer and remained data size:
            uint32_t currentRxSize = (m_rxState.remainSize > transferMaxsize) ? transferMaxsize : m_rxState.remainSize;
            m_rxState.remainSize -= currentRxSize;
            // Next transaction:
            rxTransaction(reinterpret_cast<void*>(m_rxState.nextAddr), currentRxSize, m_rxState.incSize);
            // Calculate next address (if there are more than one transaction):
            m_rxState.nextAddr = (m_rxState.remainSize != 0) ? (m_rxState.nextAddr + (currentRxSize * m_rxState.incSize)) : 0;
        }
        else    // Finish receiving
        {
            m_currentTask = OperType::Idle;
            m_busy.store(false);   // Release transmitter
        }
    }
    else if (m_currentTask == OperType::Duplex)
    {
        if (m_txState.remainSize > 0)   // In duplex mode RX and TX are the same
        {
            // Calculate current transfer and remained data size:
            uint32_t currExchangeSize = (m_txState.remainSize > transferMaxsize) ? transferMaxsize : m_txState.remainSize;
            m_txState.remainSize -= currExchangeSize;
            m_rxState.remainSize = m_txState.remainSize;    // Not needed?
            // Next transaction:
            txRxTransaction(reinterpret_cast<void*>(m_txState.nextAddr), reinterpret_cast<void*>(m_rxState.nextAddr), currExchangeSize, m_txState.incSize);
            // Calculate next addresses (if there are more than one transaction):
            m_txState.nextAddr = (m_txState.remainSize != 0) ? (m_txState.nextAddr + (currExchangeSize * m_txState.incSize)) : 0;
            m_rxState.nextAddr = (m_rxState.remainSize != 0) ? (m_rxState.nextAddr + (currExchangeSize * m_rxState.incSize)) : 0;
        }
        else    // Finish receiving
        {
            m_currentTask = OperType::Idle;
            m_busy.store(false);   // Release transmitter
        }
    }
}

bool DmaTransceiver::isBusy()
{
    return m_busy.load();
}

}   // namespace spi
}   // namespace driver
