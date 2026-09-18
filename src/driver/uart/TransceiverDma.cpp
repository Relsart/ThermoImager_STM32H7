#include "TransceiverDma.h"
#include "Uart.h"
#include "driver/Dwt.h"

namespace driver {
namespace uart {

DmaTransceiver::DmaTransceiver(Port& holder) : m_uart(holder.m_uart), m_holder(holder)
{}

DmaTransceiver::~DmaTransceiver()
{
    free(m_rxBuffer);
    free(m_rxBuffer2);
}

void DmaTransceiver::onTxStream(dma::Stream* stream)
{
    if (!stream)
        return;

    m_txStream = stream;
    m_txConfig.request = dma::Manager::getInstance().getUartMuxRequestId((uint32_t)m_uart, dma::Direction::Mem2Periph);
    m_txConfig.mode = dma::Mode::Normal;
    m_txConfig.periphInc = false;
    m_txConfig.memInc = true;
    m_txConfig.fifoMode = false;  // Has not finished at stream yet!
    //m_txConfig.nvicPriority = 1;    // Debug!
    m_txStream->configurate(&m_txConfig);
    m_txStream->onTransferCompleteSlot(this);
}

void DmaTransceiver::onRxStream(dma::Stream* stream)
{
    if (!stream)
        return;

    m_rxStream = stream;
    m_rxConfig.request = dma::Manager::getInstance().getUartMuxRequestId((uint32_t)m_uart, dma::Direction::Periph2Mem);
    m_rxConfig.mode = dma::Mode::Normal;
    m_rxConfig.periphInc = false;
    m_rxConfig.memInc = true;
    m_rxConfig.fifoMode = false;  // Has not finished at stream yet!
    m_rxStream->configurate(&m_rxConfig);
    m_rxStream->onTransferCompleteSlot(this);
}

void DmaTransceiver::allocateRxBuffer(uint32_t size, bool doubleBufMode)
{
    free(m_rxBuffer);
    free(m_rxBuffer2);
    m_rxBuffer = nullptr;
    m_rxBuffer2 = nullptr;

    if (doubleBufMode)
    {
        m_rxBuffer = reinterpret_cast<uint8_t*>(malloc(size * 2));
        m_rxBuffer2 = &m_rxBuffer[size];
    }
    else
    {
        m_rxBuffer = reinterpret_cast<uint8_t*>(malloc(size));
    }

    m_bufferSize = size;
}

void DmaTransceiver::run(dma::SignalPack pack, uint32_t)
{
    if (pack.streamAddr == (uint32_t*)m_rxStream)         // It's RECEIVING interruption - get data
    {
        m_holder.m_rxRingBuffer.write(pack.data, pack.size);
        // Send data to Offline or Instant signal:
        #ifndef WITH_RTOS
        if (m_holder.m_signalRx.isConnected())
            m_holder.m_signalRx.activ(&m_holder.m_rxRingBuffer);
        else if (m_holder.m_signalInstantRx.isConnected())
            m_holder.m_signalInstantRx.activ(&m_holder.m_rxRingBuffer);
        #else
        if (m_holder.m_signalInstantRx.isConnected())
            m_holder.m_signalInstantRx.activ(&m_holder.m_rxRingBuffer);
        #endif
    }
    else if (pack.streamAddr == (uint32_t*)m_txStream)    // It's TNRANSMITTING interruption - send next data or release
    {
        if (m_nextTxData && m_nextTxSize > 0)
        {
            uint32_t currentSize = m_nextTxSize > dma::maxStreamTransactionSize ? dma::maxStreamTransactionSize : m_nextTxSize;
            m_nextTxSize -= currentSize;
            m_txStream->periphTransfer(m_nextTxData, currentSize, sizeof(uint8_t));
            m_nextTxData = (m_nextTxSize > 0) ? m_nextTxData + currentSize : nullptr;
        }
    }
}

void DmaTransceiver::transmit(uint8_t* data, uint32_t size)
{
    if (!data || size == 0 || !m_txStream || !m_txStream->isInitialised())
        return;

    // Wait for the stream to be released:
    driver::DwtTimer::TimeoutData param = driver::DwtTimer::getInstance().timeoutInit(m_timeoutMs);  
    while (m_txStream->isBusy())
    {
        if (driver::DwtTimer::getInstance().checkTimeout(param))
            return;     // Timeout!
    }

    SET_BIT(m_uart->CR3, USART_CR3_DMAT);
    
    uint32_t currentSize = size > driver::dma::maxStreamTransactionSize ? driver::dma::maxStreamTransactionSize : size;
    m_nextTxSize = size - currentSize;
    m_nextTxData = (m_nextTxSize > 0) ? data + currentSize : nullptr;
    m_txStream->periphTransfer(data, currentSize, sizeof(uint8_t));
}

void DmaTransceiver::startReading(uint16_t size, bool circular)
{
    if (!m_rxStream || !m_rxStream->isInitialised())
        return; // Rx stream not initialised

    if (!m_rxBuffer || m_bufferSize < size)
        return; // Rx buffer not allocated

    if (m_circRxMode != circular) // Re-init to new mode
    {
        if (!circular)  // Switch to general (normal single-request) mode
            m_rxConfig.mode = dma::Mode::Normal;
        else            // Switch to circular mode (double- or single-buffered)
            m_rxConfig.mode = m_rxBuffer2 ? dma::Mode::DoubleBuffM0 : dma::Mode::Circular;

        m_rxStream->configurate(&m_rxConfig);
        m_circRxMode = circular;
    }

    SET_BIT(m_uart->CR3, USART_CR3_DMAR);

    if (m_circRxMode && m_rxBuffer2)
        m_rxStream->periphTransfer(m_rxBuffer, m_rxBuffer2, size, sizeof(uint8_t));   // Double-buffered mode
    else
        m_rxStream->periphTransfer(m_rxBuffer, size, sizeof(uint8_t));  // Simple single-buffered mode
}

bool DmaTransceiver::isTxBusy()
{
    return (m_txStream->isBusy() || (m_nextTxSize > 0));
}

}   // namespace uart
}   // namespace driver
