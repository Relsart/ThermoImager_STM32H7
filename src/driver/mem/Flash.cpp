#include "Flash.h"

extern "C" {
    
void flashUnlockB2(void)
{
    if (FLASH->CR2 & FLASH_CR_LOCK)
    {
        // This two magic values should be written here to unblock CR2 register:
        FLASH->KEYR2 = 0x45670123;
        FLASH->KEYR2 = 0xCDEF89AB;
    }
}

void flashLockB2(void)
{
    FLASH->CR2 |= FLASH_CR_LOCK;
}

void flashEraseSectorB2(uint8_t sector)
{
    if (sector > 7)
        return; // Sector number out of limit

    while (FLASH->SR2 & FLASH_SR_QW);   // Waiting for the end of the previous operations
    FLASH->CCR2 = 0x00FCffff;           // Error flags clearing

    // Setting up the clearing of the target sector:
    FLASH->CR2 &= ~FLASH_CR_SNB;
    FLASH->CR2 |= (sector << FLASH_CR_SNB_Pos);

    FLASH->CR2 |= FLASH_CR_SER;         // Sector Erase mode enable
    FLASH->CR2 |= FLASH_CR_START;       // Start the operation
    while (FLASH->SR2 & FLASH_SR_QW);   // Waiting for the end of operation
    FLASH->CR2 &= ~FLASH_CR_SER;        // Sector Erase mode disable
}

void flashWrite(uint32_t address, uint32_t* source, uint32_t packsNumber)
{
    while (FLASH->SR2 & FLASH_SR_QW);   // Waiting for the end of the previous operations
    FLASH->CCR2 = 0x00FCffff;           // Error flags clearing
    FLASH->CR2 |= FLASH_CR_PG;          // Enable write operations

    const uint8_t WordsInPack = 8;
    while (packsNumber > 0)
    {
        volatile uint32_t* dest = (volatile uint32_t*)address;
        for (int i = 0; i < WordsInPack; i++)
            dest[i] = source[i];

        while (FLASH->SR2 & FLASH_SR_QW);           // Waiting for the end of operation
        address += sizeof(uint32_t) * WordsInPack;  // Offset the destination address by 32 bytes
        source += WordsInPack;                      // Offset the sourse address by 8 words
        packsNumber--; 
    }
    FLASH->CR2 &= ~FLASH_CR_PG; // Disable write operations
}

} // extern "C"
