#pragma once

#include "driver/gpio/Gpio.h"
#include "driver/SysTimer.h"
#include "driver/Crc.h"
#include "driver/spi/Spi.h"
#include "driver/can/Can.h"
#include "driver/i2c/I2c.h"
#include "driver/Rcc.h"
#include "driver/uart/Uart.h"
#include "driver/dma/Stream.h"
#include "driver/nvic/Nvic.h"
#include "driver/usb/Usb.h"
#include "driver/Dwt.h"
#include "driver/sdmmc/Sdmmc.h"
#include "Blinker.h"

#include "MutexWrapper.h"
#include "tasks/ThermoCapture.h"
#include "tasks/ThermoScreen.h"

#include "St77xxDisplay.h" 
#include "MCUPeriphDriver.h"
#include "Mlx90640.h"

using namespace driver;
namespace platform {

//#define SD_CARD_ON_DEVEBOX    /* Option for DevEBox (little black board): init SDMMC pins for microSD card */

/**
 * @brief MCU internal periphery class
 */
class Mcu
{
public:
    // GPIO pins:
    gpio::Pin boardLed;
    gpio::Pin boardButton;
    // ST77xx display additional pins:
    gpio::Pin screenBlk;    // Backlight control pin (LED)
    gpio::Pin screenReset;  // Reset pin (LCD_RST)
    gpio::Pin screenDc;     // Data/Command select pin (LCD_RS)
    gpio::Pin screenCS;     // Display SPI Chip Select pin (LCD_CS)

    gpio::Pin thermoSensorPwr;  // MLX90640 power control pin

    LedBlinker ledBlinker;  // On-board led

    // DMA streams:
    dma::Stream logStream;  // Stream for console logs
    dma::Stream i2cStream;
    dma::Stream spiStream;
    
    // Communication ports and interfaces:
    uart::Port console;
    spi::Spi screenSpi;
    I2C i2cPort;

    /**
     * @brief Constructor.
     */
    Mcu ();

    /**
     * @brief Periphery initialization
     */
    void init ();
};

/**
 * @brief External peripheral devices class
 * @details Sensors, displays etc...
 */
class Peripheral
{
public:
friend class Mcu;
    /**
     * @brief Constructor.
     */
    Peripheral();

    /**
     * @brief Periphery initialization
     * @return True == Ok
     */
    bool init();

private:
    /* Common variables */
    Mcu* const m_Mcu;                       // Link to MCU periphery platform class
    rtos::MutexWrapper m_spiDisplayMutex;   // Display periphery mutex

    /* Sensors and peripheral device classes */
    graphic::st77xx::Display m_display;                 // Display driver
    graphic::st77xx::PeripheryDriver m_displayPeriph;   // Periphery driver for display

    /* Tasks */
    task::ThermoMatrixManager m_thermoMatrixTask;
    TermoScreen m_thermoDisplayTask;
};

extern Mcu mcu;
extern Peripheral peripheral;

}   // namespace platform
