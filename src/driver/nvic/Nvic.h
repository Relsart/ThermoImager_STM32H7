#pragma once
#include <stdint.h>
#include "stm32h7xx.h"
#include "Signal.h"

typedef uint8_t Irq;    // Indicates, that slot connected to interruption

namespace driver {

/**
 * @brief    Nvic interruption class
 * @details  Mainly, it is a wrapper for interruption signal (IrqSignal), which connects to target slot (Timer, SPI, Uart, etc.)
 */
class Nvic
{
private:
    IRQn_Type m_irq;              // Interruption unique number
    bool m_initialised = false;   // Initialised flag
    Signal<Irq> IrqSignal;        // Interruption signal. Activates in system handlers (Nvic.cpp)

    static constexpr uint8_t maxPriorityValue = 15;

public:
    /**
     * @brief initialization
     * @details Enabling interruption in NVIC controller, connect to target slot and set priority
     * @param [in] irq       Interruption unique number (defined in stm32h743xx.h)
     * @param [in] priority  Priority 1 (high) to 15 (low)
     * @param [in] slot      Slot for interruption signal connecting
     */
    void init(IRQn_Type _irq, uint8_t priority, SlotInterface<Irq>& slot);

    /**
     * @brief Set (change) interruption priority
     * @param [in] priority  Priority 1 (high) to 15 (low)
     */
    void setPriority(uint8_t priority);

    /**
     * @brief Interruptions enable
     */
    void enable();

    /**
     * @brief Interruptions disable
     */
    void disable();

    /**
     * @brief Activate interruption signal (from low-leveled system IRQ handlers)
     */
    void activate();
};

/**
 * @brief Hard Fault logging section
 */
namespace hardfault {

constexpr uint8_t FlashLogSector = 7;           // Number of sector in the Flash Memory Bank 2 for Hard fault log
constexpr uint32_t FlashLogAddr = 0x081E0000;   // Last sector (7) of the Flash Memory Bank 2 start address

/**
 * @brief Hard fault log struct
 */
struct HardFaultLog
{
    uint32_t magic; // Preamble value
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t psr;
    uint32_t hfsr;  // HardFault Status Register
    uint32_t cfsr;  // Configurable Fault Status Register
    uint32_t mmar;  // MemManage Fault Address Register (if active)
    uint32_t bfar;  // BusFault Address Register (if active)
    uint32_t reserved[3]; 

};  // Sizeof must be multiple of 32 bytes (32, 64, ...)!

}   // namespace hardfault

}   // namespace driver
