#pragma once

namespace driver {
namespace dma {

/**
 * @brief DMAMUX multiplexor request IDs
 */
enum class MuxRequestIDs
{
    // Memory to Memory transfer:
    MemToMem = 0, 
    // Generators requests:
    Gen0 = 1,
    Gen1 = 2,
    Gen2 = 3,
    Gen3 = 4,
    Gen4 = 5,
    Gen5 = 6,
    Gen6 = 7,
    Gen7 = 8,
    // ADC/DAC requests:
    Adc1 = 9,
    Adc2 = 10,
    Adc3 = 115,
    Dac1Ch1 = 67,
    Dac1Ch2 = 68,
    // Timers requests:
    Tim1Ch1 = 11,
    Tim1Ch2 = 12,
    Tim1Ch3 = 13,
    Tim1Ch4 = 14,
    Tim1Up = 15,
    Tim1Trig = 16,
    Tim1Com = 17,
    Tim2Ch1 = 18,
    Tim2Ch2 = 19,
    Tim2Ch3 = 20,
    Tim2Ch4 = 21,
    Tim2Up = 22,
    Tim3Ch1 = 23,
    Tim3Ch2 = 24,
    Tim3Ch3 = 25,
    Tim3Ch4 = 26,
    Tim3Up = 27,
    Tim3Trig = 28,
    Tim4Ch1 = 29,
    Tim4Ch2 = 30,
    Tim4Ch3 = 31,
    Tim4Up = 32,
    Tim5Ch1 = 55,
    Tim5Ch2 = 56,
    Tim5Ch3 = 57,
    Tim5Ch4 = 58,
    Tim5Up = 59,
    Tim5Trig = 60,
    Tim6Up = 69,
    Tim7Up = 70,
    Tim8Ch1 = 47,
    Tim8Ch2 = 48,
    Tim8Ch3 = 49,
    Tim8Ch4 = 50,
    Tim8Up = 51,
    Tim8Trig = 52,
    Tim8Com = 53,
    Tim15Ch1 = 105,
    Tim15Up = 106,
    Tim15Trig = 107,
    Tim15Com = 108,
    Tim16Ch1 = 109,
    Tim16Up = 110,
    Tim17Ch1 = 111,
    Tim17Up = 112,
    // UART requests:
    Usart1RX = 41,
    Usart1TX = 42,
    Usart2RX = 43,
    Usart2TX = 44,
    Usart3RX = 45,
    Usart3TX = 46,
    Uart4RX = 63,
    Uart4TX = 64,
    Uart5RX = 65,
    Uart5TX = 66,
    Usart6RX = 71,
    Usart6TX = 72,
    Uart7RX = 79,
    Uart7TX = 80,
    Uart8RX = 81,
    Uart8TX = 82,
    // SPI requests:
    Spi1RX = 37,
    Spi1TX = 38,
    Spi2RX = 39,
    Spi2TX = 40,
    Spi3RX = 61,
    Spi3TX = 62,
    Spi4RX = 83,
    Spi4TX = 84,
    Spi5RX = 85,
    Spi5TX = 86,
    // I2C requests:
    I2c1RX = 33,
    I2c1TX = 34,
    I2c2RX = 35,
    I2c2TX = 36,
    I2c3RX = 73,
    I2c3TX = 74,
    // Other requests:
    Dcmi = 75,
    CrypIn = 76,
    CrypOut = 77,
    HashIn = 78,
    Sai1A = 87,
    Sai1B = 88,
    Sai2A = 89,
    Sai2B = 90,
    Sai3A = 113,
    Sai3B = 114,
    SwmpiRX = 91,
    SwmpiTX = 92,
    SpdifRxDt = 93,
    SpdifRxCs = 94,
    HtrimMaster = 95,
    HtrimTimerA = 96,
    HtrimTimerB = 97,
    HtrimTimerC = 98,
    HtrimTimerD = 99,
    HtrimTimerE = 100,
    Dfsdm1Flt0 = 101,
    Dfsdm1Flt1 = 102,
    Dfsdm1Flt2 = 103,
    Dfsdm1Flt3 = 104,
};

/**
 * @brief Stream direction
 */
enum class Direction
{
    Periph2Mem = 0, // Periphery -> to memory
    Mem2Periph = 1, // Memory -> to periphery
    Mem2Mem = 2     // Memory -> to memory
};

/**
 * @brief Stream working mode
 */
enum class Mode
{
    Normal = 0,             // Normal Mode
    PFCtrl = 32,            // Peripheral flow control mode
    Circular = 256,         // Circular (single-buffered) mode
    DoubleBuffM0 = 262144,  // Double-buffered circular mode (first target memory M0)
    DoubleBuffM1 = 786432   // Double-buffered circular mode (first target memory M1)
};

/**
 * @brief Stream (not NVIC!) priority level
 */
enum class Priority
{
    Low = 0,
    Medium = 1,
    High = 2,
    VeryHigh = 3
};

enum class FifoThreshold
{
    Quater = 0,         // 1/4
    Half = 1,           // 1/2
    ThreeQuaters = 2,   // 3/4
    Full = 3            // full
};

enum class Burst
{
    Inc1 = 0,
    Inc4 = 1,
    Inc8 = 2,
    Inc16 = 3
};

enum class IncSize
{
    Byte = 0,       // 8 bits
    HalfWord = 1,   // 16 bits
    Word = 2        // 32 bits
};


/**
 * @brief Stream configuration struct
 */
struct StreamConfig
{
    MuxRequestIDs request;                          // DMAMUX request ID
    Mode mode = Mode::Normal;                       // Stream mode
    bool periphInc = false;                         // Periphery address increment enable (todo:: romove from config to transfer args)
    bool memInc = false;                            // Memory addresss increment ennable (todo:: romove from config to transfer args)
    IncSize periphIncSize = IncSize::Byte;          // Periphery increment size
    IncSize memIncSize = IncSize::Byte;             // Memory increment size
    bool fifoMode = true;                           // FIFO enable
    FifoThreshold fifoThresh = FifoThreshold::Full; // FIFO full threshold
    Priority priority = Priority::Medium;           // Stream priority
    uint8_t nvicPriority = 2;                       // NVIC priority
    Burst memBurst = Burst::Inc1;                   // Burst (packet) mode for memory (packet increment size)
    Burst periphBurst = Burst::Inc1;                // Burst (packet) mode for periphery (packet increment size)
};

/**
 * @brief Struct for interrupt singnal
 */
struct SignalPack
{
    uint32_t* streamAddr = nullptr; // Address of stream (to determine who the message came from)
    uint8_t* data = nullptr;        // Data (for receiving)
    uint32_t size = 0;              // Size of receiving/transmitting data
};

}   // namespace dma
}   // namespace driver
