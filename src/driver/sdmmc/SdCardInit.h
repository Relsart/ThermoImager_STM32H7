#pragma once
#include <stdint.h>
#include "stm32h7xx.h"
#include "DataTypes.h"
#include "Command.h"

namespace driver {
namespace sdmmc {

/**
 * @brief Class implements the SD card initialization procedure
 */
class Configurator
{
public:
    /**
     * @brief Constructor
     * @param [in] sdmmc Periphery device address
     */
    Configurator(SDMMC_TypeDef* sdmmc, CommandSender& cmdSender);

    /**
     * @brief SDMMC SD Card initialization
     * @note For MMC chips the init procedure is different
     * @param [in] Config Main configurations
     */
    Result init(const Config& Config);

    /**
     * @brief Setting the data transfering configuration
     * @param [in] config settings
     */
    void dataTransferConfig(const DataConfig& config);

    const CardInfo& getCardInfo();

private:
    

    const uint32_t m_StaticFlags = 0x1FE00FFF;      // SDMMC_STATIC_FLAGS
    const uint32_t m_StaticDataFlags = 0x18000F3A;  // SDMMC_STATIC_DATA_FLAGS

    SDMMC_TypeDef* const m_sdmmc;   // Periphery module
    CommandSender& m_cmdSender;     // Link to the Command sender instance
    uint32_t m_rca = 0;             // Relative Card Address RCA
    uint32_t m_deviceSize = 0;      // Device size (?)
    SdState m_cardState;
    CardInfo m_cardInfo;    // Card properties and specifications

    /**
     * @brief Applying configs for bus width, clock, edge to SDMMC module
     */
    void setBusClockControl(Config& config);

    /**
     * @brief Setting the data block size
     * @param [in] blockSize value in bytes
     * @param [out] result Result code
     * @return result == Ok
     */
    bool setBlockSize(BlockSize blockSize, Result& result);

    /**
     * @brief Go to Idle state command
     * @param [out] result Result code
     * @return result == Ok
     */
    bool goToIdleState(Result& result);

    /**
     * @brief Switching to application commands
     * @param [in] arg Argument
     * @param [out] result Result code
     * @return result == Ok
     */
    bool toApplicationCmd(uint32_t arg, Result& result);

    /**
     * @brief Get SD card version (V1.0 or V2.0)
     * @param [out] version value
     * @param [out] result Result code
     * @return result == Ok
     */
    bool readCardVers(CardVersion& version, Result& result);

    /**
     * @brief Get SD card capacity type (standard or high/extended)
     * @param [out] type Card capacity type
     * @param [out] result Result code
     * @return result == Ok
     */
    bool readCapacityType(CapacityType& type, Result& result);

    /**
     * @brief Get CID (Chip IDentificator)
     * @param [out] cid value
     * @param [out] result Result code
     * @return result == Ok
     */
    bool readCid(uint32_t* cid, Result& result);
    
    /**
     * @brief Get RCA (Relative Card Address)
     * @param [out] rca value
     * @param [out] result Result code
     * @return result == Ok
     */
    bool readRca(uint32_t* rca, Result& result);

    /**
     * @brief Get CSD (Card Specific Data)
     * @details Get info about blocks: size, number etc
     * @param [in] rca address
     * @param [in] cardType Card Capacity
     * @param [out] result Result code
     * @return result == Ok
     */
    bool readCSD(uint32_t rca, CapacityType cardType, Result& result);

    /**
     * @brief Get SCR (SD Card Configuration register)
     * @param [out] rca value
     * @param [out] result Result code
     * @return result == Ok
     */
    bool readSCR(SCR& scrStruct, Result& result);

    /**
     * @brief Select card by its RCA address
     * @param [in] rca address
     * @param [out] result Result code
     * @return result == Ok
     */
    bool cardSelect(uint32_t rca, Result& result);

    /**
     * @brief Request the 512-bits card status structure (extended ACMD_13 command)
     * @details Get state and speed specifications from it
     * @param [out] result Result code
     * @return result == Ok
     */
    bool readCardExtStatus(Result& result);

    /**
     * @brief Get cards state
     * @param [in] rca address
     * @return state value
     */
    SdCardState readCardState(uint32_t rca);

    /**
     * @brief Set bus width
     * @param [in] busWide SD cards support only 1- and 4-bit bus
     * @param [out] result Result code
     * @return result == Ok
     */
    bool setBusWidth(BusWide busWide, Result& result);
};

}   // namespace sdmmc
}   // namespace driver
