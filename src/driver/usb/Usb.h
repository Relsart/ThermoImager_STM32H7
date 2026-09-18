
#pragma once
#include <stdint.h>
#include "stm32h7xx.h"
#include "DataTypes.h"

namespace driver
{
namespace usb
{

class Device
{
private:
    uint32_t const m_UsbBase;
    USB_OTG_GlobalTypeDef* const m_Usb;
    USB_OTG_DeviceTypeDef* const m_UsbDevice;

    USB_OTG_OUTEndpointTypeDef* const m_OutEndPoints;
    USB_OTG_INEndpointTypeDef* const m_InEndPoints;

    uint32_t* const m_UsbPcgcCtrl;
    
    static const uint32_t m_UsbTimeoutMs = 2000;    // TODO: Adjust ??
    static constexpr uint8_t m_MaxEndPointCount = 16;
    EndPoint m_InpEndPointsStorage[m_MaxEndPointCount];    // Array of Input EndPoints
    EndPoint m_OutEndPointsStorage[m_MaxEndPointCount];    // Array of Output EndPoints


    Config m_config;


    bool usbCoreReset();
    void endPointsInit();

    bool flushFifo(FifoType fifoType, uint8_t number = 0);

    /**
     * @brief Set size for TxFIFO
     * @param [in] fifoNumber Number of Tx FIFO
     * @param [in] size FIFO size
     */
    void setTxFifo(uint8_t fifoNumber, uint16_t size);

    /**
     * @brief Set size for RxFIFO
     * @param [in] size FIFO size
     */
    void setRxFifo(uint16_t size);

public:
    Device(USB_OTG_GlobalTypeDef* usb);

    bool init(const Config& cfg);
};

}   // namespace usb
}   // namespace driver
