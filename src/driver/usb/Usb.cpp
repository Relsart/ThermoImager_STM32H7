#include "Usb.h"
#include "driver/Dwt.h"

namespace driver
{
namespace usb
{

Device::Device(USB_OTG_GlobalTypeDef* usb) : 
    m_UsbBase(reinterpret_cast<uint32_t>(usb)),
    m_Usb(usb), /* 0x40080000 */
    m_UsbDevice(reinterpret_cast<USB_OTG_DeviceTypeDef*>(m_UsbBase + USB_OTG_DEVICE_BASE)), /* 0x40080800 */
    m_UsbPcgcCtrl(reinterpret_cast<uint32_t*>(m_UsbBase + USB_OTG_PCGCCTL_BASE)),   /* 0x40080e00 */
    m_OutEndPoints(reinterpret_cast<USB_OTG_OUTEndpointTypeDef*>(m_UsbBase + USB_OTG_OUT_ENDPOINT_BASE)), /* from 0x40080b00 */
    m_InEndPoints(reinterpret_cast<USB_OTG_INEndpointTypeDef*>(m_UsbBase + USB_OTG_IN_ENDPOINT_BASE))  /* from 0x40080900 */
{

}

bool Device::usbCoreReset()
{
    auto param = driver::DwtTimer::getInstance().timeoutInit(m_UsbTimeoutMs);
    while (!READ_BIT(m_Usb->GRSTCTL, USB_OTG_GRSTCTL_AHBIDL)) // AHB master idle
    {
        if (driver::DwtTimer::getInstance().checkTimeout(param))
            return false;
    }

    SET_BIT(m_Usb->GRSTCTL, USB_OTG_GRSTCTL_CSRST);     // Core soft reset
    param = driver::DwtTimer::getInstance().timeoutInit(m_UsbTimeoutMs);
    while (READ_BIT(m_Usb->GRSTCTL, USB_OTG_GRSTCTL_CSRST))
    {
        if (driver::DwtTimer::getInstance().checkTimeout(param))
            return false;
    }
    return true;
}

void Device::endPointsInit()
{
    // TODO: Check configs for validity
    for (int i = 0; i < m_config.dev_endpoints; i++)
    {
        m_InpEndPointsStorage[i].is_in = 1;
        m_InpEndPointsStorage[i].num = i;
        m_InpEndPointsStorage[i].tx_fifo_num = i;
        m_InpEndPointsStorage[i].type = EndPointType::CTRL;  // Control until ep is activated
        m_OutEndPointsStorage[i].is_in = 0;
        m_OutEndPointsStorage[i].num = i;
        m_OutEndPointsStorage[i].type = EndPointType::CTRL;
    }
}

bool Device::flushFifo(FifoType fifoType, uint8_t number)
{
    uint32_t flushCmdMsk = fifoType == FifoType::RX ? USB_OTG_GRSTCTL_RXFFLSH : USB_OTG_GRSTCTL_TXFFLSH;
    auto param = driver::DwtTimer::getInstance().timeoutInit(m_UsbTimeoutMs);
    while (!READ_BIT(m_Usb->GRSTCTL, USB_OTG_GRSTCTL_AHBIDL)) // AHB master idle
    {
        if (driver::DwtTimer::getInstance().checkTimeout(param))
            return false;
    }

    m_Usb->GRSTCTL = fifoType == FifoType::RX ? USB_OTG_GRSTCTL_RXFFLSH : (USB_OTG_GRSTCTL_TXFFLSH | number << 6);
    param = driver::DwtTimer::getInstance().timeoutInit(m_UsbTimeoutMs);
    while (READ_BIT(m_Usb->GRSTCTL, flushCmdMsk))
    {
        if (driver::DwtTimer::getInstance().checkTimeout(param))
            return false;
    }
    return true;
}

void Device::setRxFifo(uint16_t size)
{
    m_Usb->GRXFSIZ = size;
}

void Device::setTxFifo(uint8_t fifoNumber, uint16_t size)
{
    /* Note from ST Cube code:  
    TXn min size = 16 words. (n : Transmit FIFO index)
    When a TxFIFO is not used, the Configuration should be as follows:
          case 1 :  n > m    and Txn is not used    (n,m  : Transmit FIFO indexes)
         --> Txm can use the space allocated for Txn.
         case2  :  n < m    and Txn is not used    (n,m  : Transmit FIFO indexes)
         --> Txn should be configured with the minimum space of 16 words
    The FIFO is used optimally when used TxFIFOs are allocated in the top
    of the FIFO.Ex: use EP1 and EP2 as IN instead of EP1 and EP3 as IN ones.
    When DMA is used 3n * FIFO locations should be reserved for internal DMA registers */

    uint32_t txOffset = m_Usb->GRXFSIZ;
    if (fifoNumber == 0U)
    {
        m_Usb->DIEPTXF0_HNPTXFSIZ = ((uint32_t)size << 16) | txOffset;
    }
    else
    {
        txOffset += (m_Usb->DIEPTXF0_HNPTXFSIZ) >> 16;
        for (int i = 0; i < (fifoNumber - 1); i++)
        {
            txOffset += (m_Usb->DIEPTXF[i] >> 16);
        }
        /* Multiply Tx_Size by 2 to get higher performance */
        m_Usb->DIEPTXF[fifoNumber - 1] = ((uint32_t)size << 16) | txOffset;
    }
}


bool Device::init(const Config& cfg)
{
    m_config = cfg; // Save the configurations

    SET_BIT(PWR->CR3, PWR_CR3_USB33DEN);                // Enable USB Voltage detector
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_USB2OTGHSEN);     // Clock enable
    CLEAR_BIT(m_Usb->GAHBCFG, USB_OTG_GAHBCFG_GINT);    // Disable the Interrupts
    SET_BIT(m_Usb->GUSBCFG, USB_OTG_GUSBCFG_PHYSEL);    // USB 1.1 full-speed serial transceiver
    if (!usbCoreReset())	// Reset after a PHY select
        return false;

    SET_BIT(m_Usb->GCCFG, USB_OTG_GCCFG_PWRDWN);        // Activate the USB Transceiver

    /* *********** Set current USB mode (for Device, not Host yet) *********** */

    CLEAR_BIT(m_Usb->GUSBCFG, USB_OTG_GUSBCFG_FHMOD | USB_OTG_GUSBCFG_FDMOD);   // Device and Host -> to Normal (not Force) Mode
    SET_BIT(m_Usb->GUSBCFG, USB_OTG_GUSBCFG_FDMOD);     // Force device mode

    const uint32_t UsbCurrentModeMaxDelayMs = 200;
    auto param = driver::DwtTimer::getInstance().timeoutInit(UsbCurrentModeMaxDelayMs);
    while (READ_BIT(m_Usb->GINTSTS, USB_OTG_GINTSTS_CMOD))  // Must be 0 == Device mode
    {
        if (driver::DwtTimer::getInstance().checkTimeout(param))
            return false;
    }

    /* *********** Endpoints initialisation *********** */

    endPointsInit();

    /* *********** USB device init *********** */

    const uint8_t EndpointsNumber = 8;
    for (int i = 0; i < EndpointsNumber; i++)
        m_Usb->DIEPTXF[i] = 0;

    if (m_config.vbusSensEn == 0)
    {
        SET_BIT(m_UsbDevice->DCTL, USB_OTG_DCTL_SDIS);      // VBUS Sensing setup
        CLEAR_BIT(m_Usb->GCCFG, USB_OTG_GCCFG_VBDEN);       // Deactivate VBUS Sensing B
        SET_BIT(m_Usb->GOTGCTL, USB_OTG_GOTGCTL_BVALOEN);   // B-peripheral session valid override enable
        SET_BIT(m_Usb->GOTGCTL, USB_OTG_GOTGCTL_BVALOVAL);
    }
    else
    {
        SET_BIT(m_Usb->GCCFG, USB_OTG_GCCFG_VBDEN); // Enable HW VBUS sensing
    }

    *m_UsbPcgcCtrl = 0;  // Restart the Phy Clock
    const uint32_t UsbOtgSpeedHigh = 0;
    m_UsbDevice->DCFG = UsbOtgSpeedHigh;    // Set Core speed to Full speed mode

    // FIFOs clearing:
    if (!flushFifo(FifoType::RX) || !flushFifo(FifoType::TX, m_MaxEndPointCount))
        return false;

    // Clear all pending Device Interrupts:
    m_UsbDevice->DIEPMSK = 0;
    m_UsbDevice->DOEPMSK = 0;
    m_UsbDevice->DAINTMSK = 0;

    for (int i = 0; i < m_config.dev_endpoints; i++)
    {
        /* Input endpoints setting */
        if (READ_BIT(m_InEndPoints[i].DIEPCTL, USB_OTG_DIEPCTL_EPENA))  // EndPoint enable
        {
            if (i == 0)
                m_InEndPoints[i].DIEPCTL = USB_OTG_DIEPCTL_SNAK;
            else
                m_InEndPoints[i].DIEPCTL = USB_OTG_DIEPCTL_EPDIS | USB_OTG_DIEPCTL_SNAK;
        }
        else
        {
            m_InEndPoints[i].DIEPCTL = 0;
        }
        m_InEndPoints[i].DIEPTSIZ = 0;
        m_InEndPoints[i].DIEPINT  = 0xFB7F;
        
        /* Output endpoints setting */
        if (READ_BIT(m_OutEndPoints[i].DOEPCTL, USB_OTG_DOEPCTL_EPENA))
        {
            if (i == 0)
                m_OutEndPoints[i].DOEPCTL = USB_OTG_DOEPCTL_SNAK;
            else
                m_OutEndPoints[i].DOEPCTL = USB_OTG_DOEPCTL_EPDIS | USB_OTG_DOEPCTL_SNAK;
        }
        else
        {
            m_OutEndPoints[i].DOEPCTL = 0;
        }
        m_OutEndPoints[i].DOEPTSIZ = 0;
        m_OutEndPoints[i].DOEPINT  = 0xFB7F;
    }

    CLEAR_BIT(m_UsbDevice->DIEPMSK, USB_OTG_DIEPMSK_TXFURM);
    m_Usb->GINTMSK = 0;             // Disable all interrupts
    m_Usb->GINTSTS = 0xBFFFFFFF;    // Clear any pending interrupts
    if (m_config.dma_enable == 0)
        SET_BIT(m_Usb->GINTMSK, USB_OTG_GINTMSK_RXFLVLM);     // Enable the common interrupts

    uint32_t devIntMask = USB_OTG_GINTMSK_USBSUSPM | USB_OTG_GINTMSK_USBRST |
                          USB_OTG_GINTMSK_ENUMDNEM | USB_OTG_GINTMSK_IEPINT |
                          USB_OTG_GINTMSK_OEPINT   | USB_OTG_GINTMSK_IISOIXFRM |
                          USB_OTG_GINTMSK_PXFRM_IISOOXFRM | USB_OTG_GINTMSK_WUIM;

    SET_BIT(m_Usb->GINTMSK, devIntMask);    // Enable interrupts matching to the Device mode ONLY 

    if (m_config.sof_enable != 0)
        SET_BIT(m_Usb->GINTMSK, USB_OTG_GINTMSK_SOFM);
    if (m_config.vbusSensEn == 1)
        SET_BIT(m_Usb->GINTMSK, (USB_OTG_GINTMSK_SRQIM | USB_OTG_GINTMSK_OTGINT));

    /* Disconnect the USB device by disabling Rpu */
    CLEAR_BIT(*m_UsbPcgcCtrl, USB_OTG_PCGCCTL_STOPCLK | USB_OTG_PCGCCTL_GATECLK);
    CLEAR_BIT(m_UsbDevice->DCTL, USB_OTG_DCTL_SDIS);
    setRxFifo(0x80);    // Set FIFOs sizes
    setTxFifo(0, 0x40);
    setTxFifo(1, 0x80);


    return true;
}

}   // namespace usb
}   // namespace driver
