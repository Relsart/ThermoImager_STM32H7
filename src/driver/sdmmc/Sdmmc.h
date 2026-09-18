#pragma once
#include <stdint.h>
#include "DataTypes.h"
#include "stm32h7xx.h"
#include "driver/nvic/Nvic.h"
#include "Terminal.h"
#include "Idma.h"
#include "SdCardInit.h"

namespace driver {

Log& operator<<(Log& out, const sdmmc::Result& result);

using namespace sdmmc;

/**
 * @brief Драйвер работы с картами SD.
 * 
 * @note ACHTUNG! Flags SDMMC_STA_CPSMACT and SDMMC_STA_DPSMACT in stm32h743xx.h can be mixed up in places! It should be like this:
 *       SDMMC_STA_DPSMACT = bit 12 (in STA register)
 *       SDMMC_STA_CPSMACT = bit 13
 * 
 * @note ACHTUNG! Для больших объемов записи (например, при работе с файловой системой FatFs) использовать метод записи через IDMA (writeBlocksViaIDMA).
 *       По невыясленным причинам обычный метод записи (writeBlocks) нестабильно работает с некоторыми SD картами. Падает в Timeout, не видит бит завершения копирования и т.п.
 *       Самый наглядный пример- при форматировании карты (не любой!) падает в ошибку FR_DISK_ERR (timeout). При использовании IDMA все работает без замечаний.
 * 
 * @note По невыясненным причинам нестабильно работает мультиблоковая запись через IDMA (CMD 25). После ее завершения (срабатывания прерывания по завершению передачи)
 *       не срабатывает функция чтения (падает в ошибку таймаута команды). С одноблоковой записью (CMD 24) такой проблемы не наблюдается. Поэтому функция записи 
 *       в модуле IDMA сделана только с командами одноблоковой записи (по принципу одна многоблоковая запись = несколько отдельных одноблоковых).
 * 
 **/

class SdCard
{
private:
    friend class sdmmc::IdmaSlot;

    CommandSender m_cmdSender;      // Command manager
    Configurator m_configurator;    // SD card configurator
    IdmaSlot idmaSlot;

    const uint32_t DataTimeOut = 0xFFFFFFFF;    // Data transferring timeoutх (in bus cycles)

    // Static flag masks to reset in ICR:
    const uint32_t StaticDataFlags = 0x18000F3A;    // SDMMC_STATIC_DATA_FLAGS
    const uint32_t StaticFlags = 0x1FE00FFF;        // SDMMC_STATIC_FLAGS
    SDMMC_TypeDef* const sdmmc;

public:
    /**
     * @brief Constructor
     * @param [in] sdmmc Periphery pointer
     */
    SdCard(SDMMC_TypeDef* sdmmc);

    /**
     * @brief SDMMC initialization
     * @param [in] Config module configuration
     */
    Result init(const Config& Config);

    /**
     * @brief Data blocks reading
     * @param [in] dest Address for data saving
     * @param [in] blockAddr Number of first block for reading
     * @param [in] blocksNumber Blocks number
     * @param [in] timeoutMs Reading timeout in milliseconds
     */
    Result readBlocks(uint8_t* dest, uint32_t blockAddr, uint32_t blocksNumber, uint32_t timeoutMs);

    /**
     * @brief Data blocks writing
     * @param [in] source Data address
     * @param [in] blockAddr Number of first block for writing
     * @param [in] blocksNumber Blocks number
     * @param [in] timeoutMs Writing timeout in milliseconds
     */
    Result writeBlocks(const uint8_t* source, uint32_t blockAddr, uint32_t blocksNumber, uint32_t timeoutMs);

    /**
     * @brief Data blocks erasing
     * @param [in] blockAddr Number of first block for erasing
     * @param [in] blocksNumber Blocks number
     */
    Result eraseBlocks(uint32_t blockAddr, uint32_t blocksNumber);

    /**
     * @brief Get number of blocks
     */
    uint32_t getBlocksNumber();

    /**
     * @brief Get size of one block
     */
    uint16_t getBlockLen();

    /**
     * @brief Чтение блоков данных с использование внутреннего IDMA.
     * @param [in] dest Указатель на данные.
     * @param [in] blockAddr Адрес блока, с которого начинать чтение.
     * @param [in] blocksNumber Количество блоков для чтения.
     */
    Result readViaIDMA(uint8_t* dest, uint32_t blockAddr, uint32_t blocksNumber);

    /**
     * @brief Запись блоков данных с использование внутреннего IDMA.
     * @param [in] source Указатель на данные.
     * @param [in] blockAddr Адрес блока, с которого начинать запись.
     * @param [in] blocksNumber Количество блоков для записи.
     */
    Result writeViaIDMA(const uint8_t* source, uint32_t blockAddr, uint32_t blocksNumber);

    /**
     * @brief Получить статус модуля встроенного потока IDMA
     */
    IdmaSlot::IdmaState getIdmaState();

    /**
     * @brief Сброс слота IDMA
     */
    void resetIdma();
};


}   // namespace driver