#include "IrqHandler.h"
#include "driver/Dwt.h"

namespace driver {
namespace can {

IrqHandler::IrqHandler(FDCAN_GlobalTypeDef* can, uint32_t fifoAddr) : m_RxFifoAddr(fifoAddr), m_can(can), m_buffer(512)
{
    Nvic* nvic = NvicManager::getInstance().getCanNvic(m_can);
    if (nvic)
    {
        IRQn_Type irq;
        if (NvicManager::getInstance().getIrqNumber(reinterpret_cast<uint32_t>(m_can), irq))
            nvic->init(irq, 3, *this);
    }
}

void IrqHandler::resetBus()
{
    driver::DwtTimer::TimeoutData param = driver::DwtTimer::getInstance().timeoutInit(500);
    while (!READ_BIT(m_can->CCCR, FDCAN_CCCR_INIT))
    {
        SET_BIT(m_can->CCCR, FDCAN_CCCR_INIT); 
        if (driver::DwtTimer::getInstance().checkTimeout(param))
            return;     // Timeout! TODO: Error log here!
    }
    param = driver::DwtTimer::getInstance().timeoutInit(500);
    while(READ_BIT (m_can->CCCR, FDCAN_CCCR_INIT))
    {
        CLEAR_BIT(m_can->CCCR, FDCAN_CCCR_INIT);
        if (driver::DwtTimer::getInstance().checkTimeout(param))
            return;     // Timeout! TODO: Error log here!
    }
}

void IrqHandler::run (uint8_t, uint32_t)
{
    /* ========== A new message has received: ========== */
    if (READ_BIT(m_can->IR, FDCAN_IR_RF0N))
    {
        SET_BIT (m_can->IR , FDCAN_IR_RF0N);  // Reset this interruption
        while ((m_can->RXF0S & FDCAN_RXF0S_F0FL_Msk) >> FDCAN_RXF0S_F0FL_Pos != 0)  // Fill level of FIFO not null
        {
            volatile uint32_t rxFifoIndex = (m_can->RXF0S & FDCAN_RXF0S_F0GI_Msk) >> FDCAN_RXF0S_F0GI_Pos;
            volatile auto element = reinterpret_cast<const FifoElement*>(m_RxFifoAddr + rxFifoIndex * sizeof (FifoElement));
            if (element->size > maxDataSize)
                return; // TODO: Eror Log

            Message msg;
            msg.remote = (element->remote != 0);
            msg.length = element->size;
            msg.extended = (element->exteded != 0);
            if (msg.extended)
                msg.id = element->id;
            else
                msg.id = (element->id >> 18) & 0x7FF;

            for (int i = 0; i < msg.length; i++)
                msg.data[i] = element->data[i];

            WRITE_REG (m_can->RXF0A, rxFifoIndex);  // Update FIFO index in the acknowledge register
            m_buffer.write(msg);
            #ifndef WITH_RTOS
            m_rxDataSignal.activ(&m_buffer);
            #endif
        } 
    }

    /* ========== Message lost interrupt (FIFO has overflowed?): ========== */
    if (READ_BIT(m_can->IR, FDCAN_IR_RF0L))
    {
        SET_BIT(m_can->IR , FDCAN_IR_RF0L);  // Reset this interruption
    };

    /* ========== Receiver FIFO is full: ========== */
    if (READ_BIT(m_can->IR, FDCAN_IR_RF0F))
    {
        SET_BIT(m_can->IR , FDCAN_IR_RF0F);  // Reset this interruption
        // do something?
    };

    /* ========== Bus-Off error (receiver stops listening to the bus): ========== */
    /* It may appears in case of connection to bus some device with invalid bitrate */
    if (READ_BIT(m_can->IR, FDCAN_IR_BO))
    {
        SET_BIT(m_can->IR, FDCAN_IR_BO);   // Reset this interruption
        resetBus(); // Try to reset the bus
    };

    /* ========== Tx FIFO is empty (Transmitting is available): ========== */
    if (READ_BIT(m_can->IR, FDCAN_IR_TFE))
    {
        SET_BIT(m_can->IR, FDCAN_IR_TFE);  // Reset this interruption
        #ifndef WITH_RTOS
        m_txFinishedSignal.activ(nullptr);
        #endif
    };
}

}   // namespace uart
}   // namespace can
