#pragma once
#include <stdint.h>
#include "stm32h7xx.h"

namespace driver {

class Rcc 
{
public:
    /**
     * @brief Switch MCU clock to external quartz and setup RCC settings
     */
    bool extClockConfig();

    /**
     * @brief Get singleton pointer
     */
    static Rcc& getInstance();

    /**
     * @brief Get clock frequency of periphery device
     * @param [in] deviceAddr Device base address (for exapmle USART1_BASE...)
     * @return Frequency value
     */
    uint32_t getFreq(uint32_t deviceAddr);

private:
    const uint32_t csiFreqHz = 4000000;    // Fixed internal RC generator (CSI) frequency

    enum class PllOutDivider { P, Q, R };
    enum class Pll { PLL1, PLL2, PLL3 };

    enum class PeriphBus
    {
        Ahb12,  // AHB1, AHB2 buses
        Apb1,   // APB1 bus
        Apb2,   // APB2 bus
        Apb3,   // APB3 bus
        Apb4    // APB4 bus
    };

    Rcc() {}
    Rcc(Rcc&) = delete;

    /**
     * @brief Get PLL modules (P, Q, R) frequency
     * @param [in] pll PLL module (1, 2 or 3)
     * @param [in] presc Prescaler out (P, Q or R)
     * @return Frequency in Hz
     */
    uint32_t getPllFreqHz(Pll pll, PllOutDivider presc);

    /**
     * @brief Get PER selector output frequency
     * @return Frequency in Hz
     */
    uint32_t getPerFreq();

    /**
     * @brief Get periphery buses clock frequency
     * @param [in] bus
     * @return Frequency in Hz
     */
    uint32_t getPeriphClock(PeriphBus bus);

    /**
     * @brief Get internal RC generator frequency
     * @details with accounting the HSIDIV divider
     * @return Frequency in Hz
     */
    uint32_t getHsiClock();
};

}   // namespace driver
