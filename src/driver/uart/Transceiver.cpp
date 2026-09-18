#include "Transceiver.h"
#include "Uart.h"

namespace driver {
namespace uart {

Transceiver::Transceiver(Port& holder) : m_uart(holder.m_uart), m_holder(holder)
{}

void Transceiver::run(uint8_t data, uint32_t size)
{
    // NOT EMPTY RECEIVER FIFO interruption enabled and active (data receiving):
    if (READ_BIT(m_uart->CR1, USART_CR1_RXNEIE) && READ_BIT(m_uart->ISR, USART_ISR_RXNE_RXFNE))
    {
        while (READ_BIT(m_uart->ISR, USART_ISR_RXNE_RXFNE))   // 
        {
            uint8_t data = m_uart->RDR;
            m_holder.m_rxRingBuffer.write(data);
        }

        // send data to ONE OF THE SIGNALS:
        if (m_holder.m_signalInstantRx.isConnected())
            m_holder.m_signalInstantRx.activ(&m_holder.m_rxRingBuffer); // If it necessary- activate instant signal
        #ifndef WITH_RTOS
        if (m_holder.m_signalRx.isConnected())
            m_holder.m_signalRx.activ(&m_holder.m_rxRingBuffer);        // Seng signal to Main loop handling...
        #endif
    }

    // NOT FULL TRANSMITTER FIFO interruption enabled and active (data sending):
    if (READ_BIT(m_uart->CR1, USART_CR1_TXEIE_TXFNFIE) && READ_BIT(m_uart->ISR, USART_ISR_TXE_TXFNF))
    {
        if (!m_txData || m_txDataSize == 0)
        {
            CLEAR_BIT(m_uart->CR1, USART_CR1_TXEIE_TXFNFIE);  // No data to send- disable interruption 
            return;
        }
        while (READ_BIT (m_uart->ISR, USART_ISR_TXE_TXFNF))
        {
            if (m_txDataSize != 0)
            {
                WRITE_REG(m_uart->TDR, *m_txData);
                m_txData++;
                m_txDataSize--;
            }
            else    // No more data- finish transaction: 
            {
                CLEAR_BIT(m_uart->CR1, USART_CR1_TXEIE_TXFNFIE);
                m_txData = nullptr;
                return;
            }
        }
    }

    SET_BIT (m_uart->ICR, icrMask);   // Reset interruptions flags
}


void Transceiver::transmit(uint8_t* data, uint32_t size)
{
    if (!data || size == 0)
        return;

    // Save tx data address, size and enable Tx FIFO Not Full interruptions:
    m_txData = data;
    m_txDataSize = size;
    SET_BIT(m_uart->CR1, USART_CR1_TXEIE_TXFNFIE);
}

}   // namespace uart
}   // namespace driver
