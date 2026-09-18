#include "Gpio.h"
#include "Loger.h"

namespace driver {
namespace gpio {
    
/**
 * @brief Simple dynamic array for saving GPIO diagnostic data
 * @note Vectors (std, etl) loses collected data of GPIO, initialised in classes constructors
 * before entering main(), and saving only GPIO, initialised in platform::init
 */
struct PinConfig
{
    GPIO_TypeDef* port;
    uint8_t pin;
};
static PinConfig* diagnosticArray = nullptr;
static uint32_t diagnosticIndex = 0;

inline char getPortLetter(uint32_t gpioBase)
{
    switch (gpioBase)
    {
        case GPIOA_BASE : return 'A';
        case GPIOB_BASE : return 'B';
        case GPIOC_BASE : return 'C';
        case GPIOD_BASE : return 'D';
        case GPIOE_BASE : return 'E';
        case GPIOF_BASE : return 'F';
        case GPIOG_BASE : return 'G';
        case GPIOH_BASE : return 'H';
        case GPIOI_BASE : return 'I';
        case GPIOJ_BASE : return 'J';
        case GPIOK_BASE : return 'K';
    }
    return 0;
}

Pin::Pin(GpioPort GPIOx, uint8_t pin, PinType type, Pull _pull, PinSpeed _speed, AltFuncNumber _altFunc, ExtiFront front) :
    Contact(GPIOx, pin),
    m_exti(*this),
    m_extiFront(front),
    m_GPIOx(GPIOx), 
    m_pin(pin), 
    m_pinPos(1 << pin)
{
    config(m_GPIOx, m_pin, type, _pull, _speed, _altFunc);
}

Pin::Pin(GpioPort GPIOx, uint8_t pin, PinType type, Pull _pull, ExtiFront front) :
    Contact(GPIOx, pin),
    m_exti(*this),
    m_extiFront(front),
    m_GPIOx(GPIOx),
    m_pin(pin),
    m_pinPos(1 << pin)
{
    config(m_GPIOx, m_pin, type, _pull, PinSpeed::Medium, AltFuncNumber::Af0);
}

Pin::Pin(GpioPort GPIOx, uint8_t pin, PinType type, Pull _pull, PinSpeed _speed, AltFuncNumber _altFunc) :
    Contact(GPIOx, pin),
    m_exti(*this),
    m_GPIOx(GPIOx),
    m_pin(pin),
    m_pinPos(1 << pin)
{
    config(m_GPIOx, m_pin, type, _pull, _speed, _altFunc);
}

void Pin::config(GpioPort GPIOx, uint8_t pin, PinType type, Pull _pull, PinSpeed _speed, AltFuncNumber _altFunc)
{
    if (!GPIOx || (pin > 15))
        return;

    // Port clocking enable:
    switch (reinterpret_cast<uint32_t>(GPIOx))
    {
    case GPIOA_BASE:
        SET_BIT (RCC->AHB4ENR, RCC_AHB4ENR_GPIOAEN);
        break;
    case GPIOB_BASE:
        SET_BIT (RCC->AHB4ENR, RCC_AHB4ENR_GPIOBEN);
        break;
    case GPIOC_BASE:
        SET_BIT (RCC->AHB4ENR, RCC_AHB4ENR_GPIOCEN);
        break;
    case GPIOD_BASE:
        SET_BIT (RCC->AHB4ENR, RCC_AHB4ENR_GPIODEN);
        break;
    case GPIOE_BASE:
        SET_BIT (RCC->AHB4ENR, RCC_AHB4ENR_GPIOEEN);
        break;
    case GPIOF_BASE:
        SET_BIT (RCC->AHB4ENR, RCC_AHB4ENR_GPIOFEN);
        break;  
    case GPIOG_BASE:
        SET_BIT (RCC->AHB4ENR, RCC_AHB4ENR_GPIOGEN);
        break;          
    }

    // Pin configuration saving for diagnistic (pinConfigDiagnostic())
    PinConfig pinConfig;
    pinConfig.port = GPIOx;
    pinConfig.pin = pin;
    if (!diagnosticArray)
    {
        diagnosticArray = new PinConfig[150];
        diagnosticIndex = 0;
    }
    diagnosticArray[diagnosticIndex++] = pinConfig;

    // Pin mode configuration:
    uint8_t mode = 0;
    switch (type)
    {
        case PinType::Input:            // Discrete input
            mode = 0b00;
            CLEAR_BIT(GPIOx->OTYPER, 1 << pin);
            break;
        case PinType::Inp_analog:       // Analog input
            mode = 0b11;
            CLEAR_BIT(GPIOx->OTYPER, 1 << pin);
            break;
        case PinType::Out_pushpull:     // Simple active output (0V, 3,3V)
            _pull = Pull::NoPull;       // In output modes no pulling!
            mode = 0b01;
            CLEAR_BIT(GPIOx->OTYPER, 1 << pin);
            break;
        case PinType::Out_opendrain:    // Open-drain output (0-to gnd, 1-floating)
            _pull = Pull::NoPull;       // In output modes no pulling!
            mode = 0b01;
            SET_BIT(GPIOx->OTYPER, 1 << pin);
            break;
        case PinType::Alt_pushpull:     // Alternative ouutput in general (active) mode
        case PinType::Alt_opendrain:    // Alternative ouutput in open-drain mode
            mode = 0b10;
            uint8_t altPosition = (pin % 8) * 4;
            if (pin < 8)
                MODIFY_REG(GPIOx->AFR[0], 0xf << altPosition, static_cast<uint8_t>(_altFunc) << altPosition);
            else
                MODIFY_REG(GPIOx->AFR[1], 0xf << altPosition, static_cast<uint8_t>(_altFunc) << altPosition);

            if (type == PinType::Alt_opendrain)
                SET_BIT(GPIOx->OTYPER, 1 << pin);
            else
                CLEAR_BIT(GPIOx->OTYPER, 1 << pin);
            break;
    }

    MODIFY_REG(GPIOx->MODER, 0b11 << (pin*2), mode << (pin*2));
    MODIFY_REG(GPIOx->PUPDR, 0b11 << (pin*2), static_cast<uint8_t>(_pull) << (pin*2));
}

void Pin::set(bool sw)
{
    if (sw)
        setOn();
    else
        setOff();
}

void Pin::setOn()
{
    WRITE_REG(m_GPIOx->BSRR, m_pinPos);
}

void Pin::setOff()
{
    WRITE_REG(m_GPIOx->BSRR, m_pinPos << 16);
}

void Pin::invert()
{
    if (getState())
        setOff();
    else
        setOn();
}

bool Pin::getState()
{
    return READ_BIT(m_GPIOx->IDR, m_pinPos);
}

void Pin::run(bool pinOn, uint32_t)
{
    if(pinOn)
        setOn();
    else
        setOff();
}

void Pin::pinConfigDiagnostic()
{
    if (!diagnosticArray)
    {
        Log(lmSystem, Error) << "GPIO configuration diagnostic failed: no data!";
        return;
    }

    bool ok = true;
    for (auto i = 0; i < diagnosticIndex; i++)
    {
        PinConfig tmpPin = diagnosticArray[i];
        for (int j = i + 1; j < diagnosticIndex; j++)
        {
            PinConfig checkPin = diagnosticArray[j];
            if (checkPin.pin == tmpPin.pin && checkPin.port == tmpPin.port)
            {
                ok = false;
                Log (lmSystem, Warn) << "Re-initialization of P" << getPortLetter(reinterpret_cast<uint32_t>(checkPin.port)) << static_cast<uint32_t>(tmpPin.pin);
            }
        }
    }
    if (ok)
        Log (lmSystem, Info) << "GPIO initialization OK";

    delete[]diagnosticArray;
}

void Pin::startExti()
{
    if (m_extiFront != ExtiFront::None)
        m_exti.init(m_extiFront);
    else
        Log(lmSystem, Error) << "Not defined EXTI front for GPIO" << getPortLetter(reinterpret_cast<uint32_t>(m_GPIOx)) << static_cast<uint32_t>(m_pin);
}

}   // namespace gpio
}   // namespace driver
