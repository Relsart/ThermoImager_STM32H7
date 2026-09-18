#pragma once

#include <stdint.h>
#include "stm32h7xx.h"
#include "DataTypes.h"
#include "Receiver.h"

namespace driver {
namespace can {

class Bus : public SlotInterface <Message*>
{
public:
    /**
     * @brief Constructor
     * @param [in] can pointer to periphery device
     */
    Bus(FDCAN_GlobalTypeDef* can);

    /**
     * @brief CAN initialization
     * @param [in] config configutation struct
     */
    void init(Config config);

    /**
     * @brief CAN initialization (overloaded)
     * @param [in] bitrateKbs Bitrate in kbit/s
     */
    void init(uint32_t bitrateKbs);

    /**
     * @brief Send CAN package to the bus
     * @param [in] msg CAN message (ID + data)
     */
    void send(Message &msg);

    /**
     * @brief Add CAN filter
     */
    void addFilter(Filter& filter);

private:
    /**
     * @brief CAN RAM Memory allocation variables
     */
    uint32_t m_StdFiltersCount = 0;                     // Number of standart (11-bit) filters
    uint32_t m_ExtFiltersCount = 0;                     // Number of extended (29-bit) filters
    static const uint32_t m_StdFiltersMaxCount = 10;    // Max number of standart (11-bit) filters
    static const uint32_t m_ExtFiltersMaxCount = 32;    // Max number of extended (29-bit) filters
    static const uint32_t m_RxFifoPacksCount = 16;      // Number of elements of Receiver FIFO
    static const uint32_t m_TxFifoPacksCount = 16;      // Number of elements of Transmitter FIFO
    static const uint8_t m_StdFilterWordSize = 1;       // Size (in 32-bit words) of standart filter in CAN Message memory 
    static const uint8_t m_ExtFilterWordSize = 2;       // Size (in 32-bit words) of extended filter in CAN Message memory
    static const uint8_t m_FifoPackWordSize = 4;        // Size (in 32-bit words) of one FIFO element
    const uint32_t m_ExtFiltersAddr;                    // Start address of extended (29-bits) filters in CAN Message memory
    const uint32_t m_RxFifoAddr;                        // Start address of receiver FIFO in CAN Message memory
    const uint32_t m_TxFifoAddr;                        // Start address of transmitter FIFO in CAN Message memory
    static const uint32_t m_ExtFiltersOffset = m_StdFiltersMaxCount * m_StdFilterWordSize * 2; // Offset (in 32-bit words units) of extended filterd area (after two standart filters areas)
    static const uint32_t m_RxFifoOffset = m_ExtFiltersOffset + m_ExtFiltersMaxCount * m_ExtFilterWordSize * 2;    // Offset (in 32-bit words units) of rx FIFO area (for 2 CANs)
    static const uint32_t m_TxFifoOffset = m_RxFifoOffset + m_RxFifoPacksCount * m_FifoPackWordSize * 2;        // Offset (in 32-bit words units) of tx FIFO area (for 2 CANs)
    static const uint32_t m_initTimeoutMs = 500;

    FDCAN_GlobalTypeDef* const m_can;       // CAN base pointer
    const uint8_t m_canNumber;              // Can bus number (0/1)
    Config m_config;                        // Saved configuration (is necessary to be saved?)
    Receiver receiver;                      // Receiver & filter manager
    MessageRingBuffer<Message> m_txBuffer;  // Buffer for transmitting messages

    

    static bool firstStart;     // First start flag (for any of CANs) 

    /**
     * @brief Signal handler
     */
    void run(Message* msg, uint32_t) override;

    /**
     * @brief Transmit messages from ring buffer to CAN (FIFO)
     */
    void txFromBufferToCan();

    bool enterInitMode();
    bool exitInitMode();

};

}   // namespace uart
}   // namespace can
