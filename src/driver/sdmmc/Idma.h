#pragma once
#include <stdint.h>
#include "DataTypes.h"
#include "stm32h7xx.h"
#include "driver/nvic/Nvic.h"
#include "Command.h"

namespace driver {

class SdCard; 

namespace sdmmc {

/**
 * @brief Internal DMA (IDMA) slot
 */
class IdmaSlot : public SlotInterface <Irq>
{
public:
    /**
     * @brief IDMA states
     */
    enum class IdmaState
    {
        Idle,           // Is free
        DataSending,    // Data transmitting in process
        DataReading,    // Data receiving in process
        FinishOk,       // All transaction are successfully accomplished
        Error,          // Transaction error state
    };

    /**
     * @brief Constructor
     * @param Link to maintainer
     */
    IdmaSlot(SdCard& maintainer, CommandSender& cmdSender);

    /**
     * @brief Data blocks writing via Internal DMA stream
     * @param [in] source data
     * @param [in] blockAddr block address
     * @param [in] blocksNumber number of blocks
     */
    Result write(const uint8_t* source, uint32_t blockAddr, uint32_t blocksNumber);

    /**
     * @brief Data blocks reading via Internal DMA stream
     * @param [in] dest input buffer
     * @param [in] blockAddr block address
     * @param [in] blocksNumber number of blocks
     */
    Result read(uint8_t* dest, uint32_t blockAddr, uint32_t blocksNumber);

    /**
     * @brief IDMA module reset
     */
    void reset();

    IdmaState getState();

private:
    CommandSender& m_cmdSender;     // Link to the Command sender instance
    IdmaState state;
    static constexpr uint16_t sdBlockSize = 512;
    static constexpr uint32_t irqSendMask = SDMMC_MASK_DCRCFAILIE | SDMMC_MASK_DTIMEOUTIE | SDMMC_MASK_TXUNDERRIE | SDMMC_MASK_DATAENDIE | SDMMC_MASK_IDMABTCIE;
    static constexpr uint32_t irqReadMask = SDMMC_MASK_DCRCFAILIE | SDMMC_MASK_DTIMEOUTIE | SDMMC_MASK_RXOVERRIE | SDMMC_MASK_DATAENDIE | SDMMC_MASK_IDMABTCIE;

    SDMMC_TypeDef* const sdmmc; // Periphery SDMMC address
    SdCard& m_maintainer;       // Link to maintainer class
    uint32_t remainBlocks = 0;  // Blocks number remained to transfer (multiblock mode)
    uint32_t currBlockAddr = 0; // Current block address (multiblock mode)
    uint8_t* currData;          // Pointer to current address of writing data
    uint32_t timeout = 1000;

    enum class Action { Read, Write };

    // Temporary method, use the TRANSFERBLOCK!
    void writeBlock(const uint8_t* source, uint32_t blockAddr);
    // Temporary method, use the TRANSFERBLOCK!
    void readBlock(uint8_t* dest, uint32_t blockAddr);

    void transferBlock(uint8_t* data, uint32_t blockAddr, Action act);

    /**
     * @brief IDMA interruptions handler
     */
    void run (Irq, uint32_t) override;
};

    }   // namespace sdmmc
    }   // namespace driver
