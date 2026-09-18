#pragma once
#include <stdint.h>

namespace driver
{
namespace usb
{

static constexpr uint8_t StdDevDescrSize = 18;
static constexpr uint8_t StdConfigDescrSize = 41;

static constexpr uint8_t StdDevDescrEP0Size = 64;

static constexpr uint8_t StdDevDescType = 1;
static constexpr uint8_t StdConfigDescType = 2;
static constexpr uint8_t StdInterfaceDescType = 4;

static constexpr uint8_t ReportDescSize = 79;

static constexpr uint8_t MaxPacketSize = 0x40;

enum DTypes : uint8_t
{
    DEV = 1,
    CONF = 2,
    IFACE = 4,
    ENDPOINT = 5,
    HID = 33,
    REPORT = 34,
};


/**
 * @brief Standard USB device descriptor
 */
const uint8_t DeviceDescriptor[StdDevDescrSize] =
{
    StdDevDescrSize,    // bLength: Descriptor size
    DTypes::DEV,        // bDescriptorType: Descriptor type
    0x00, 0x02,         // bcdUSB: USB version: 2.0
    0x00,               // bDeviceClass: Device Class
    0x00,               // bDeviceSubClass: Device SubClass (allways null)
    0x00,               // bDeviceProtocol: Device Protocol (allways null)
    StdDevDescrEP0Size, // bMaxPacketSize0: max package size for zero endpoint
    0x83, 0x04,         // idVendor: (0x0483 == STMicroelectronics)
	0x11, 0x57,         // idProduct: (0x5711 == some AES3500 TruePrint Sensor)
    0x00, 0x01,         // bcdDevice: Number of release
    0x01,               // iManufacturer: Index of the string with Vendor name
    0x02,               // iProduct: Index of the string with Product name
    0x03,               // iSerialNumber: Index of the string with Device Serial Number
    0x01                // bNumConfigurations: Number of possible device configurations
};

const uint8_t HIDConfigDescriptor[StdConfigDescrSize] =
{
    /* ---------- Config section ---------- */
    0x09,                       // bLength: Descriptor size
    DTypes::CONF,               // bDescriptorType: Descriptor type
    StdConfigDescrSize, 0x00,   // wTotalLength: Total data size of this descriptor
    0x01,                       // bNumInterfaces: Number of Interfaces
    0x01,                       // bConfigurationValue: Index of current configuration
    0x00,                       // iConfiguration: Index of the string with with current configuration
    0xE0,                       // bmAttributes: TODO: check this!
    0x32,                       // MaxPower: 100 mA
    /* ---------- Interface section ---------- */
    0x09,                       // bLength: Descriptor size
    DTypes::IFACE,              // bDescriptorType: Descriptor type
    0x00,                       // bInterfaceNumber: Interface number (starts from 0)
    0x00,                       // bAlternateSetting: Alternate interface (not used)
    0x02,                       // bNumEndpoints - Number of End Points (apart from End Point 0)
    0x03,                       // bInterfaceClass: Interface class - HID
    0x00,                       // bInterfaceSubClass
    0x00,                       // nInterfaceProtocol
    0x00,                       // iInterface: Index of the string with description of Interface
    /* ---------- HID descriptor section ---------- */
    0x09,                       // bLength: Descriptor size
    DTypes::HID,                // bDescriptorType: Descriptor type
    0x01, 0x01,                 // bcdHID: HID version (1.1)
    0x00,                       // bCountryCode: if necessary
    0x01,                       // bNumDescriptors: number of REPORT descriptors
    DTypes::REPORT,             // bDescriptorType: Type of Descriptor: report
    ReportDescSize, 0x00,       // wItemLength: Length of Report-descriptor
    /* ---------- EndPoint descriptor #1 (IN) ---------- */
    0x07,                       // bLength: Descriptor size
    DTypes::ENDPOINT,           // bDescriptorType: Descriptor type
    0x81,                       // bEndpointAddress: Address (bits 0..3) and Direction (bit 7: 0=Tx, 1=Rx)
    0x03,                       // bmAttributes: Type = transaction by interruption
    MaxPacketSize, 0x00,        // wMaxPacketSize: Max package size for EndPoint
    0x20,                       // bInterval: Polling interval in milliseconds (32 ms)
    /* ---------- EndPoint descriptor #1 (OUT) ---------- */
    0x07,                       // bLength: Descriptor size
    DTypes::ENDPOINT,           // bDescriptorType: Descriptor type
    0x01,                       // bEndpointAddress: Address (bits 0..3) and Direction (bit 7: 0=Tx, 1=Rx)
    0x03,                       // bmAttributes: Type = transaction by interruption
    MaxPacketSize, 0x00,        // wMaxPacketSize: Max package size for EndPoint
    0x20,                       // bInterval: Polling interval in milliseconds (32 ms)
};

}   // namespace usb
}   // namespace driver
