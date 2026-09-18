#pragma once
#include <stdint.h>
#include "stm32h7xx.h"

extern "C" {

/**
 * @brief Unblock the access to the Control Register for Bank 2
 */
void flashUnlockB2(void);

/**
 * @brief Block the access to the Control Register for Bank 2
 */
void flashLockB2(void);

/**
 * @brief Sector erasing (128 KBytes)
 * @param [in] sector Sector number
 */
void flashEraseSectorB2(uint8_t sector);

/**
 * @brief Writing data to FLASH memory. Writing in Flash-words, strictly 32 bytes each!
 * @param [in] address Flash memory address
 * @param [in] source Source data address
 * @param [in] packsNumber Number of Flash-words (32 bytes each)
 */
void flashWrite(uint32_t address, uint32_t* source, uint32_t packsNumber = 1);

} // extern "C"
