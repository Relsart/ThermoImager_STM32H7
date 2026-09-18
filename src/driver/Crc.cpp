
#include"Crc.h"
#include "stm32h7xx.h"

namespace driver
{

void CRCInit ()
{
    SET_BIT(RCC->AHB4ENR, RCC_AHB4ENR_CRCEN);
}

uint8_t getcrc8 (const uint8_t*data, uint32_t size, uint32_t init, uint32_t poly, uint8_t config)
{
    if ((!data) || (!size))
        return 0;

    WRITE_REG (CRC->CR, CRC_CR_POLYSIZE_1 | 
                        ((config & CRC_CONFIG_REVIN)? CRC_CR_REV_IN_0 : (0 << CRC_CR_REV_IN_Pos)) | 
                        ((config & CRC_CONFIG_REVOUT)? CRC_CR_REV_OUT : (0 << CRC_CR_REV_OUT_Pos)));
    
    if (config & CRC_CONFIG_RESET)
    {
        WRITE_REG (CRC->INIT, init);
        SET_BIT(CRC->CR, CRC_CR_RESET);           
    }

    WRITE_REG (CRC->POL, poly);
    while (size--)
    {
        WRITE_REG (*(__IO uint8_t *) & CRC->DR, *data++);
    }
    return static_cast<uint8_t>(CRC->DR);
}


uint16_t getcrc16 (const uint8_t*data, uint32_t size, uint32_t init, uint32_t poly, uint8_t config)
{
    if ((!data) || (!size))
        return 0;
    
    WRITE_REG (CRC->CR, CRC_CR_POLYSIZE_0 | 
                        ((config & CRC_CONFIG_REVIN)? CRC_CR_REV_IN_0 : (0 << CRC_CR_REV_IN_Pos)) |  
                        ((config & CRC_CONFIG_REVOUT)? CRC_CR_REV_OUT : (0 << CRC_CR_REV_OUT_Pos))); 
   
    if (config & CRC_CONFIG_RESET)
    {
        WRITE_REG (CRC->INIT, init);
        SET_BIT(CRC->CR, CRC_CR_RESET);           
    }
    
    WRITE_REG (CRC->POL, poly);
    while (size--)
    {
         WRITE_REG (*(__IO uint8_t *) & CRC->DR, *data++);
    }
    return (config & CRC_CONFIG_EXOR)? (static_cast<uint16_t>(CRC->DR))^0xFFFF : static_cast<uint16_t>(CRC->DR);
}


uint32_t getcrc32 (const uint32_t*data, uint32_t size, uint32_t init, uint32_t poly, uint8_t config)    
{
    if ((!data) || (!size))
        return 0;

    WRITE_REG (CRC->CR, ((config & CRC_CONFIG_REVIN)? CRC_CR_REV_IN_0 : (0 << CRC_CR_REV_IN_Pos)) | 
                        ((config & CRC_CONFIG_REVOUT)? CRC_CR_REV_OUT : (0 << CRC_CR_REV_OUT_Pos))); 
   
    if (config & CRC_CONFIG_RESET)
    {
        WRITE_REG (CRC->INIT, init);
        SET_BIT(CRC->CR, CRC_CR_RESET);            
    }
    
    WRITE_REG (CRC->POL, poly);
    while (size--)
    {
        WRITE_REG (CRC->DR, *data++);
    }
    return (config & CRC_CONFIG_EXOR)?((CRC->DR)^0xFFFFFFFF):(CRC->DR);  
}

}