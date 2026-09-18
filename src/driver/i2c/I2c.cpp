#include "I2c.h"
#include "driver/Rcc.h"
#include "driver/Dwt.h"
#include "driver/nvic/NvicManager.h"
#include "Loger.h"

namespace driver
{

I2C::I2C(I2C_TypeDef* _i2c) : m_i2c(_i2c), m_dmaTransceiver(*this), m_busyDMA(false)
{}

void I2C::init(Speed speed)
{
    // I2C clocking:
    switch (reinterpret_cast<uint32_t> (m_i2c))
    {
    case I2C1_BASE: SET_BIT(RCC->APB1LENR, RCC_APB1LENR_I2C1EN);
        break;
    case I2C2_BASE: SET_BIT(RCC->APB1LENR, RCC_APB1LENR_I2C2EN);
        break;
    case I2C3_BASE: SET_BIT(RCC->APB1LENR, RCC_APB1LENR_I2C3EN);
        break;
    case I2C4_BASE: SET_BIT(RCC->APB4ENR, RCC_APB4ENR_I2C4EN);
        break;
    }

    // Disable periphery:
    CLEAR_BIT(m_i2c->CR1, I2C_CR1_PE);
    uint32_t timingRegVal = 0;

    // Timing calculation for SCL frequency:
    #ifdef SCL_FREQ_CALCULATE
    m_speed = speed;
    uint32_t i2cClkFreq = Rcc::getInstance().getFreq (reinterpret_cast<uint32_t>(m_i2c));
    if (i2cClkFreq == 0)
        return; // TODO: return false and error log here!
    // Get period of one tick (i2c periphery clock period):
    float clkPeriodUs = 1.0 / (static_cast<float>(i2cClkFreq) / 1000000);
    // Get low and high periods for selected speed:
    float lowLevelTimeUs = (speed == Speed::Std100kHz) ? 4.7 : 1.3;
    float highLevelTimeUs = (speed == Speed::Std100kHz) ? 4.0 : 0.6;
    // How many ticks demand both periods (for current i2c clock frequency) and select the largest of them:
    auto countL = static_cast<uint32_t>(lowLevelTimeUs / clkPeriodUs);
    auto countH = static_cast<uint32_t>(highLevelTimeUs / clkPeriodUs);
    uint32_t count = countL > countH ? countL : countH;
    // Calculate the required Divider (one period, according to its tick count, must fit in 8 bit field):
    uint8_t divider = (count / 250) + (count % 250 ? 1 : 0);
    if (divider == 0)
    {
        return; // TODO: return false and error log here!
    }
    else if (divider > 1)
    {
        // Correct the values according to calculated divider:
        clkPeriodUs *= static_cast<float>(divider); 
        countL = (countL / divider + (countL % divider ? 1 : 0));
        countH = (countH / divider + (countH % divider ? 1 : 0));
    }
    SET_BIT(timingRegVal, countL << I2C_TIMINGR_SCLL_Pos);
    SET_BIT(timingRegVal, countH << I2C_TIMINGR_SCLH_Pos);
    SET_BIT(timingRegVal, (divider - 1) << I2C_TIMINGR_PRESC_Pos);
    SET_BIT(timingRegVal, 2 << I2C_TIMINGR_SCLDEL_Pos);
    #else
    timingRegVal = 0x10707DBC;   // A good timing for 100 kHz (at clock 64MHz)
    //0x00C02670;
    //0x00300F88;
    //0x00300F38;
    //0x1070bcbc;
    #endif

    const uint32_t timingsMask = 0xF0FFFFFF;
    WRITE_REG(m_i2c->TIMINGR, timingRegVal & timingsMask);

    // Set address:
    CLEAR_BIT(m_i2c->OAR1, I2C_OAR1_OA1EN);              // Disable address before setting
    uint32_t OwnAddress1 = 0;                          // TODO: Support 10-bit addresses (if needed)
    WRITE_REG(m_i2c->OAR1, I2C_OAR1_OA1EN | OwnAddress1);
    SET_BIT(m_i2c->CR2, I2C_CR2_AUTOEND | I2C_CR2_NACK);
    CLEAR_BIT(m_i2c->OAR2, I2C_OAR2_OA2EN);              // Disable second address before setting

    // TODO: Add analog and digital filters here (if necessary) 
    // Enable periphery:
    SET_BIT(m_i2c->CR1, I2C_CR1_PE);

    // NVIC initialisation:
    Nvic* nvic = NvicManager::getInstance().getI2CNvic(m_i2c);
    if (nvic)
    {
        IRQn_Type irq;
        if (NvicManager::getInstance().getIrqNumber(reinterpret_cast<uint32_t>(m_i2c), irq))
            nvic->init(irq, 3, *this); // I2C NVIC priority must be lower, than its DMA stream NVIC priority
    }
    m_busyDMA.store(false);
}

void I2C::onRxStream(dma::Stream* stream)
{
    m_dmaTransceiver.onRxStream(stream);
}

void I2C::xferCmpltSubscribeISR(SlotInterface<uint16_t>*slot)
{
    m_XferComplete.connect(slot);
}

bool I2C::masterTx(uint16_t devAddr, uint8_t *source, uint16_t size, uint32_t timeout)
{
    if (!source || size == 0)
        return false;

    // Waiting for device to be released:
    DwtTimer::TimeoutData timeoutParam = driver::DwtTimer::getInstance().timeoutInit(timeout);
    while (READ_BIT(m_i2c->ISR, I2C_ISR_BUSY))
    {
        if (driver::DwtTimer::getInstance().checkTimeout(timeoutParam))
        {
            resetBus();
            return false;
        }
    }

    // Transmitting configuration
    TransferConf conf {0};
    conf.SAddr = devAddr;   // Slave address
    conf.NBytes = size;     // Bytesize
    conf.RW = false;        // Write
    conf.Autoend = true;    // Stop after writing NBytes bytes
    conf.Start = true;      // Generation of Start signal
    conf.Stop = false;      // No Stop signal generation (it sends with Autoend)
    conf.Reload = false;    // Only NBytes (not more)
    transConfig(conf);      // Set config

    while (size > 0)
    {
        // Waiting for I2C to be released:
        timeoutParam = driver::DwtTimer::getInstance().timeoutInit(timeout);
        while (!READ_BIT(m_i2c->ISR, I2C_ISR_TXIS))
        {
            if (driver::DwtTimer::getInstance().checkTimeout(timeoutParam))
            {
                resetBus();
                return false;
            }
        }
        // Data writing
        WRITE_REG(m_i2c->TXDR, *source);
        source++;
        size--;
    }

    // Waiting for Stop flag
    timeoutParam = driver::DwtTimer::getInstance().timeoutInit(timeout);
    while (!READ_BIT(m_i2c->ISR, I2C_ISR_STOPF))
    {
        if (driver::DwtTimer::getInstance().checkTimeout(timeoutParam))
        {
            resetBus();
            return false;
        }
    }
    SET_BIT(m_i2c->ICR, I2C_ISR_STOPF);  // Reset Stop flag

    // Clearing control register
    CLEAR_BIT(m_i2c->CR2, I2C_CR2_SADD | I2C_CR2_HEAD10R | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_RD_WRN);
    return true;
}

bool I2C::masterRx(uint16_t devAddr, uint8_t *dest, uint16_t size, uint32_t timeout)
{
    // Waiting for device to be released:
    DwtTimer::TimeoutData timeoutParam = driver::DwtTimer::getInstance().timeoutInit(timeout);
    while (READ_BIT(m_i2c->ISR, I2C_ISR_BUSY))
    {
        if (driver::DwtTimer::getInstance().checkTimeout(timeoutParam))
        {
            resetBus();
            return false;
        }
    }
    // Receiving configuration
    TransferConf conf {0};
    conf.SAddr = devAddr;   // Slave address
    conf.NBytes = size;     // Bytesize
    conf.RW = true;         // Read
    conf.Autoend = true;    // Stop after writing NBytes bytes
    conf.Start = true;      // Generation of Start signal
    conf.Stop = false;      // No Stop signal generation (it sends with Autoend)
    conf.Reload = false;    // Only NBytes (not more)
    transConfig(conf);      // Set config

    uint8_t *_dest = dest;
    while(size > 0)
    {
        // Waiting until data is available in receiver:
        timeoutParam = driver::DwtTimer::getInstance().timeoutInit(timeout);
        while (!READ_BIT(m_i2c->ISR, I2C_ISR_RXNE))
        {
            if (driver::DwtTimer::getInstance().checkTimeout(timeoutParam))
            {
                resetBus();
                return false;
            }
        }
        // Data reading
        *_dest = READ_REG(m_i2c->RXDR);
        _dest++;
        size--;
    }

    // Waiting for Stop flag
    timeoutParam = driver::DwtTimer::getInstance().timeoutInit(timeout);
    while (!READ_BIT(m_i2c->ISR, I2C_ISR_STOPF))
    {
        if (driver::DwtTimer::getInstance().checkTimeout(timeoutParam))
        {
            resetBus();
            return false;
        }
    }
    SET_BIT(m_i2c->ICR, I2C_ISR_STOPF);   // Reset Stop flag

    // Clearing control register
    CLEAR_BIT(m_i2c->CR2, I2C_CR2_SADD | I2C_CR2_HEAD10R | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_RD_WRN);
    return true;
}

bool I2C::masterTxRx(uint16_t devAddr, uint8_t *source, uint16_t sourceSize, uint8_t *dest, uint32_t destSize, uint32_t timeout)
{
    // Waiting for device to be released:
    DwtTimer::TimeoutData timeoutParam = driver::DwtTimer::getInstance().timeoutInit(timeout);
    while (READ_BIT(m_i2c->ISR, I2C_ISR_BUSY))
    {
        if (driver::DwtTimer::getInstance().checkTimeout(timeoutParam))
        {
            resetBus();
            return false;
        }
    }

    // Transmitting configuration:
    TransferConf conf {0};
    conf.SAddr = devAddr;       // Slave addr
    conf.NBytes = sourceSize;   // Количество байтов для чтения
    conf.RW = false;            // Запись
    conf.Autoend = false;       // Нет автостопа!
    conf.Start = true;          // Генерация Старт-сигнала
    conf.Stop = false;          // Стоп не генерим
    conf.Reload = 0;            // Отправляем только NBytes (не больше)
    transConfig(conf);          // Конфигурация
    while (sourceSize > 0)
    {
        // Ожидание, пока I2C освободится:
        timeoutParam = driver::DwtTimer::getInstance().timeoutInit(timeout);
        while (!READ_BIT(m_i2c->ISR, I2C_ISR_TXIS))
        {
            if (driver::DwtTimer::getInstance().checkTimeout(timeoutParam))
            {
                resetBus();
                return false;
            }
        }

        // Запись данных:
        WRITE_REG(m_i2c->TXDR, *source);   
        source++;
        sourceSize--;
    }

    // Waiting for transmitting xfer is complete:
    timeoutParam = driver::DwtTimer::getInstance().timeoutInit(timeout);
    while (!READ_BIT(m_i2c->ISR, I2C_ISR_TC))
    {
        if (driver::DwtTimer::getInstance().checkTimeout(timeoutParam))
        {
            resetBus();
            return false;
        }
    }

    uint32_t xferSize = (destSize > 0xFF) ? 0xFF : destSize;
    bool reloadMode = destSize > xferSize;
    bool firstXfer = true;
    while (destSize > 0)
    {
        // Receiving configuration:
        conf.NBytes = xferSize;             // Количество байтов для чтения
        conf.RW = 1;                        // Чтение
        conf.Autoend = 0;                   // Нет автостопа!
        conf.Start = firstXfer ? 1 : 0;     // Генерация Старт-сигнала
        conf.Stop = 0;                      // Стоп не генерим (он генерится через Autoend)
        conf.Reload = reloadMode ? 1 : 0;   // Отправляем только NBytes (не больше)
        transConfig(conf);

        for (int i = 0; i < xferSize; i++)
        {
            // Waiting for receiver filling:
            timeoutParam = driver::DwtTimer::getInstance().timeoutInit(timeout);
            while (!READ_BIT(m_i2c->ISR, I2C_ISR_RXNE))
            {
                if (driver::DwtTimer::getInstance().checkTimeout(timeoutParam))
                {
                    resetBus();
                    return false;
                }
            }

            // Data reading:
            *dest = m_i2c->RXDR;
            dest++;
        }
        destSize -= xferSize;
        xferSize = (destSize > 0xFF) ? 0xFF : destSize;
        reloadMode = (destSize > 0xFF);
        firstXfer = false;
    }

    // STOP generation
    SET_BIT(m_i2c->CR2, I2C_CR2_STOP);
    while (!READ_BIT(m_i2c->ISR, I2C_ISR_STOPF));
    // Stop flag reset
    SET_BIT(m_i2c->ICR, I2C_ISR_STOPF);
    // Register CR2 clearing:
    CLEAR_BIT(m_i2c->CR2, I2C_CR2_SADD | I2C_CR2_HEAD10R | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_RD_WRN);
    return true;
}

void I2C::transConfig(TransferConf& conf)
{
    uint32_t tmpreg = 0;
    tmpreg |= (conf.SAddr << 1);  // 7-bit Slave address writes to register with a shift of 1 bit left
    SET_BIT(tmpreg, conf.NBytes << I2C_CR2_NBYTES_Pos);
    SET_BIT(tmpreg, conf.RW << I2C_CR2_RD_WRN_Pos);
    SET_BIT(tmpreg, conf.Start << I2C_CR2_START_Pos);
    SET_BIT(tmpreg, conf.Stop << I2C_CR2_STOP_Pos);
    SET_BIT(tmpreg, conf.Reload << I2C_CR2_RELOAD_Pos);
    SET_BIT(tmpreg, conf.Autoend << I2C_CR2_AUTOEND_Pos);
    uint32_t clearMask = I2C_CR2_SADD | I2C_CR2_RD_WRN | I2C_CR2_START | I2C_CR2_STOP | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_AUTOEND;
    // ACHTUNG! At this point is important to use Modify operation, not a clear-write sequence!
    // Otherwise after CR2 register clearing the flag "Transfer Complete" (not Reload!) will be set immediately,
    // and the further (reload) transactions (without Start) will be impossible.
    MODIFY_REG(m_i2c->CR2, clearMask, tmpreg);
}

void I2C::resetBus()
{
    Log(lmSystem, Info) << "Reset I2C bus...";
    // Clear interruptions enable flags and re-init:
    CLEAR_BIT(m_i2c->CR1, I2C_CR1_TXIE | I2C_CR1_RXIE | I2C_CR1_ERRIE | I2C_CR1_TCIE | I2C_CR1_STOPIE | I2C_CR1_NACKIE);
    init(m_speed);
}

bool I2C::isDmaBusy()
{
    return m_busyDMA.load();
}

void I2C::run(uint8_t, uint32_t)
{
    /* --------------- I S R --------------- */

    /* Transmit interrupt status (Data register is empty and the data can be sent) */
    if (READ_BIT(m_i2c->ISR, I2C_ISR_TXIS))
    {
        if (m_16bitRegAddrMode)
        {
            //  DMA Register Reading Mode: send LSB of 16-bit register address
            m_i2c->TXDR = m_currentRegAddr[0];
            m_16bitRegAddrMode = false;
        }
    }
    /* Transfer complete reload interruption (Multitransfer mode) */
    else if (READ_BIT(m_i2c->ISR, I2C_ISR_TCR))
    {
        if (m_currentXferSize == 0)
        {
            // TODO: Error handler!
        }

        // Clear interrupts flags, enable only errors:
        CLEAR_BIT(m_i2c->CR1, I2C_CR1_TXIE | I2C_CR1_STOPIE | I2C_CR1_TCIE);
        SET_BIT(m_i2c->CR1, I2C_CR1_NACKIE | I2C_CR1_ERRIE);

        // Set config for next DMA data receiving:
        TransferConf conf  
        {
            .RW = 0,
            .Start = 0,
            .Stop = 0,
            .Reload = (m_reloadMode ? 1 : 0),
            .Autoend = (m_reloadMode ? 0 : 1),
            .SAddr = m_currentSlaveAddr,
            .NBytes = m_currentXferSize
        };
        transConfig(conf);  // Set config
        SET_BIT(m_i2c->CR1, I2C_CR1_RXDMAEN);   // Enable I2C DMA Receiver
    }
    /* Transfer complete interruption (in Master mode) */
    else if (READ_BIT(m_i2c->ISR, I2C_ISR_TC))
    {
        /* Set config for DMA data receiving: */
        TransferConf conf  
        {
            .RW = 1,
            .Start = 1,
            .Stop = 0,
            .Reload = (m_reloadMode ? 1 : 0),
            .Autoend = (m_reloadMode ? 0 : 1),
            .SAddr = m_currentSlaveAddr,
            .NBytes = m_currentXferSize
        };
        transConfig(conf);   // Set config
        SET_BIT(m_i2c->CR1, I2C_CR1_RXDMAEN);   // Enable I2C DMA Receiver
    }
    /* STOP detection interruption */
    else if (READ_BIT(m_i2c->ISR, I2C_ISR_STOPF))
    {
        SET_BIT(m_i2c->ICR, I2C_ISR_STOPF);   // Reset the Stop flag
        CLEAR_BIT(m_i2c->CR1, I2C_CR1_TXIE | I2C_CR1_RXIE | I2C_CR1_ERRIE | I2C_CR1_TCIE | I2C_CR1_STOPIE | I2C_CR1_NACKIE);
        m_busyDMA.store(false);
        m_XferComplete.activ(m_currentSlaveAddr);
    }
}

bool I2C::readRegViaDMA(uint16_t devAddr, uint8_t* regAddr, uint8_t regAddrSize, uint8_t *dest, uint32_t destSize)
{
    if (!dest || destSize == 0 || m_busyDMA.load())
        return false;

    if (regAddrSize != sizeof(uint8_t) && regAddrSize != sizeof(uint16_t))
        return false;

    // 1. Collect the new transaction info:
    m_16bitRegAddrMode = (regAddrSize == sizeof(uint16_t));
    memcpy(m_currentRegAddr, regAddr, regAddrSize);
    m_currentDestAddr = dest;
    m_currentSlaveAddr = devAddr;
    m_totalXferSize = destSize;
    if (m_totalXferSize > m_MaxNbytesSize)
    {
        m_currentXferSize = m_MaxNbytesSize;
        m_reloadMode = true;
    }
    else
    {
        m_currentXferSize = m_totalXferSize;
        m_reloadMode = false;
    }

    // 2. Write the Register address (or MSB of uint16_t address):
    m_i2c->TXDR = m_16bitRegAddrMode ? m_currentRegAddr[1] : m_currentRegAddr[0];

    // 3. Prepare the DMA Rx transaction
    if (!m_dmaTransceiver.dmaRxRequest(m_currentDestAddr, m_currentXferSize))
        return false;

    m_busyDMA.store(true);
    m_totalXferSize -= m_currentXferSize;   // Calculate the next xfer size

    // 4. Setup the general (none-DMA trensmitting)
    I2C::TransferConf conf {0};
    conf.SAddr = devAddr;           // Slave address
    conf.NBytes = regAddrSize;      // Bytesize of register address (one or two)
    conf.RW = false;                // Write
    conf.Autoend = false;           // Stop after writing NBytes bytes
    conf.Start = true;              // Generation of Start signal
    conf.Stop = false;              // No Stop signal generation (it sends with Autoend)
    conf.Reload = false;            // Only NBytes (not more)
    transConfig(conf);              // Set config

    // 5. Enable interruptions for general transmitting:
    SET_BIT(m_i2c->CR1, I2C_CR1_TXIE | I2C_CR1_ERRIE | I2C_CR1_TCIE | I2C_CR1_STOPIE | I2C_CR1_NACKIE);
    return true;
}

bool I2C::readRegViaDMA_8bitAddr(uint16_t devAddr, uint8_t regAddr, uint8_t *dest, uint32_t destSize)
{
    return readRegViaDMA(devAddr, (uint8_t*)&regAddr, sizeof(uint8_t), dest, destSize);
}

bool I2C::readRegViaDMA_16bitAddr(uint16_t devAddr, uint16_t regAddr, uint8_t *dest, uint32_t destSize)
{
    return readRegViaDMA(devAddr, (uint8_t*)&regAddr, sizeof(uint16_t), dest, destSize);
}

}   // namespace driver
