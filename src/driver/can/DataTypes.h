#pragma once
#include <stdint.h>
#include "stm32h7xx.h"

namespace driver {
namespace can {

static constexpr uint8_t maxDataSize = 8;

/**
 * @brief CAN message struct for using in signals
 */
struct Message
{
    uint32_t id = 0;                // Pack ID
    uint8_t data[maxDataSize]{0};   // Data
    uint8_t length = 0;             // Data size (0..8) in bytes
    bool extended = true;           // Extended ID (29 bits) flag
    bool remote = false;            // Remote frame (request without data payload)
};

struct Config
{
    uint32_t bitrateKbs;            // Bitrate in kbit/s
    bool oneShotMode = false;       // Disable frame auto retransmission (One-shot mode)
    bool listenOnlyMode = false;    // Listen-only (silent) mode
    // Todo: sample point here?
};

/**
 * @brief CAN Tx FIFO message element
 */
struct __attribute__((packed)) FifoElement
{
    uint32_t id:29;             // Can message ID
    uint32_t remote:1;          // RTR (remote, without data) flag
    uint32_t exteded:1;         // Extended ID (29 bits) flag
    uint32_t error:1;           // Receive error flag
    uint32_t reserv1:16;
    uint32_t size:4;            // Data size (0..8 bytes)
    uint32_t reserv2:12;
    uint8_t data[maxDataSize];  // Data
};

/**
 * @brief CAN extended filter element
 */
struct __attribute__((packed)) ExtFilterElement
{
    uint32_t efid:29;    // Extended Filter ID
    uint32_t efec:3;     // Extended filter element configuration (see EfecVal)
    uint32_t mask:29;    // Mask (classic filter type)
    uint32_t reserved:1;
    uint32_t efti:2;     // Filter type (see EftiVal)
};

enum class EfecVal : uint8_t
{
    Disable = 0,        // Disable filter elemen
    Fifo_0_Store = 1,   // Store in Rx FIFO 0 if filter matches
    Fifo_1_Store = 2,   // Store in Rx FIFO 1 if filter matches
    RejectID = 3,       // Reject ID if filter matches
    PriorityValid = 4,  // Set priority if filter matches
    PriorityFifo0 = 5,  // Set priority and store in FIFO 0 if filter matches
    PriorityFifo1 = 6,  // Set priority and store in FIFO 1 if filter matches
    StoreRxbuffer = 7   // Store into Rx buffer, configuration of EFTI ignored
};

enum class EftiVal : uint8_t
{
    Range1 = 0,     // Range filter from EF1ID to EF2ID
    DualID = 1,     // Dual ID filter for EF1ID or EF2ID
    Classic = 2,    // Classic filter: filter and mask
    Range2 = 3      // Range filter from EF1ID toEF2ID
};

}   // namespace uart
}   // namespace can
