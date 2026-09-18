#pragma once
#include "Signal.h"
#include "driver/SysTimer.h"
#include "Loger.h"

namespace platform {

/**
 * @brief Led-blinker control class
 * @details Indicator of MCU correct working
 */
class LedBlinker : public SlotInterface <Irq>
{
private:
    #ifndef WITH_RTOS
    SignalTime<Irq> m_timeSignal;
    #endif
    driver::gpio::Pin& m_pin;

    void run (Irq , uint32_t) override
    {
        //Log(lmSystem, Info) << "Tick " << driver::getMsTicks();
        m_debugSignal.activ(1);
        m_pin.invert ();
    }

public:
    Signal<Irq> m_debugSignal;

    #ifndef WITH_RTOS
    LedBlinker (driver::gpio::Pin& pin) : m_timeSignal(500), m_pin(pin)
    {
        m_timeSignal.connect(this);
    }
    #else
    LedBlinker (driver::gpio::Pin& pin) : m_pin(pin)
    {}
    #endif
};

}   // namespace platform
