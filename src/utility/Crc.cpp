
#include "Crc.h"
#include "driver/Crc.h"


uint16_t crc16Ccitt (const uint8_t* pcBlock, uint16_t len, bool reset)
{
    if (reset)
        return driver::getcrc16 (pcBlock, len, 0xFFFFFFFF, 0x1021, 0x01);
    else
        return driver::getcrc16 (pcBlock, len, 0xFFFFFFFF, 0x1021, 0x00);
}

uint8_t crc8 (const uint8_t* pcBlock, uint16_t len, bool reset)
{
    if (reset)
        return driver::getcrc8 (pcBlock, len, 0xFF, 0x31, 0x01);
    else
        return driver::getcrc8 (pcBlock, len, 0xFF, 0x31, 0x00);
}


