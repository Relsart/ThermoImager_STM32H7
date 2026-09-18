#pragma once
#include <stdint.h>

namespace driver
{
namespace usb
{

enum class FifoType {RX, TX};

enum class UsbDevSpeed
{
    High = 0,
    Full = 2
};

enum class PhyIFace
{
    ULPI = 1,
    EMBEDDED = 2,
    UTMI = 3
};

struct Config
{
    uint8_t dev_endpoints;          // Device Endpoints number (depends on the used USB core) (0..15) Achtung! Was 1..15!
    uint8_t host_channels;          // Host Channels number (depends on the used USB core) (0..15)
    uint8_t dma_enable;             // USB DMA state. Null if DMA is not supported.
    UsbDevSpeed speed;              // USB Core speed
    uint8_t ep0_mps;                // Set the Endpoint 0 Max Packet size
    PhyIFace phy_itface;            // Select the used PHY interface
    uint8_t sof_enable = 0;         // Enable or disable the output of the SOF signal
    uint8_t low_power_enable = 0;   // Enable or disable the low Power Mode
    uint8_t lpm_enable = 0;         // Enable or disable Link Power Management
    uint8_t batChargingEn = 0;      // Enable or disable Battery charging
    uint8_t vbusSensEn = 0;         // Enable or disable the VBUS Sensing feature
    uint8_t use_dedicated_ep1 = 0;  // Enable or disable the use of the dedicated EP1 interrupt
    uint8_t use_external_vbus = 0;  // Enable or disable the use of the external VBUS
};

enum class EndPointType
{
    CTRL = 0,
    ISOC = 1,
    BULK = 2,
    INTR = 3,
    MSK = 3
};

struct EndPoint
{
    uint8_t num;                  // Endpoint number (1..15)
    uint8_t is_in;                // Endpoint direction (0..1)
    uint8_t is_stall;             // Endpoint stall condition (0..1)
    uint8_t is_iso_incomplete;    // Endpoint isoc condition (0..1)
    EndPointType type;            // Endpoint type
    uint8_t data_pid_start;       // Initial data PID (0..1)
    uint32_t maxpacket = 0;       // Endpoint Max packet size (0..64KB)
    uint8_t *xfer_buff = nullptr; // Pointer to transfer buffer
    uint32_t xfer_len = 0;        // Current transfer length
    uint32_t xfer_count;          // Partial transfer length in case of multi packet transfer
    uint8_t even_odd_frame;       // IFrame parity (0..1)
    uint16_t tx_fifo_num;         // Transmission FIFO number (1..15)
    uint32_t dma_addr;            // 32 bits aligned transfer buffer address
    uint32_t  xfer_size;          // requested transfer size
};


}   // namespace usb
}   // namespace driver