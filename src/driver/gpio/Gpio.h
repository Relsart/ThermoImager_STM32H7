#pragma once
#include <stdint.h>
#include "DataTypes.h"
#include "Signal.h"
#include "etl/vector.h"
#include "driver/nvic/Nvic.h"
#include "Exti.h"
#include "Debounce.h"

namespace driver {
namespace gpio {
    
/**
 * @brief Base GPIO driver class
 * @details Slot for pin control (discrete on/off) signals. 
 * Contains debounce (class Contact) functional
 * 
 *  It is not nessesary to create an object for all using pins only for configuration (for example, alternative functions).
 *  Simple invoke the static method config(..).
 * 
 *  EXTI interruption usage:
 *  1. Pin object must be configured as discrete input with some EXTI front (not "none")
 *  2. Connect Pin::extiSignal to subscribers slot
 *  3. Invoke Pin::startExti()
 *  TODO: if nessesary- make functional to stop EXTI handling
 */
class Pin : public SlotInterface <bool>, public Contact
{
private:
    friend class Exti;
    const GpioPort m_GPIOx;     // Port (GPIOA, GPIOB...)  
    const uint8_t m_pin;        // Pin number
    const uint32_t m_pinPos;    // Pin mask (1 << pin)
    ExtiFront m_extiFront;      // Detecting front for EXTI interruptions
    Exti m_exti;                // External interruptions slot
    /**
     * @brief Pin control signal handler
     * @param [in] pinOn on/off
     */ 
    void run(bool pinOn, uint32_t) override;

public:
    /**
     * @brief Pin configurating
     * @param [in] _GPIOx      Port (GPIOA, GPIOB и т.д.)
     * @param [in] _pin        Pin number
     * @param [in] _type       Pin type
     * @param [in] _pull       Pulling
     * @param [in] _speed      Speed
     * @param [in] _altFunc    Alternative function number
     * @param [in] _extiFront  Detecting front for EXTI interruptions
     */
    static void config(GpioPort _GPIOx, uint8_t _pin, PinType _type, Pull _pull, PinSpeed _speed, AltFuncNumber _altFunc = AltFuncNumber::Af0);

    /**
     * @brief General constructor for all parameters
     */ 
    Pin(GpioPort _GPIOx, uint8_t _pin, PinType _type, Pull _pull, PinSpeed _speed, AltFuncNumber _altFunc, ExtiFront _extiFront);

    /**
     * @brief Constructor for discrete signals
     */
    Pin(GpioPort _GPIOx, uint8_t _pin, PinType _type, Pull _pull, ExtiFront _extiFront = ExtiFront::None);

    /**
     * @brief Constructor for alternative functions
     */
    Pin(GpioPort _GPIOx, uint8_t _pin, PinType _type, Pull _pull, PinSpeed _speed, AltFuncNumber _altFunc);

    /**
     * @brief Initialise and start External Interrupt working
     */
    void startExti();

    /**
     * @brief General pin control methods
     */
    void set(bool sw);  // Pin discrete control (on/off)
    void setOn();       // Set pin to high state
    void setOff();      // Set pin to low state
    void invert();      // Pin state inversion
    bool getState();    // Get pin discrete state

    /**
     * @brief Checking pins configuration for erroneus doubled initialization same pin for different purposes
     * @details Invoked at startup after all platform periphery configuration
     */
    static void pinConfigDiagnostic();

    Signal<uint8_t> extiSignal; // EXTI interruption signal (todo -> in private)
};

}   // namespace gpio
}   // namespace driver
