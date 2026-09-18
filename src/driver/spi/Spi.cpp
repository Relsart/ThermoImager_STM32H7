#include "Spi.h"
#include "driver/Dwt.h"
#include "driver/Rcc.h"
#include "driver/nvic/NvicManager.h"
#include "Loger.h"

namespace driver {
namespace spi {

Spi::Spi(SPI_TypeDef* spi) : m_spi(spi), m_dmaTransceiver(*this)
{
    m_transferMode = TransferMethod::General;
}

void Spi::onDmaTxStream(driver::dma::Stream* stream)
{
    m_dmaTransceiver.onTxStream(stream);
}

void Spi::onDmaRxStream(driver::dma::Stream* stream)
{
    m_dmaTransceiver.onRxStream(stream);
}

void Spi::xferCmpltSubscribe(SlotInterface<uint8_t>*slot)
{
    m_XferComplete.connect(slot);
}

void Spi::init(Config config)
{
    // Enable periphery:
    switch (reinterpret_cast<uint32_t>(m_spi))
    {
    case SPI1_BASE:
        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_SPI1EN);
        break;
    case SPI2_BASE:
        SET_BIT(RCC->APB1LENR, RCC_APB1LENR_SPI2EN);
        break;
    case SPI3_BASE:
        SET_BIT(RCC->APB1LENR, RCC_APB1LENR_SPI3EN);
        break;
    case SPI4_BASE:
        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_SPI4EN);
        break;
    case SPI5_BASE:
        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_SPI5EN);
        break;
    case SPI6_BASE:
        SET_BIT(RCC->APB4ENR, RCC_APB4ENR_SPI6EN);
        break;  
    default:
        return;
    }

    stopTransfer();         // Disable SPI
    m_mode = config.mode;   // Save mode

    // Configurations for Master:
    if (config.mode == DeviceMode::Master)
    {
        // SCK signal auto suspending option (in case of FIFO overflov) can prevent data loss
        // Achtung: it works only with frame size >= 8 bit!
        if (config.masterRxAutoSuspend)
            SET_BIT(m_spi->CR1, SPI_CR1_MASRX);
        else
            CLEAR_BIT(m_spi->CR1, SPI_CR1_MASRX);

        // Set frequency of SCK (clock divider)
        MODIFY_REG(m_spi->CFG1, SPI_CFG1_MBR, static_cast<uint8_t>(config.speedDiv) << SPI_CFG1_MBR_Pos);
    }

    // FIFO threshold - frame counts in one package:
    if (config.fifoThreshold == 0)  // min
        config.fifoThreshold = 1;    
    if (config.fifoThreshold > 16)  // max
        config.fifoThreshold = 16;   
    MODIFY_REG(m_spi->CFG1, SPI_CFG1_FTHLV, (config.fifoThreshold - 1) << SPI_CFG1_FTHLV_Pos);    // value in reg -1

    // SlaveSelect signal management:
    if (config.nssMode == CsMode::Soft)
    {
        SET_BIT (m_spi->CFG2, SPI_CFG2_SSM);      // Software control: signal SS is a value of bit SSI in CR1 reg.
        if (((config.mode == DeviceMode::Master) && (config.nssPol == Level::Low)) || ((config.mode == DeviceMode::Slave) && (config.nssPol == Level::High)))
            SET_BIT(m_spi->CR1, SPI_CR1_SSI);
    }
    else
    {
        CLEAR_BIT(m_spi->CFG2, SPI_CFG2_SSM);    // Hardware control: signal SS from GPIO pin
    }
    
    MODIFY_REG(m_spi->CFG2, SPI_CFG2_SSIOP, static_cast<uint8_t>(config.nssPol) << SPI_CFG2_SSIOP_Pos);          // Active level of SS signal (low/high)
    MODIFY_REG(m_spi->CFG2, SPI_CFG2_CPOL, static_cast<uint8_t>(config.sckPol) << SPI_CFG2_CPOL_Pos);            // SCK level in idle mode
    MODIFY_REG(m_spi->CFG2, SPI_CFG2_CPHA, static_cast<uint8_t>(config.sckPhase) << SPI_CFG2_CPHA_Pos);          // Detecting front (change) of signal SCK for data capture.
    MODIFY_REG(m_spi->CFG2, SPI_CFG2_LSBFRST, static_cast<uint8_t>(config.firstBit) << SPI_CFG2_LSBFRST_Pos);    // Transfer order of bits
    MODIFY_REG(m_spi->CFG2, SPI_CFG2_MASTER, static_cast<uint8_t>(m_mode) << SPI_CFG2_MASTER_Pos);               // Spi mode: Master / Slave
    MODIFY_REG(m_spi->CFG2, SPI_CFG2_COMM, static_cast<uint8_t>(config.dirMode) << SPI_CFG2_COMM_Pos);           // Transfer mode (duplex, half-duplex)

    if (config.mode == DeviceMode::Slave)   // Behavior of Slave at underrun condition:
    {
        if (!config.crcEnable)
            MODIFY_REG(m_spi->CFG1, SPI_CFG1_UDRDET, SPI_CFG1_UDRDET_0);    // Detection of underrun condition at end of last data frame
        MODIFY_REG(m_spi->CFG1, SPI_CFG1_UDRCFG, SPI_CFG1_UDRCFG_1);        // -> Repeats its lastly transmitted data frame
    }
    
    CLEAR_BIT(m_spi->I2SCFGR, SPI_I2SCFGR_I2SMOD);   // Disable I2S mode!

    // Enable RX data interrupt (for slave)
    if (config.mode == DeviceMode::Slave)
        SET_BIT(m_spi->IER, SPI_IER_RXPIE);

    // Initialisation interrupt in NVIC class:
    Nvic* nvic = NvicManager::getInstance().getSpiNvic(m_spi);
    
    if (nvic)
    {
        IRQn_Type irq;
        if (NvicManager::getInstance().getIrqNumber(reinterpret_cast<uint32_t>(m_spi), irq))
            nvic->init (irq, 3, *this); // SPI NVIC priority must be lower, than its DMA stream NVIC priority
    }
}

bool Spi::stopSpi(DwtTimer::TimeoutData& timeoutParams)
{
    CLEAR_BIT(m_spi->CR1, SPI_CR1_SPE);      // Disable SPI and wait for disabling
    while (READ_BIT(m_spi->CR1, SPI_CR1_SPE))
    {
        if (driver::DwtTimer::getInstance().checkTimeout(timeoutParams))
        {
            // TODO: error handler may be here
            return false;
        }
    }
    return true;
}

template<typename Type>
void Spi::sendData(Type* data, uint32_t datasize, uint32_t options)
{
    auto txRegPtr8 = (__IO uint8_t*)&m_spi->TXDR;
    auto txRegPtr16 = (__IO uint16_t*)&m_spi->TXDR;
    auto txRegPtr32 = &m_spi->TXDR;
    driver::DwtTimer::TimeoutData timeoutParam; // Argument for dwt timeout
    
    FrameSize frame;
    switch (sizeof(Type))
    {
    case sizeof(uint8_t):  frame = FrameSize::Frame_8bit;  break;
    case sizeof(uint16_t): frame = FrameSize::Frame_16bit; break;
    case sizeof(uint32_t): frame = FrameSize::Frame_32bit; break;
    default: return; // Bad frame size!
    }

    do
    {
        uint32_t transferSize = datasize > transferMaxsize ? transferMaxsize : datasize;
        auto dataPtr8 = (uint8_t*)data;
        auto dataPtr16 = (uint16_t*)data;
        auto dataPtr32 = (uint32_t*)data;

        // Disable SPI and set frame size:
        CLEAR_BIT(m_spi->CR1, SPI_CR1_SPE);
        MODIFY_REG (m_spi->CFG1, SPI_CFG1_DSIZE, static_cast<uint8_t>(frame) << SPI_CFG1_DSIZE_Pos);
        MODIFY_REG(m_spi->CFG2, SPI_CFG2_COMM, SPI_CFG2_COMM_0);    // Set SPI mode = Simplex Transmitter
        MODIFY_REG(m_spi->CR2, SPI_CR2_TSIZE, transferSize);        // Set number of frames (data size)
        SET_BIT(m_spi->CR1, SPI_CR1_SPE);                           // Enable SPI

        if (m_mode == DeviceMode::Master)
            SET_BIT(m_spi->CR1, SPI_CR1_CSTART);  // Start data transfering (if Master)

        for (int i = 0; i < transferSize; i++)
        {
            timeoutParam = driver::DwtTimer::getInstance().timeoutInit(100);
            while (!READ_BIT (m_spi->SR, SPI_SR_TXP))
            {
                if (driver::DwtTimer::getInstance().checkTimeout(timeoutParam))
                    return; // TODO: error log here?
            }
            
            int index = ((options & RepeatedTx) == 0) ? i : 0;
            switch (frame)
            {
            case FrameSize::Frame_8bit:
                *txRegPtr8 = dataPtr8[index];
                break;
            case FrameSize::Frame_16bit:
                *txRegPtr16 = dataPtr16[index];
                break;
            case FrameSize::Frame_32bit:
                 *txRegPtr32 = dataPtr32[index];
                break;
            default:
                return; // TODO: Error log!
            }
        }

        timeoutParam = driver::DwtTimer::getInstance().timeoutInit(1000);
        while(!READ_BIT (m_spi->SR, SPI_SR_EOT))   // Waiting for End Of Transfering
        {
            uint32_t reg = m_spi->SR;
            if (driver::DwtTimer::getInstance().checkTimeout(timeoutParam))
                return; // TODO: error log here?
        }

        stopTransfer();
        datasize -= transferSize;           // Calculate size of next transaction
        if ((options & RepeatedTx) == 0)    // Calculate data address for next transaction
            data += transferSize;

    } while (datasize != 0);
}

template<typename Type>
void Spi::readData(Type* destPtr, uint32_t datasize)
{
    const uint8_t frameByteSize = sizeof (Type);                                            // Frame size in bytes (1/2/4)
    uint32_t transferByteSize = datasize * frameByteSize;                                   // Transaction size in bytes
    uint32_t transferWordSize = (transferByteSize / 4) + (transferByteSize % 4 ? 1 : 0);    // Transaction size in 32-bit words

    union   // Receiving buffer for layouting words to bytes (half-words):
    {
        uint32_t wordBuffer = 0;
        uint8_t getBytes[4];
        uint16_t getHalfWords[2];
    };

    do
    {
        // Calculate currrent transaction size (up to 65535 elements)
        uint16_t thisTransferWordSize = transferWordSize > transferMaxsize ? transferMaxsize : transferWordSize;
        uint16_t transferWordCounter = thisTransferWordSize;    // Decrement counter
        // Disable SPI and set 32-bit frame size:
        CLEAR_BIT(m_spi->CR1, SPI_CR1_SPE);
        MODIFY_REG(m_spi->CFG1, SPI_CFG1_DSIZE, static_cast<uint8_t>(FrameSize::Frame_32bit) << SPI_CFG1_DSIZE_Pos);
        MODIFY_REG(m_spi->CR2, SPI_CR2_TSIZE, thisTransferWordSize);  // Set number of frames (data size)
        MODIFY_REG(m_spi->CFG2, SPI_CFG2_COMM, SPI_CFG2_COMM_1);      // Set SPI mode = Simplex Receiver   
        SET_BIT(m_spi->CR1, SPI_CR1_SPE);                             // Enable SPI
        if (m_mode == DeviceMode::Master)
            SET_BIT(m_spi->CR1, SPI_CR1_CSTART);   // If Master- run transaction

        while (transferWordCounter != 0)    // Word counter in current transaction (up to 65535 frames)
        {
            if (READ_BIT(m_spi->SR, SPI_SR_RXP))  // There is received data in FIFO Rx
            {
                switch (frameByteSize)
                {
                case 1:     // 4 bytes in 1 frame:
                {
                    wordBuffer = m_spi->RXDR;
                    for (int i = 3; i >= 0; i--)
                    {
                        *destPtr++ = getBytes[i];
                        if (--datasize == 0)
                            break;
                    }
                }
                    break;
                case 2:     // 2 half-words in one frame:
                    wordBuffer = m_spi->RXDR;
                    for (int i = 1; i >= 0; i--)
                    {
                        *destPtr++ = getHalfWords[i];
                        if (--datasize == 0)
                            break;
                    }
                    break;
                case 4:     // 1 word in one frame:
                    *destPtr++ = m_spi->RXDR;
                    datasize--;
                    break;
                default:
                    Log (lmSystem, Error) << "SPI: frame size " << frameByteSize << " bytes not supported!";
                    return;
                }
                transferWordCounter--;
            }
        }

        while (!READ_BIT(m_spi->SR, SPI_SR_EOT));   // Wait End Of Transfer (TODO: here should be timeout!).
        stopTransfer();                             // Finish transfer: clear flags
        transferWordSize -= thisTransferWordSize;

    } while (transferWordSize != 0);    // Word counter in all transfer task (not limited by 65535 frames)
}

template<typename Type>
void Spi::sendReadData(Type* dataTx, Type* bufferRx, uint16_t datasize)
{
    auto txRegPtr8 = (__IO uint8_t*)&m_spi->TXDR;
    auto txRegPtr16 = (__IO uint16_t*)&m_spi->TXDR;
    auto rxRegPtr8 = (__IO uint8_t*)&m_spi->RXDR;
    auto rxRegPtr16 = (__IO uint16_t*)&m_spi->RXDR;
    uint16_t txCounter = datasize;
    uint16_t rxCounter = datasize;

    FrameSize frame;
    switch (sizeof(Type))
    {
    case 1: frame = FrameSize::Frame_8bit;  break;
    case 2: frame = FrameSize::Frame_16bit; break;
    case 4: frame = FrameSize::Frame_32bit; break;
    default: return; // Bad frame size!
    }

    // Disable SPI and set frame size:
    CLEAR_BIT (m_spi->CR1, SPI_CR1_SPE);
    MODIFY_REG (m_spi->CFG1, SPI_CFG1_DSIZE, static_cast<uint8_t>(frame) << SPI_CFG1_DSIZE_Pos);
    CLEAR_BIT (m_spi->CFG2, SPI_CFG2_COMM);               // Set Full-Duplex mode
    MODIFY_REG(m_spi->CR2, SPI_CR2_TSIZE, datasize);      // Set number of frames (data size)
    SET_BIT (m_spi->CR1, SPI_CR1_SPE);                    // Enable SPI
    if (m_mode == DeviceMode::Master)
        SET_BIT(m_spi->CR1, SPI_CR1_CSTART);  // If Master- run transaction

    while ((txCounter > 0) || (rxCounter > 0))
    {
        // Transmit:
        if (READ_BIT (m_spi->SR, SPI_SR_TXP) && (txCounter > 0))  // There is data for send and free place in TxFIFO
        {
            switch (frame)
            {
            case FrameSize::Frame_8bit:
                *txRegPtr8 = *dataTx++;
                break;
            case FrameSize::Frame_16bit:
                *txRegPtr16 = *dataTx++;
                break;
            case FrameSize::Frame_32bit:
                m_spi->TXDR = *dataTx++;
                break;
            }
            txCounter--;
        }
        // Recieve:
        if (READ_BIT (m_spi->SR, SPI_SR_RXP) && (rxCounter > 0))  // There is received data in FIFO Rx
        {
            switch (frame)
            {
            case FrameSize::Frame_8bit:
                *bufferRx++ = *rxRegPtr8;
                break;
            case FrameSize::Frame_16bit:
                *bufferRx++ = *rxRegPtr16;
                break;
            case FrameSize::Frame_32bit:
                *bufferRx++ = m_spi->RXDR;
                break;
            }
            rxCounter--;
        }
    }

    while (!READ_BIT(m_spi->SR, SPI_SR_EOT));     // Wait End Of Transfer (TODO: here should be timeout!).
    stopTransfer();                          // Finish transfer: clear flags
}

void Spi::stopTransfer()
{
    SET_BIT(m_spi->IFCR, SPI_IFCR_EOTC);     // Clear End Of Transfer flag
    SET_BIT(m_spi->IFCR, SPI_IFCR_TXTFC);    // Clear Transmission Transfer Filled flag
    CLEAR_BIT(m_spi->CR1, SPI_CR1_CSTART);   // Clear SPI Start Transfer command (must be cleared automatic with EOT)

    // Disable SPI and wait for disabling
    driver::DwtTimer::TimeoutData timeoutParam = driver::DwtTimer::getInstance().timeoutInit(100);
    if (!stopSpi(timeoutParam))
        return;

    uint32_t interruptMask = SPI_IER_EOTIE | SPI_IER_TXPIE | SPI_IER_RXPIE | SPI_IER_DXPIE | SPI_IER_UDRIE | SPI_IER_OVRIE | SPI_IER_TIFREIE | SPI_IER_MODFIE;
    CLEAR_BIT(m_spi->IER, interruptMask);                           // Disable interrupts
    CLEAR_BIT(m_spi->CFG1, SPI_CFG1_TXDMAEN | SPI_CFG1_RXDMAEN);    // Disable DMA handlers
    CLEAR_REG(m_spi->CR2);                                          // Clear data counter
}

void Spi::run(uint8_t, uint32_t)
{
    // This interrupt handler working for DMA, in General mode all transceiving in blocking mode
    stopTransfer(); // Finish transfer: clear flags

    if (m_transferMode == TransferMethod::Dma)
    {
        m_dmaTransceiver.nextTransaction();
        if (!m_dmaTransceiver.isBusy())    // Transaction complete
            m_XferComplete.activ(1);
    }
}

uint32_t Spi::getSckFreq ()
{
    if (m_mode == DeviceMode::Slave)
        return 0;

    uint32_t freq = driver::Rcc::getInstance().getFreq(reinterpret_cast<uint32_t>(m_spi));
    uint8_t divShift = READ_BIT(m_spi->CFG1, SPI_CFG1_MBR) >> SPI_CFG1_MBR_Pos;
    divShift++;
    return freq >> divShift;
}

void Spi::transmit8(uint8_t* data, uint32_t size, uint32_t options)
{
    if (!data || size == 0)
        return;

    m_transferMode = TransferMethod::General;
    sendData<uint8_t>(data, size, options);
}

void Spi::transmit16(uint16_t* data, uint32_t size, uint32_t options)
{
    if (!data || size == 0)
        return;

    m_transferMode = TransferMethod::General;
    sendData<uint16_t>(data, size, options);
}

void Spi::transmit32(uint32_t* data, uint32_t size, uint32_t options)
{
    if (!data || size == 0)
        return;

    m_transferMode = TransferMethod::General;
    sendData<uint32_t>(data, size, options);
}

void Spi::transmit8viaDMA(uint8_t* data, uint32_t size, uint32_t options)
{
    if (!data || size == 0)
        return;

    m_transferMode = TransferMethod::Dma;
    m_dmaTransceiver.transmit(data, sizeof(uint8_t), size, options);
}

void Spi::transmit16viaDMA(uint16_t* data, uint32_t size, uint32_t options)
{
    if (!data || size == 0)
        return;

    m_transferMode = TransferMethod::Dma;
    m_dmaTransceiver.transmit(data, sizeof(uint16_t), size, options);
}

void Spi::transmit32viaDMA(uint32_t* data, uint32_t size, uint32_t options)
{
    if (!data || size == 0)
        return;

    m_transferMode = TransferMethod::Dma;
    m_dmaTransceiver.transmit(data, sizeof(uint32_t), size, options);
}

void Spi::receive8(uint8_t *rxBuffer, uint32_t size)
{
    if (!rxBuffer || size == 0)
        return;

    m_transferMode = TransferMethod::General;
    readData<uint8_t>(rxBuffer, size);
}

void Spi::receive16(uint16_t *rxBuffer, uint32_t size)
{
    if (!rxBuffer || size == 0)
        return;

    m_transferMode = TransferMethod::General;
    readData<uint16_t>(rxBuffer, size);
}

void Spi::receive32(uint32_t *rxBuffer, uint32_t size)
{
    if (!rxBuffer || size == 0)
        return;

    m_transferMode = TransferMethod::General;
    readData<uint32_t>(rxBuffer, size);
}

void Spi::receive8viaDMA(uint8_t *rxBuffer, uint32_t size)
{
    if (!rxBuffer || size == 0)
        return;

    m_transferMode = TransferMethod::Dma;
    m_dmaTransceiver.receive(rxBuffer, sizeof(uint8_t), size);
}

void Spi::receive16viaDMA(uint16_t *rxBuffer, uint32_t size)
{
    if (!rxBuffer || size == 0)
        return;

    m_transferMode = TransferMethod::Dma;
    m_dmaTransceiver.receive(rxBuffer, sizeof(uint16_t), size);
}

void Spi::receive32viaDMA(uint32_t *rxBuffer, uint32_t size)
{
    if (!rxBuffer || size == 0)
        return;

    m_transferMode = TransferMethod::Dma;
    m_dmaTransceiver.receive(rxBuffer, sizeof(uint32_t), size);
}

void Spi::transceive8(uint8_t *txData, uint8_t *rxBuffer, uint16_t size)
{
    if (!txData || !rxBuffer || size == 0)
        return;

    m_transferMode = TransferMethod::General;
    sendReadData<uint8_t>(txData, rxBuffer, size);
}

void Spi::transceive16(uint16_t *txData, uint16_t *rxBuffer, uint16_t size)
{
    if (!txData || !rxBuffer || size == 0)
        return;

    m_transferMode = TransferMethod::General;
    sendReadData<uint16_t>(txData, rxBuffer, size);
}

void Spi::transceive32(uint32_t *txData, uint32_t *rxBuffer, uint16_t size)
{
    if (!txData || !rxBuffer || size == 0)
        return;

    m_transferMode = TransferMethod::General;
    sendReadData<uint32_t>(txData, rxBuffer, size);
}

void Spi::transceive8viaDMA(uint8_t *txData, uint8_t *rxBuffer, uint16_t size)
{
    if (!txData || !rxBuffer || size == 0)
        return;

    m_transferMode = TransferMethod::Dma;
    m_dmaTransceiver.transmitReceive(txData, rxBuffer, sizeof(uint8_t), size);
}

void Spi::transceive16viaDMA(uint16_t *txData, uint16_t *rxBuffer, uint16_t size)
{
    if (!txData || !rxBuffer || size == 0)
        return;

    m_transferMode = TransferMethod::Dma;
    m_dmaTransceiver.transmitReceive(txData, rxBuffer, sizeof(uint16_t), size);
}

void Spi::transceive32viaDMA(uint32_t *txData, uint32_t *rxBuffer, uint16_t size)
{
    if (!txData || !rxBuffer || size == 0)
        return;

    m_transferMode = TransferMethod::Dma;
    m_dmaTransceiver.transmitReceive(txData, rxBuffer, sizeof(uint32_t), size);
}

}   // namespace spi
}   // namespace driver
