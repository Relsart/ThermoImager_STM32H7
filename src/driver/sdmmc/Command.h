#pragma once
#include <stdint.h>
#include "stm32h7xx.h"
#include "DataTypes.h"

namespace driver {
namespace sdmmc {

/**
 * @brief SD commands
 */
enum class Commands
{
    GoIdleState =       0,  // Go to Ile state (card reset)
    SendOpCond =        1,  // OCR (Operating Conditions Register) request
    GetCid =            2,  // CID (Chip IDentification) request 
    SetRelativeAddr =   3,  // Setting the relative card address RCA
    SetBusWidth =       6,  // Setting the width of data bus (1 or 4 wires)
    SelectCard =        7,  // Select card by its RCA address
    GetInterfaceCond =  8,  // Request the extended CSD
    GetCSD =            9,  // Request card specific data (CSD)
    StopTransfer =      12, // Stop data transferring
    GetStatus =         13, // Card state request
    SetBlockLen =       16, // Setting the length of data block
    ReadSingleBlock =   17, // Reading one data block
    ReadMultiBlock =    18, // Read several data blocks
    WriteSingleBlock =  24, // Write one data block
    WriteMultiBlock =   25, // Write several data blocks
    EraseBlockStart =   32, // Set the first block address for erasing
    EraseBlockEnd =     33, // Set the last block address for erasing
    Erase =             38, // Erasing data blocks group
    GetSCR =            51, // For SD cards: request SCR (SD Card Configuration register)
    ApplicationCmd =    55, // Specifies next command as application (not a standard one)
    SD_SendOpCond=      41, // For SD cards: request OCR (Operating Conditions Register)
};

/**
 * @brief Command struct
 */
struct Command
{
    uint32_t Argument = 0;
    Commands CmdIndex;
    ResponseSize Response;
    bool WaitForInterrupt = false;
    bool CPSM;
};

/**
 * @brief The Command sender
 */
class CommandSender
{
public:
    /**
     * @brief Constructor
     * @param [in] sdmmc Periphery device address
     */
    CommandSender(SDMMC_TypeDef* sdmmc);

    /**
     * @brief Command to the card
     * @param [in] cmd Command
     * @param [in] respType Expected response type
     * @param [out] respType Command execution result
     * @return result == Ok
     */
    bool sendCommand(const Command& cmd, ResponseType respType, Result& result);

    /**
     * @brief Receive the response from card
     * @param [in] type Type of response
     * @param [in] respReg Address for reading the response
     */
    void getResponse(ResponseSize type, uint32_t* respReg);

private:
    SDMMC_TypeDef* const m_sdmmc;                   // Periphery module
    const uint32_t m_CmdTimeOutMs = 1000;           // Command executing timeout (in milliseconds)
    const uint32_t m_StaticCmdFlags = 0x002000C5;   // SDMMC_STATIC_CMD_FLAGS

    const uint32_t m_R1endFlags = SDMMC_STA_CCRCFAIL | SDMMC_STA_CMDREND | SDMMC_STA_CTIMEOUT | SDMMC_STA_BUSYD0END;    // Command finish flags mask for response type 1
    const uint32_t m_R237endFlags = SDMMC_STA_CCRCFAIL | SDMMC_STA_CMDREND | SDMMC_STA_CTIMEOUT;    // Command finish flags mask for responses type 2, 3 or 7

    /**
     * @brief Waiting for the completion of the command execution
     * @param [in] finishstate Flags for the completion conditions. If = 0 then only command sending is expected
     * @return Command execution result: True = executed (correctly or failed), False = timeout 
     */
    bool waitCmdDone(uint32_t finishstate);

    /**
     * @brief Checking the execution of commands for different responses types.
     */
    Result checkCmdSendError();
    Result checkResp1Error();
    Result checkResp2Error();
    Result checkResp3Error();
    Result checkResp7Error();
};

}   // namespace sdmmc
}   // namespace driver
