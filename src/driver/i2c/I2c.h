
#pragma once
#include <stdint.h>
#include "stm32h7xx.h"
#include "TransceiverDma.h"
#include "etl/atomic.h"

/* Option for TIMINGS register calculating. For memory saving it can be switched off, 
   and the previously calculated timings settings can be hardcoded */
#define SCL_FREQ_CALCULATE

namespace driver
{

class I2C : public SlotInterface <Irq>
{
public:
    friend class DmaTransceiver;

    enum class Speed : uint8_t
    {
        Std100kHz,
        Fast400kHz
    };

private:
    static constexpr uint16_t m_MaxNbytesSize = 255;    // Max one transaction data size

    /**
     * @brief Transaction configuration struct
     */
    struct TransferConf
    {
        uint8_t RW : 1;         // Direction (1:read, 0:write)
        uint8_t Start : 1;      // "Start" signal generation
        uint8_t Stop : 1;       // "Stop" signal generation
        uint8_t Reload : 1;     // Not finishing transaction afret NBytes bytes transmittion
        uint8_t Autoend : 1;    // Automatic generation "Stop" signal after NBytes bytes transmittion
        uint8_t SAddr;          // Slave i2c address
        uint8_t NBytes;         // Number of bytes for transmittion
    };

    I2C_TypeDef* const m_i2c;
    DmaTransceiver m_dmaTransceiver;    // DMA transceiver slot
    Signal<uint16_t> m_XferComplete;    // Transaction complete signal. Using with DMA Receiving. Parameter is Slave address
    Speed m_speed;                      // Saved speed configuration (for re-initialisation purpose)

    uint32_t m_currentXferSize;         // Current transaction size in bytes
    uint32_t m_totalXferSize;           // Total transfer size in bytes
    uint16_t m_currentSlaveAddr;        // Saved slave i2c address
    uint8_t* m_currentDestAddr;         // Saved address of data receiving buffer

    uint8_t m_currentRegAddr[sizeof(uint16_t)]; // Saving the register address for sending next byte in the next iteration
    bool m_16bitRegAddrMode = false;            // DMA Register Reading Mode: register address has 16-bit length (so, two tx iterations)
    bool m_reloadMode = false;                  // Reload mode: transaction size exceeds the 255-bytes limit
    etl::atomic<bool> m_busyDMA;                // Transaction via DMA is in process flag

    /**
     * @brief Transaction configuration
     */
    void transConfig(TransferConf& conf);

    /**
     * @brief Bus reset
     */
    void resetBus();

    /**
     * @brief I2C interruptions handler
     */
    void run(uint8_t, uint32_t) override;

    /**
     * @brief Read registers from slave device. Write register address, then read data.
     * @details Register address send in General mode, data receiving- in DMA.
     * @param [in] devAddr slave device address
     * @param [in] regAddr register address
     * @param [in] regAddrSize bytesize of register address (1 or 2)
     * @param [in] dest destination buffer address
     * @param [in] destSize receiving data size
     * @return result: true == Ok (but it is not a transaction finish!)
     */
    bool readRegViaDMA(uint16_t devAddr, uint8_t* regAddr, uint8_t regAddrSize, uint8_t *dest, uint32_t destSize);

public:
    /**
     * @brief Constructor
     * @param [in] i2c Periphery address
     */
    I2C(I2C_TypeDef* i2c);   

    /**
     * @brief initialization
     */
    void init(Speed speed);

    /**
     * @brief Subscribe to receiver DMA stream
     */
    void onRxStream(dma::Stream* stream);

    /**
     * @brief Subscribe to Transfer Finish Slot (for DMA receiving)
     * @details Connected functional will be a part of ISR, so it should be short and fast! 
     */
    void xferCmpltSubscribeISR(SlotInterface<uint16_t>*slot);

    /**
     * @brief Checking stream state
     * @return True == is busy
     */
    bool isDmaBusy();

    /**
     * @brief Read registers from slave device. Write 8-bit register address, then read data.
     * @details Register address send in General mode, data receiving- in DMA.
     * @note User wrapper for readRegViaDMA
     * 
     * @param [in] devAddr slave device address
     * @param [in] regAddr register address (8-bit length)
     * @param [in] dest destination buffer address
     * @param [in] destSize receiving data size (in bytes)
     * @return result: true == Ok (but it is not a transaction finish!)
     */
    bool readRegViaDMA_8bitAddr(uint16_t devAddr, uint8_t regAddr, uint8_t *dest, uint32_t destSize);

    /**
     * @brief Read registers from slave device. Write 16-bit register address, then read data.
     * @details Register address send in General mode, data receiving- in DMA.
     * @note User wrapper for readRegViaDMA
     * 
     * @param [in] devAddr slave device address
     * @param [in] regAddr register address (16-bit length)
     * @param [in] dest destination buffer address
     * @param [in] destSize receiving data size (in bytes)
     * @return result: true == Ok (but it is not a transaction finish!)
     */
    bool readRegViaDMA_16bitAddr(uint16_t devAddr, uint16_t regAddr, uint8_t *dest, uint32_t destSize);

    /**
     * @brief Write data to slave
     * @param [in] devAddr slave device address
     * @param [in] source data for writing
     * @param [in] size data size
     * @param [in] timeout waiting timeout for ready slave device (10ms default)
     * @return result: true == Ok
     */
    bool masterTx(uint16_t devAddr, uint8_t *source, uint16_t size, uint32_t timeout = 100);

    /**
     * @brief Read data from slave
     * @param [in] devAddr slave device address
     * @param [in] dest pointer fo saving readed data
     * @param [in] size readed data size
     * @param [in] timeout waiting timeout for ready slave device (10ms default)
     * @return result: true == Ok
     */
    bool masterRx(uint16_t devAddr, uint8_t *dest, uint16_t size, uint32_t timeout = 100);

    /**
     * @brief Read and write data in one transaction 
     * @param [in] devAddr slave device address
     * @param [in] source data for writing
     * @param [in] sourceSize data size for writing
     * @param [in] dest pointer fo saving readed data
     * @param [in] destSize readed data size
     * @param [in] timeout waiting timeout for ready slave device (10ms default)
     * @return result: true == Ok
     */
    bool masterTxRx(uint16_t devAddr, uint8_t *source, uint16_t sourceSize, uint8_t *dest, uint32_t destSize, uint32_t timeout = 100);
};

}   // namespace driver
