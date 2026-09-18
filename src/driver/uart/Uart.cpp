#include "Uart.h"
#include "driver/Dwt.h"

namespace driver {
namespace uart {

etl::map<uint8_t, Port*, 10> Port::m_registeredUarts;

Port::Port(USART_TypeDef* uart, uint32_t rxSize) : m_uart(uart),
                                                   m_rxRingBuffer(rxSize),
                                                   m_transceiver(*this),
                                                   m_dmaTransceiver(*this),
                                                   rxRingBufferSize(rxSize)
{
    nvic = NvicManager::getInstance().getUartNvic(m_uart);
}

void Port::init(uint32_t speed, Parity parity)
{
    uint8_t uartNumber;
    // Clock enabling:
    switch (reinterpret_cast<uint32_t>(m_uart))
    {
    case USART1_BASE:
        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_USART1EN);
        uartNumber = 1;
        break;
    case USART2_BASE:
        SET_BIT(RCC->APB1LENR, RCC_APB1LENR_USART2EN);
        uartNumber = 2;
        break;
    case USART3_BASE:
        SET_BIT(RCC->APB1LENR, RCC_APB1LENR_USART3EN);
        uartNumber = 3;
        break;
    case UART4_BASE:
        SET_BIT(RCC->APB1LENR, RCC_APB1LENR_UART4EN);
        uartNumber = 4;
        break;
    case UART5_BASE:
        SET_BIT(RCC->APB1LENR, RCC_APB1LENR_UART5EN);
        uartNumber = 5;
        break;
    case USART6_BASE:
        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_USART6EN);
        uartNumber = 6;
        break;
    case UART7_BASE:
        SET_BIT(RCC->APB1LENR, RCC_APB1LENR_UART7EN);
        uartNumber = 7;
        break;
    case UART8_BASE:
        SET_BIT(RCC->APB1LENR, RCC_APB1LENR_UART8EN);
        uartNumber = 8;
        break;
    }

    CLEAR_BIT(m_uart->CR1, USART_CR1_UE);        // Disable UART
    SET_BIT(m_uart->ICR, Transceiver::icrMask);  // Reset all interruptions flags
    configBaudrate(speed);                     // Set baudrate

    // Parity configurating:
    if (parity == Parity::Even)
    {
        SET_BIT(m_uart->CR1, USART_CR1_PCE);
        CLEAR_BIT(m_uart->CR1, USART_CR1_PS);
    }
    else if (parity == Parity::Odd)
    {
        SET_BIT(m_uart->CR1, USART_CR1_PCE);
        SET_BIT(m_uart->CR1, USART_CR1_PS);
    }
    else    // None
    {
        CLEAR_BIT(m_uart->CR1, USART_CR1_PCE);
        CLEAR_BIT(m_uart->CR1, USART_CR1_PS);
    }

    // Enable receiver, FIFO mode and RxFIFO Not Empty interruptions
    SET_BIT(m_uart->CR1, USART_CR1_RE | USART_CR1_RXNEIE | USART_CR1_FIFOEN);

    // Enable transmitter, FIFO mode
    SET_BIT(m_uart->CR1, USART_CR1_TE | USART_CR1_FIFOEN);

    // Nvic initialization:
    if (nvic)
    {
        IRQn_Type irq;
        if (NvicManager::getInstance().getIrqNumber(reinterpret_cast<uint32_t>(m_uart), irq))
            nvic->init(irq, 3, m_transceiver);
    }

    // Update data in UART ports list:
    if (!m_registeredUarts.contains(uartNumber))
        m_registeredUarts.insert({uartNumber, this});

    SET_BIT(m_uart->CR1, USART_CR1_UE);  // Enable UART
}

void Port::onTxStream(dma::Stream* stream)
{
    if (stream)
        m_dmaTransceiver.onTxStream(stream);
}

void Port::onRxStream(dma::Stream* stream)
{
    if (stream)
        m_dmaTransceiver.onRxStream(stream);
}

void Port::setRxMode(Mode mode)
{
    m_rxMode = mode;
    // Switch off DMA receiver if general:
    if (m_rxMode == Mode::General)
        CLEAR_BIT(m_uart->CR3, USART_CR3_DMAR);
}

void Port::setTxMode(Mode mode)
{
    m_txMode = mode;
    // Switch off DMA receiver if general:
    if (m_txMode == Mode::General)
        CLEAR_BIT(m_uart->CR3, USART_CR3_DMAT);
}

void Port::transmit(void* data, uint32_t size)
{
    if (data == nullptr || size == 0)
        return;
    
    if (m_txMode == Mode::Dma)
        m_dmaTransceiver.transmit(reinterpret_cast<uint8_t*>(data), size);
    else    
        m_transceiver.transmit(reinterpret_cast<uint8_t*>(data), size);
}

void Port::receiveViaDma(uint16_t size, bool circular)
{
    if (m_rxMode == Mode::Dma)
        m_dmaTransceiver.startReading(size, circular);
}

void Port::setSlotRx(SlotInterface<RingBuffer*>*slot)
{
    // Both (real and offline) signals can't be activated at once, so it may be connected one of them:
    if (m_signalInstantRx.isConnected())
        m_signalInstantRx.disconnectAll();    // TODO: warning here!
    #ifndef WITH_RTOS
    m_signalRx.connect(slot);
    #else
    m_signalInstantRx.connect(slot);  // For RTOS use only direct signal
    #endif
}

void Port::setSlotInstantRx(SlotInterface<RingBuffer*>*slot)
{
    // Both (real and offline) signals can't be activated at once, so it may be connected one of them:
    #ifndef WITH_RTOS
    if (m_signalRx.isConnected())
        m_signalRx.disconnectAll();    // TODO: warning here!
    #endif
    m_signalInstantRx.connect(slot);
}

void Port::setDmaRxBufferSize (uint32_t bufferSize, bool doubleBufMode)
{
    //if (bufferSize > rxRingBufferSize)    // Todo: warning here! Ring buffer may overflow!
    m_dmaTransceiver.allocateRxBuffer(bufferSize, doubleBufMode);
}

void Port::setNvicPriority(uint16_t priority)
{
    if (m_dmaTransceiver.m_txStream && m_dmaTransceiver.m_txStream->nvic)
        m_dmaTransceiver.m_txStream->nvic->setPriority(priority);

    if (m_dmaTransceiver.m_rxStream && m_dmaTransceiver.m_rxStream->nvic)
        m_dmaTransceiver.m_rxStream->nvic->setPriority(priority);

    nvic->setPriority(priority);    // General NVIC for rx/tx
}

void Port::run(uint8_t* data, uint32_t size)
{
    transmit(data, size);   // Sending depends on mode (dma/general)
}

void Port::configBaudrate(uint32_t baudrate)
{
    uint32_t freq = Rcc::getInstance ().getFreq (reinterpret_cast<uint32_t>(m_uart));
    m_uart->BRR = (freq + (baudrate / 2)) / baudrate;
    m_baudrate = baudrate;
}

bool Port::dmaTxIsBusy(uint32_t timeoutMs)
{
    if (timeoutMs == 0)
        return (m_dmaTransceiver.isTxBusy());
    else
    {
        driver::DwtTimer::TimeoutData param = driver::DwtTimer::getInstance().timeoutInit(timeoutMs);
        while (m_dmaTransceiver.isTxBusy())
        {
            if (driver::DwtTimer::getInstance().checkTimeout(param))
                return true;
        }
        return false;
    }
}

}   // namespace uart
}   // namespace driver
