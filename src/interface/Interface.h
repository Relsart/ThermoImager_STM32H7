#pragma once
#include <stdint.h>
#ifdef WITH_ETL
#include "etl/delegate.h"
#include "etl/string.h"
#else
#include <functional>
#endif

namespace i_face
{

/**
 * @brief DMA bufffer interface
 */
class I_DmaBuffer
{
public:
    virtual ~I_DmaBuffer() = default;    // Interface virtual destructor

    virtual uint32_t* getAddr() = 0;
    virtual const uint32_t* getAddr() const = 0;
    virtual size_t getSize() const = 0;
    virtual uint8_t& operator[](size_t index) = 0;
    virtual const uint8_t& operator[](size_t index) const = 0;
    virtual void clean() const = 0;
    virtual void invalidate() = 0;
};

/**
 * @brief One Wire bus interface
 */
class I_OneWire
{
public:
    /**
     * @brief OneWire general commands codes
     */
    enum class Comamnds : uint8_t
    {
        SearchRom   = 0xF0,  // Search algorithm, which find out the unique 64-bit addresses of all devices on the bus
        SkipRom     = 0xCC,  // Accessing all devices at once
        MatchRom    = 0x55,  // Accessing a specific device by its 64-bit address
        ReadRom     = 0x33,  // Read the address if there is only one device on the bus
        AlarmSearch = 0xEC   // Search for devices that have an alarm flag triggered
    };

    /**
     * @brief Unique device ROM id (8 bytes) with Family Code and CRC
     */
    union DeviceRomId
    {
        struct __attribute__((packed)) RomStruct
        {
            uint8_t familyCode;
            uint8_t serial[6];
            uint8_t crc;
        } romStruct;
        uint8_t romArray[8];
        uint64_t romPack;
    };

    virtual ~I_OneWire() = default;         // Interface virtual destructor
    virtual void writeByte(uint8_t) = 0;    // Transmit byte to the bus
    virtual uint8_t readByte() = 0;         // Receive byte from the bus
    virtual bool resetBus() = 0;            // Bus reset. Return: True: there is at least one device on the bus
    virtual bool readBit() = 0;             // Read one time slot (1 bit)
    virtual void writeBit(bool) = 0;        // One bit transmitting
    virtual uint8_t calcCrc(uint8_t* , uint32_t) = 0;  // Checksum calculation
};

/**
 * @brief Timer interface
 */
class I_Timer
{
public:
    #ifdef WITH_ETL
    using Callback = etl::delegate<void()>;
    #else
    using Callback = std::function<void()>;
    #endif

    /**
     * @brief Timer working modes: one-shot or persistent
     */
    enum class WorkingMode { Once, Periodic };

    virtual ~I_Timer() = default;   // Interface virtual destructor
    virtual void start(uint32_t time, WorkingMode mode = WorkingMode::Once) = 0;    // Start the timer
    virtual void stop() = 0;        // Stop the timer

    void setCallback(Callback h) { m_callback = h; }    // Subscribe for timers callback
    
protected:
    Callback m_callback;
    WorkingMode m_mode;
};

/**
 * @brief Ring Buffer interface
 */
class I_RingBuffer
{
public:
    virtual ~I_RingBuffer() = default;                      // Interface virtual destructor
    virtual size_t getOccupiedSize() const = 0;             // Get received (occupied) byte size
    virtual size_t getFreeSize() const = 0;                 // Get free (available) byte size
    virtual bool push(const uint8_t*, size_t) = 0;          // Write data to buffer
    virtual bool push(uint8_t) = 0;                         // Write one byte to buffer
    virtual bool pushOverwrite(const uint8_t*, size_t) = 0; // Write data to buffer with overwriting old
    virtual size_t pop(uint8_t*, size_t) = 0;               // Read data from buffer
    virtual bool pop(uint8_t&) = 0;                         // Read one byte from buffer
    #ifdef WITH_ETL
    virtual size_t pop(etl::istring* data) = 0;             // Read data from buffer into etl::string
    #endif
    virtual size_t peek(uint8_t*, size_t) = 0;  // Read data from buffer without read pointer moving
    virtual bool peek(uint8_t&) = 0;            // Read one byte from buffer without read pointer moving
    virtual void drop(size_t) = 0;              // Drop (remove) data from buffer by moving its read pointer
    virtual void clear() = 0;                   // Clear the buffer
};

/**
 * @brief Message Ring Buffer interface
 */
template <typename Type>
class I_MessageRingBuffer 
{
public:
    virtual ~I_MessageRingBuffer() = default;
    virtual size_t getOccupiedSize() const = 0;
    virtual size_t getFreeSize() const = 0;
    virtual bool push(const Type& message) = 0;
    virtual bool pushOverwrite(const Type& message) = 0;
    virtual bool pop(Type& dest) = 0;
    virtual bool peek(Type& dest) const = 0;
    virtual void drop(size_t count) = 0;
    virtual void clear() = 0;
};

/**
 * @brief UART interface
 */
class I_Uart
{
public:
    #ifdef WITH_ETL
    using RxCallback = etl::delegate<void(I_RingBuffer&)>;
    #else
    using RxCallback = std::function<void(I_RingBuffer&)>;
    #endif

    /**
     * @brief Working mode
     */
    enum class Mode { Pio, Dma };

    /**
     * @brief Enumeration for Parity setting
     */
    enum Parity { None, Even, Odd };

    virtual ~I_Uart() = default;                                // Interface virtual destructor
    virtual void setRxCallback(RxCallback) = 0;                 // Subscribe for receiving finish callback
    virtual void setRxMode(Mode) = 0;                           // Set receiving mode (PIO or DMA)
    virtual void setTxMode(Mode) = 0;                           // Set transmitting mode (PIO or DMA)
    virtual void transmit(void*, uint32_t) = 0;                 // Send data to port
    virtual void receiveViaDma(uint16_t, bool circular) = 0;    // Start receiving data via DMA stream
    virtual I_RingBuffer& getBuffer() = 0;                      // Get link to ring buffer
};

/**
 * @brief I2C interface
 */
class I_I2C
{
public:
    virtual ~I_I2C() = default;     // Interface virtual destructor
    /* General functional: */
    virtual bool isBusy() = 0;
    virtual void resetBus() = 0;
    virtual bool masterTx(uint16_t addr, uint8_t* src, uint16_t size, uint32_t timeout) = 0;
    virtual bool masterRx(uint16_t addr, uint8_t* dest, uint16_t size, uint32_t timeout) = 0;
    virtual bool masterTxRx(uint16_t addr, uint8_t* src, uint16_t srcSize, uint8_t *dest, uint32_t destSize, uint32_t timeout) = 0;
    /* Extended registers access methods */
    virtual bool read8bReg_DMA(uint16_t addr, uint8_t regAddr, I_DmaBuffer& dest, size_t readSize, size_t bufferOffset = 0) = 0;
    virtual bool read16bReg_DMA(uint16_t addr, uint16_t regAddr, I_DmaBuffer& dest, size_t readSize, size_t bufferOffset = 0) = 0;
};

}   // namespace i_face
