#pragma once
#include "stm32h7xx.h"
#include <etl/limits.h>

namespace driver {
namespace spi {

const uint32_t transferMaxsize = etl::numeric_limits<uint16_t>::max();

/**
 * @brief Transfer method
 */
enum class TransferMethod : uint8_t
{
    General,
    Dma
};

/**
 * @brief Spi modes
 */
enum class DeviceMode : uint8_t
{
    Slave = 0,
    Master = 1
};

/**
 * @brief Transaction method
 */
enum class WorkMethod : uint8_t
{
    TwoLines = 0,           // Full duplex
    TwoLinesTxOnly = 1,     // Simplex transmitter
    TwoLinesRxOnly = 2,     // Simplex resiever
    OneLine = 3             // Half-duplex
};

/**
 * @brief Signal level
 */
enum class Level : uint8_t
{
    Low = 0,
    High = 1
};

/**
 * @brief Signal phase
 */
enum class Phase : uint8_t
{
    First = 0,
    Second = 1
};

/**
 * @brief Mode of Slave Select (Chip Select) signal
 */
enum class CsMode : uint8_t
{
    Soft,   // Software
    Hard    // From chip pin
};

/**
 * @brief Frequency prescaler
 */
enum class FreqPrescaler : uint8_t
{
    Div2 = 0,
    Div4 = 1,
    Div8 = 2,
    Div16 = 3,
    Div32 = 4,
    Div64 = 5,
    Div128 = 6,
    Div256 = 7
};

/**
 * @brief Transaction bit order
 */
enum class FirstBit : uint8_t
{
    Msb = 0,
    Lsb = 1
};

/**
 * @brief Frame size
 */
enum class FrameSize : uint8_t
{
    Frame_8bit = 7,
    Frame_16bit = 15,
    Frame_32bit = 31,
};

/**
 * @brief Configurations structure
 */
struct Config
{
    DeviceMode mode;                                // Mode (master/slave)
    FirstBit firstBit = FirstBit::Msb;              // Frame transaction order
    WorkMethod dirMode = WorkMethod::TwoLines;      // Transaction method (full-duplex as default)
    FreqPrescaler speedDiv = FreqPrescaler::Div4;   // Frequency prescaler for master SCK signal
    CsMode nssMode = CsMode::Soft;                  // SS(CS) signal source 
    Level nssPol = Level::Low;                      // Active level of SS (CS) signal
    Level sckPol = Level::Low;                      // SCK level in idle mode (CPOL)
    Phase sckPhase = Phase::First;                  // The front of the SCK signal for frame capturing start (CPHA)
    bool masterRxAutoSuspend = false;               // SCK signal auto suspending by master when FIFO is full (receiving data)
    bool crcEnable = false;                         // Checksum calculating option (has not supported yet)
    uint8_t fifoThreshold = 1;                      // Size of data in one data package. The package size should not exceed 1/2 FIFO (16 bytes for SPI1..3, 8 bytes for SPI4..6)
};

/**
 * @brief Additional options bits
 */
enum OptionFlags
{
    RepeatedTx = 1, // Repeated send one variable (no memory increment)
};

}   // namespace spi
}   // namespace driver
