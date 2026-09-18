#pragma once
#include "stm32h7xx.h"

namespace driver {
namespace gpio {

typedef GPIO_TypeDef* GpioPort;

/**
 * @brief Pins functional modes (pulling up/down sets in other parameter!)
 */
enum class PinType
{
    Input,              // Discrete input (pull-up, pull-down, floating)
    Inp_analog,         // Analog input
    Out_pushpull,       // Simple active discrete out
    Out_opendrain,      // Open drain output (0-sets to gnd, 1-floating, not connected)
    Alt_pushpull,       // Output of alternative function in general (active) mode
    Alt_opendrain       // Output of alternative function in open drain mode
};

/**
 * @brief Speed modes
 */
enum class PinSpeed
{
    Low = 0,
    Medium = 1,
    High = 2,
    VeryHigh = 3
};

/**
 * @brief EXTI front configuration
 */
enum class ExtiFront
{
    None,           // No EXTI interruption
    RisingFront,
    FailingFront,
    BothFronts
};

/**
 * @brief Pulling mode
 */
enum class Pull
{
    NoPull = 0,     // No pull
    PullUp = 1,     // To Vcc
    PullDown = 2    // To Gnd
};

/**
 * @brief Alternative function numbers
 * @note  ACTHUNG!!! The same function may have different numbers at different pins! Check in datasheet!
 */
enum class AltFuncNumber
{
    Af0    = 0,
    Af1    = 1,
    Af2    = 1,
    Af3    = 3,
    Af4    = 4,
    I2C_1   = Af4,
    I2C_2   = Af4,
    I2C_3   = Af4,
    I2C_4   = Af4,
    Af5    = 5,
    AfSpi1 = Af5,   // MOSI, MISO, SCK, NSS
    AfSpi2 = Af5,   // MOSI, MISO, SCK, NSS
    Af6    = 6,
    Af7    = 7,
    AfUart1 = Af7,
    AfUart2 = Af7,
    AfUart3 = Af7,
    AfUart6 = Af7,
    AfUart7 = Af7,
    Af8    = 8,
    AfUart4 = Af8,
    AfUart5 = Af8,
    AfUart8 = Af8,
    Af9    = 9,
    AfCan_1 = Af9,
    AfCan_2 = Af9,
    SDMMC2_CMD = Af9,
    SDMMC2_CK = Af9,
    SDMMC2_D0 = Af9,
    SDMMC2_D1 = Af9,
    SDMMC2_D3 = Af9,
    Af10   = 10,
    USB_OTG_DM = Af10,
    USB_OTG_DP = Af10,
    SDMMC2_D2 = Af10,
    SDMMC2_D4 = Af10,
    SDMMC2_D5 = Af10,
    SDMMC2_D6 = Af10,
    SDMMC2_D7 = Af10,
    Af11   = 11,
    Af12   = 12,
    SDMMC1_D0 = Af12,
    SDMMC1_D1 = Af12,
    SDMMC1_D2 = Af12,
    SDMMC1_D3 = Af12,
    SDMMC1_D4 = Af12,
    SDMMC1_D5 = Af12,
    SDMMC1_D6 = Af12,
    SDMMC1_D7 = Af12,
    SDMMC1_CMD = Af12,
    SDMMC1_CK = Af12,
    Af13   = 13,
    Af14   = 14,
    Af15   = 15
};

}   // namespace gpio
}   // namespace driver
