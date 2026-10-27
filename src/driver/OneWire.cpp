#include "OneWire.h"
#include "gpio/Gpio.h"
#include "Crc.h"
#include "Loger.h"

namespace driver {

OneWire::OneWire(GPIO_TypeDef* GPIOx, uint8_t pin) : m_GPIOx(GPIOx), m_pin(pin), m_pinMask(1U << pin), m_delayer(DwtTimer::getInstance())
{}

void OneWire::init()
{
    gpio::Pin::config(m_GPIOx, m_pin, gpio::PinType::Out_opendrain, gpio::Pull::NoPull, gpio::PinSpeed::VeryHigh);
}

bool OneWire::resetBus()
{
    bool someOnBus = false;
    WRITE_REG(m_GPIOx->BSRR, m_pinMask << 16);  // Push to low
    m_delayer.delayUs(480);
    WRITE_REG(m_GPIOx->BSRR, m_pinMask);        // Release to High (ready to read state)

    /* Critical zone: */
    __disable_irq();
    m_delayer.delayUs(70);  // Wait until the slave device is guaranteed to push the bus to Low
    someOnBus = READ_BIT(m_GPIOx->IDR, m_pinMask) ? false : true;
    __enable_irq();

    m_delayer.delayUs(410);
    return someOnBus;
}

void OneWire::searchReset()
{
    m_searchState.lastDeviceFlag = false;
    m_searchState.lastDiscrepancy = 0;
    m_searchState.lastFamilyDiscrepancy = 0;
    for (uint8_t i = 0; i < sizeof(SearchState::romIdValue); i++)
        m_searchState.romIdValue[i] = 0;
}

uint8_t OneWire::scanBus(DeviceRomId* serial, int maxDevCount)
{
    if (!serial || maxDevCount == 0)
        return 0;
    
    uint8_t sensorCount = 0;
    searchReset();
    while (searchCycle() && sensorCount < maxDevCount)
    {
        memcpy(serial[sensorCount].romArray, m_searchState.romIdValue, 8);
        sensorCount++;
    }
    return sensorCount;
}

bool OneWire::searchCycle()
{
    uint8_t bitPos = 0;     // Address bit position 0..63
    bool searchDirection;
    uint8_t lastZero = 0;
    uint8_t romByteNumber = 0;
    uint8_t romByteMask = 1;

    if (m_searchState.lastDeviceFlag)   // Last device was found on the previous search
        return false;

    /* Reset the bus at every bit checking: */
    if (!resetBus())
    {
        searchReset();
        return false;
    }

    /* Send SearchROM command and results processing cycle: */
    writeByte(static_cast<uint8_t>(Comamnds::SearchRom));
    while (bitPos < 64)
    {
        bool idBit = readBit();      // Read address bit
        bool cmpIdBit = readBit();   // Read inverted address bit

        if (idBit && cmpIdBit)  // No devices at the bus
        {
            break;
        }
        else if (idBit != cmpIdBit)     // No collisions (all devices have the same bit)
        {
            searchDirection = idBit;
        }
        else    // A Collision! There are devices with both 0 and 1 in this bit
        {
            if (bitPos < m_searchState.lastDiscrepancy)
                searchDirection = ((m_searchState.romIdValue[romByteNumber] & romByteMask) > 0);
            else
                searchDirection = (bitPos == m_searchState.lastDiscrepancy);

            if (!searchDirection)
                lastZero = bitPos;
        }

        /* Write result to the address array: */
        if (searchDirection)
            m_searchState.romIdValue[romByteNumber] |= romByteMask;
        else
            m_searchState.romIdValue[romByteNumber] &= ~romByteMask;

        /* Transmitting the selected direction to the devices: */
        writeBit(searchDirection);

        /* Counters incrementation: */
        bitPos++;
        if (romByteMask < 0x80)
        {
            romByteMask <<= 1;
        }
        else
        {
            romByteNumber++;
            romByteMask = 1;
        }
    }
    
    /* Evaluation of search results */
    bool crcOk = (m_searchState.romIdValue[7] == calcCrc(m_searchState.romIdValue, 7));
    bool searchIsOk = false;
    if (bitPos == 64 && (crcOk))
    {
        m_searchState.lastDiscrepancy = lastZero;
        m_searchState.lastDeviceFlag = (m_searchState.lastDiscrepancy == 0);
        searchIsOk = true;
    }

    if (m_searchState.romIdValue[0] == 0)   // Family code can't be zero!
    {
        Log(lmSystem, Error) << "OneWire: invalid family code!"; 
        searchIsOk = false;
    }
    
    if (!crcOk)
        Log(lmSystem, Error) << "OneWire: checksum failed!"; 

    if (!searchIsOk)
        searchReset();  // Search is unsuccessful - reset results

    return searchIsOk;
}

inline bool OneWire::readBit()
{
    bool bitstate = false;

    /* Critical zone: */
    __disable_irq();
    WRITE_REG(m_GPIOx->BSRR, m_pinMask << 16);                      // Push to low
    m_delayer.delayUs(2);
    WRITE_REG(m_GPIOx->BSRR, m_pinMask);                            // Release to High (initial reading impulse)
    m_delayer.delayUs(10);                                          // Waiting until the sensor data is sampled
    bitstate = READ_BIT(m_GPIOx->IDR, m_pinMask) ? true : false;    // Get bit state
    m_delayer.delayUs(50);                                          // Waiting for the end of time slot
    __enable_irq();
    return bitstate;
}

inline void OneWire::writeBit(bool bit)
{
    /* Critical zone: */
    __disable_irq();
    if (bit)
    {
        WRITE_REG(m_GPIOx->BSRR, m_pinMask << 16);  // Push to low
        m_delayer.delayUs(6);
        WRITE_REG(m_GPIOx->BSRR, m_pinMask);        // Release to High
        m_delayer.delayUs(64);
    }
    else
    {
        WRITE_REG(m_GPIOx->BSRR, m_pinMask << 16);  // Push to low
        m_delayer.delayUs(60);
        WRITE_REG(m_GPIOx->BSRR, m_pinMask);        // Release to High
        m_delayer.delayUs(10);
    }
    __enable_irq();
}

void OneWire::writeByte(uint8_t dataByte)
{
    // Transmit starting from the LSB:
    for (uint8_t i = 0; i < 8; i++)
    {
        writeBit(dataByte & (1U << i));
    }
}

uint8_t OneWire::readByte()
{
    uint8_t dataByte = 0;
    // Receive starting from the LSB:
    for (uint8_t i = 0; i < 8; i++)
    {
        if (readBit())
            dataByte |= (1U << i);
    }
    return dataByte;
}

uint8_t OneWire::calcCrc(uint8_t* data, uint32_t size)
{
    if (!data || size == 0)
        return 0;

    return driver::getcrc8(data, size, 0x00, 0x31, 0x07);
}

}   // namespace driver
