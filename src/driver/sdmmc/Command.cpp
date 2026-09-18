#include "Command.h"
#include "driver/Dwt.h"
#include "Loger.h"

namespace driver {
namespace sdmmc {

CommandSender::CommandSender(SDMMC_TypeDef* sdmmc) : m_sdmmc(sdmmc)
{}

bool CommandSender::waitCmdDone(uint32_t finishStateMsk)
{
    uint32_t cpsmAct = SDMMC_STA_CPSMACT;   // Command path state machine (CPSM) active flag

    if (finishStateMsk == 0)    // If checking only command sending
    {
        finishStateMsk = SDMMC_STA_CMDSENT;     // Checking conditions is only "Command Sent"
        cpsmAct = 0;                            // CPSM not tracking
    }

    driver::DwtTimer::TimeoutData param = driver::DwtTimer::getInstance().timeoutInit(m_CmdTimeOutMs);
    while ((!READ_BIT(m_sdmmc->STA, finishStateMsk)) || (READ_BIT(m_sdmmc->STA, cpsmAct)))
    {
        if (driver::DwtTimer::getInstance().checkTimeout(param))
            return false;
    }
    return true;
}

bool CommandSender::sendCommand(const Command& cmd, ResponseType respType, Result& result)
{
    m_sdmmc->ARG = cmd.Argument;
    const uint32_t CmdClrMsk = 0x11f3f;
    uint32_t tmpreg = static_cast<uint8_t>(cmd.CmdIndex);
    tmpreg |= (static_cast<uint8_t>(cmd.Response)) << SDMMC_CMD_WAITRESP_Pos;
    tmpreg |= cmd.WaitForInterrupt << SDMMC_CMD_WAITINT_Pos;
    tmpreg |= cmd.CPSM << SDMMC_CMD_CPSMEN_Pos;
    MODIFY_REG(m_sdmmc->CMD, CmdClrMsk, tmpreg);
    // Checking the execution result for different response types:
    switch (respType)
    {
    case ResponseType::None :
        result = checkCmdSendError();
        break;
    case ResponseType::R1 :
        result = checkResp1Error();
        break;
    case ResponseType::R2 :
    case ResponseType::R6 :
        result = checkResp2Error();
        break;
    case ResponseType::R3 :
        result = checkResp3Error();
        break;
    case ResponseType::R7 :
        result = checkResp7Error();
        break;
    default: result = Result::ArgError;
    }
    return (result == Result::Ok);
}

void CommandSender::getResponse(ResponseSize type, uint32_t* respReg)
{
    if (!respReg || type == ResponseSize::NoResponse)
        return;

    uint8_t regCount = 0;
    if (type == ResponseSize::ShortResponse)
        regCount = 1;
    else
        regCount = 4;

    volatile uint32_t* tempResp = &(m_sdmmc->RESP1);
    for (int i = 0; i < regCount; i++)
    {
        respReg[i] = *tempResp;
        tempResp++;
    }
}

Result CommandSender::checkCmdSendError()
{
    if (!waitCmdDone(0))
        return Result::ProgramTimeout;

    SET_BIT(m_sdmmc->ICR, m_StaticCmdFlags);   // Static flags reset
    return Result::Ok;
}

Result CommandSender::checkResp1Error()
{
    if (!waitCmdDone(m_R1endFlags))
        return Result::ProgramTimeout;

    if (READ_BIT(m_sdmmc->STA, SDMMC_STA_CTIMEOUT))
    {
        SET_BIT(m_sdmmc->ICR, SDMMC_STA_CTIMEOUT);
        return Result::SysCmdTimeout;
    }
    else if (READ_BIT(m_sdmmc->STA, SDMMC_STA_CCRCFAIL))
    {
        SET_BIT (m_sdmmc->ICR, SDMMC_STA_CCRCFAIL);
        return Result::SysCmdCrcFail;
    }

    SET_BIT(m_sdmmc->ICR, m_StaticCmdFlags);
    uint32_t response = 0;
    getResponse(ResponseSize::ShortResponse, &response);

    // Response OK:
    const uint32_t R1ErrorBits = 0xFDF98008;  // Error bits in the card state
    if ((response & R1ErrorBits) == 0)
        return Result::Ok;
    // Get response error:
    else if (response & (1 << 31))
        return Result::StatusOutOfRange;
    else if (response & (1 << 30))
        return Result::StatusAddrError;
    else if (response & (1 << 29))
        return Result::StatusBlockLenErr;
    else if ((response & (1 << 27)) || (response & (1 << 28)))
        return Result::StatusEraseError;
    else if (response & (1 << 26))
        return Result::StatusWriteProtect;
    else if (response & (1 << 24))
        return Result::StatusUnlockFail;
    else if (response & (1 << 23))
        return Result::StatusCrcError;
    else if (response & (1 << 22))
        return Result::StatusCmdError;
    else if (response & (1 << 21))
        return Result::StatusEccError;
    else if (response & (1 << 20))
        return Result::StatusCtrlError;
    else if (response & (1 << 19))
        return Result::StatusOtherError;
    else if (response & (1 << 16))
        return Result::StatusCsdError;
    else if (response & (1 << 15))
        return Result::StatusEraseSkip;
    else if (response & (1 << 3))
        return Result::StatusIdentError;
    else
        return Result::CardStatusError;   
}

Result CommandSender::checkResp2Error()
{
    if (!waitCmdDone(m_R237endFlags))
        return Result::ProgramTimeout;

    if (READ_BIT(m_sdmmc->STA, SDMMC_STA_CTIMEOUT))
    {
        SET_BIT(m_sdmmc->ICR, SDMMC_STA_CTIMEOUT);
        return Result::SysCmdTimeout;
    }
    else if (READ_BIT(m_sdmmc->STA, SDMMC_STA_CCRCFAIL))
    {
        SET_BIT(m_sdmmc->ICR, SDMMC_STA_CCRCFAIL);
        return Result::SysCmdCrcFail;
    }
    
    SET_BIT(m_sdmmc->ICR, m_StaticCmdFlags);
    return Result::Ok;
}

Result CommandSender::checkResp3Error()
{
    if (!waitCmdDone(m_R237endFlags))
        return Result::ProgramTimeout;

    if (READ_BIT(m_sdmmc->STA, SDMMC_STA_CTIMEOUT))
    {
        SET_BIT(m_sdmmc->ICR, SDMMC_STA_CTIMEOUT);
        return Result::SysCmdTimeout;
    }
    
    SET_BIT(m_sdmmc->ICR, m_StaticCmdFlags);
    return Result::Ok;
}

Result CommandSender::checkResp7Error()
{
    if (!waitCmdDone(m_R237endFlags))
        return Result::ProgramTimeout;

    if (READ_BIT(m_sdmmc->STA, SDMMC_STA_CTIMEOUT))
    {
        SET_BIT(m_sdmmc->ICR, SDMMC_STA_CTIMEOUT);
        return Result::SysCmdTimeout;
    }
    else if (READ_BIT(m_sdmmc->STA, SDMMC_STA_CCRCFAIL))
    {
        SET_BIT (m_sdmmc->ICR, SDMMC_STA_CCRCFAIL);
        return Result::SysCmdCrcFail;
    }

    if (READ_BIT(m_sdmmc->STA, SDMMC_STA_CMDREND))
        SET_BIT(m_sdmmc->ICR, SDMMC_STA_CMDREND);

    return Result::Ok;
}

}   // namespace sdmmc
}   // namespace driver
