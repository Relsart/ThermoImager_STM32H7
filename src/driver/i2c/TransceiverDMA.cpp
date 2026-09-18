#include "TransceiverDma.h"
#include "I2c.h"

namespace driver {

DmaTransceiver::DmaTransceiver(I2C& holder) : m_holder(holder), m_i2c(holder.m_i2c)
{}

void DmaTransceiver::run(dma::SignalPack, uint32_t)
{
    /* Handler of DMA Transfer (Receive) Complete Interruption: */
    CLEAR_BIT(m_i2c->CR1, I2C_CR1_RXDMAEN); // Disable DMA Rx requests
    if (m_holder.m_totalXferSize > 0)       // If there are some data left to read
    {
        m_holder.m_currentDestAddr += m_holder.m_currentXferSize;   // Offset the Rx buffer pointer
        if (m_holder.m_totalXferSize > m_holder.m_MaxNbytesSize)
        {
            m_holder.m_currentXferSize = m_holder.m_MaxNbytesSize;
            m_holder.m_reloadMode = true;
        }
        else
        {
            m_holder.m_currentXferSize = m_holder.m_totalXferSize;
            m_holder.m_reloadMode = false;
        }

        if (m_stream->periphTransfer(m_holder.m_currentDestAddr, m_holder.m_currentXferSize, sizeof(uint8_t)))
        {
            m_holder.m_totalXferSize -= m_holder.m_currentXferSize;
            SET_BIT(m_i2c->CR1, I2C_CR1_TCIE);  // Enable Transfer Complete interruption
        }
    }
    else    // If it was the Last Transaction 
    {
        SET_BIT(m_i2c->CR1, I2C_CR1_TCIE | I2C_CR1_STOPIE);
    }
}

void DmaTransceiver::onRxStream(dma::Stream* stream)
{
    if (!stream)
        return;
    m_stream = stream;
    dma::StreamConfig m_rxConfig;
    m_rxConfig.request = dma::Manager::getInstance().getI2CMuxRequestId((uint32_t)m_i2c, dma::Direction::Periph2Mem);
    m_rxConfig.mode = dma::Mode::Normal;
    m_rxConfig.periphInc = false;
    m_rxConfig.memInc = true;
    m_rxConfig.fifoMode = false;  // Has not finished at stream yet!
    m_stream->configurate(&m_rxConfig);
    m_stream->onTransferCompleteSlot(this);
}

bool DmaTransceiver::dmaRxRequest(uint8_t *dest, uint16_t destSize)
{
    if (!m_stream || !dest || destSize == 0)
        return false;

    return m_stream->periphTransfer(dest, destSize, sizeof(uint8_t));
}

}   // namespace driver
