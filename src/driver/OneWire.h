#pragma once
#include <stdint.h>
#include <stdio.h>
#include "stm32h7xx.h"
#include "Interface.h"
#include "Dwt.h"

namespace driver {
using namespace i_face;

/**
 * @brief OneWire interface class
 */
class OneWire : public I_OneWire
{
public:
    /**
     * @brief Constructor
     * @param [in] GPIOx Port (GPIOA, GPIOB..)
     * @param [in] pin Pin number
     */
    OneWire(GPIO_TypeDef* GPIOx, uint8_t pin);

    /**
     * @brief Initialization (for GPIO periphery)
     */
    void init();

    /**
     * @brief Bus reset
     * @return True: there is at least one device on the bus, False: no devices
     */
    bool resetBus() override;

    /**
     * @brief Bus scan for connected devices ROM IDs
     * @param [out] serial Buffer for received ROM IDs
     * @param [in] maxDevCount Number of connected devices 
     * @return Number of successfully found devices
     */
    uint8_t scanBus(DeviceRomId* serial, int maxDevCount);

    /**
     * @brief Transmit byte to the bus
     * @param [in] dataByte Data
     */
    void writeByte(uint8_t dataByte) override;

    /**
     * @brief Receive byte from the bus
     * @return Data
     */
    uint8_t readByte() override;

    /**
     * @brief One bit receiving
     * @return Bit value
     */
    bool readBit() override;

    /**
     * @brief One bit transmitting
     * @param [in] bit Bit value
     */
    void writeBit(bool bit) override;

    /**
     * @brief Checksum calculation
     * @param [in] data
     * @param [in] size
     * @return Checksum value
     */
    uint8_t calcCrc(uint8_t* data, uint32_t size) override;

private:
    GPIO_TypeDef* const m_GPIOx;    // GPIO port
    const uint8_t m_pin;            // GPIO pin number
    const uint32_t m_pinMask;       // GPIO pin mask
    DwtTimer& m_delayer;            // Link for precisious us intervals generator

    /**
     * @brief Saved state for devices searching process
     */
    struct SearchState
    {
        uint8_t lastDiscrepancy = 0;
        uint8_t lastFamilyDiscrepancy = 0;
        bool lastDeviceFlag = false;
        uint8_t romIdValue[8]{0};
    };
    SearchState m_searchState;

    /**
     * @brief One device ROM Id searching cycle (64 bits)
     * @details Temporary results and states saved to the SearchState structure
     * @return True: one device ROM Id successfully read; False: error or no more devices left
     */
    bool searchCycle();

    /**
     * @brief Reset the temporary results in SearchState structure
     */
    void searchReset();
};

}   // namespace driver
