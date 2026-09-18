#include "Sdmmc.h"
#include "Loger.h"
#include "driver/Dwt.h"

namespace driver {

Log& operator<<(Log& out, const Result& result)
{
    switch (result)
    {
    case Result::Ok:                    out << "Ok"; break;
    case Result::ArgError:              out << "ArgError"; break;
    case Result::ClockError:            out << "ClockError"; break;
    case Result::ProgramTimeout:        out << "ProgramTimeout"; break;
    case Result::SysCmdTimeout:         out << "SysCmdTimeout"; break;
    case Result::SysDataTimeout:        out << "SysDataTimeout"; break;
    case Result::SysCmdCrcFail:         out << "SysCmdCrcFail"; break;
    case Result::SysDataCrcFail:        out << "SysDataCrcFail"; break;
    case Result::SysFifoFail:           out << "SysFifoFail"; break;
    case Result::CardStatusError:       out << "CardStatusError"; break;
    case Result::StatusCardLocked:      out << "StatusCardLocked"; break;
    case Result::StatusOutOfRange:      out << "StatusOutOfRange"; break;
    case Result::StatusAddrError:       out << "StatusAddrError"; break;
    case Result::StatusBlockLenErr:     out << "StatusBlockLenErr"; break;
    case Result::StatusEraseError:      out << "StatusEraseError"; break;
    case Result::StatusWriteProtect:    out << "StatusWriteProtect"; break;
    case Result::StatusUnlockFail:      out << "StatusUnlockFail"; break;
    case Result::StatusCrcError:        out << "StatusCrcError"; break;
    case Result::StatusCmdError:        out << "StatusCmdError"; break;
    case Result::StatusEccError:        out << "StatusEccError"; break;
    case Result::StatusCtrlError:       out << "StatusCtrlError"; break;
    case Result::StatusOtherError:      out << "StatusOtherError"; break;
    case Result::StatusCsdError:        out << "StatusCsdError"; break;
    case Result::StatusEraseSkip:       out << "StatusEraseSkip"; break;
    case Result::StatusIdentError:      out << "StatusIdentError"; break;
    case Result::PowerOnFail:           out << "PowerOnFail"; break;
    case Result::CardTypeFail:          out << "CardTypeFail"; break;
    case Result::OptionNotSupported:    out << "OptionNotSupported"; break;
    case Result::IdmaBusy:              out << "IdmaBusy"; break;
    case Result::CardNotInitialised:    out << "CardNotInitialised"; break;
    case Result::OtherAnyError:         out << "OtherAnyError"; break;
    }
    return out;
}

SdCard::SdCard (SDMMC_TypeDef* _sdmmc) : 
    sdmmc (_sdmmc), 
    m_cmdSender(_sdmmc), 
    m_configurator(_sdmmc, m_cmdSender),
    idmaSlot(*this, m_cmdSender)
{}

IdmaSlot::IdmaState SdCard::getIdmaState()
{
    return idmaSlot.getState();
}

Result SdCard::init(const Config& userConfig)
{
    return m_configurator.init(userConfig);
}

Result SdCard::readBlocks (uint8_t* dest, uint32_t blockAddr, uint32_t blocksNumber, uint32_t timeoutMs)
{
    const auto& cardInfo = m_configurator.getCardInfo();
    if (!dest || (blocksNumber == 0) || ((blockAddr + blocksNumber) > cardInfo.blocksNumber))
        return Result::ArgError;

    if (!cardInfo.initialised)
        return Result::CardNotInitialised;
    
    DataConfig dataConfig
    {
        .timeOut = DataTimeOut,
        .dataLength = blocksNumber * cardInfo.blockSize,
        .blockSize = BlockSize::Size_512B,
        .direction = TransferDir::ToHost,
        .mode = TransferMode::BlockMode,
        .dpsm = false   // Don't start the transferring without command
    };
    WRITE_REG (sdmmc->DCTRL, 0);
    m_configurator.dataTransferConfig(dataConfig);
    // For small volume cards, the addressing is by-byte:
    if (cardInfo.capacityType == CapacityType::StandartCapacity)
        blockAddr *= 512;

    // Data transfer command:
    SET_BIT (sdmmc->CMD, SDMMC_CMD_CMDTRANS);
    Command cmd
    {
        .Argument = blockAddr,
        .CmdIndex = (blocksNumber > 1) ? Commands::ReadMultiBlock : Commands::ReadSingleBlock,
        .Response = ResponseSize::ShortResponse,
        .WaitForInterrupt = false,
        .CPSM = true
    };

    Result result;
    if (!m_cmdSender.sendCommand(cmd, ResponseType::R1, result))
    {
        SET_BIT (sdmmc->ICR, StaticFlags);
        return result;
    }

    // Finish flags: FIFO overrun, CRC error, Timeout, End  Of Transfer:
    uint32_t finishState = SDMMC_STA_RXOVERR | SDMMC_STA_DCRCFAIL | SDMMC_STA_DTIMEOUT | SDMMC_STA_DATAEND;
    auto dest32 = reinterpret_cast<uint32_t*>(dest);
    DwtTimer::TimeoutData param = driver::DwtTimer::getInstance().timeoutInit(timeoutMs);
    while(!READ_BIT(sdmmc->STA, finishState))
    {
        // FIFO half-full (8*32-bit words)
        if (READ_BIT(sdmmc->STA, SDMMC_STA_RXFIFOHF))
        {
            for (int i = 0; i < 8; i++)
            {
                *dest32 = sdmmc->FIFO;
                dest32++;
            }
        }
        if (DwtTimer::getInstance().checkTimeout(param))
            return Result::ProgramTimeout;
    }
    CLEAR_BIT (sdmmc->CMD, SDMMC_CMD_CMDTRANS);  // Data transfer command reset

    // Stop command for multiblock reading (if completed successfully):
    if (READ_BIT (sdmmc->STA, SDMMC_STA_DATAEND) && (blocksNumber > 1))
    {
        cmd.Argument = 0;
        cmd.CmdIndex = Commands::StopTransfer;
        cmd.Response = ResponseSize::ShortResponse;
        cmd.WaitForInterrupt = false;
        cmd.CPSM = true;
        if (!m_cmdSender.sendCommand(cmd, ResponseType::R1, result))
        {
            SET_BIT (sdmmc->ICR, StaticFlags);
            return result;
        }
    }
    
    // Check result:
    if (READ_BIT (sdmmc->STA, SDMMC_STA_RXOVERR))
    {
        SET_BIT (sdmmc->ICR, StaticFlags);
        return Result::SysFifoFail;
    }
    else if (READ_BIT (sdmmc->STA, SDMMC_STA_DCRCFAIL))
    {
        SET_BIT (sdmmc->ICR, StaticFlags);
        return Result::SysDataCrcFail;
    }
    else if (READ_BIT (sdmmc->STA, SDMMC_STA_DTIMEOUT))
    {
        SET_BIT (sdmmc->ICR, StaticFlags);
        return Result::SysDataTimeout;
    }
    SET_BIT (sdmmc->ICR, StaticDataFlags);
    return Result::Ok;
}

Result SdCard::writeBlocks (const uint8_t* source, uint32_t blockAddr, uint32_t blocksNumber, uint32_t timeoutMs)
{
    const auto& cardInfo = m_configurator.getCardInfo();
    if (!source || (blocksNumber == 0) || ((blockAddr + blocksNumber) > cardInfo.blocksNumber))
        return Result::ArgError;

    if (!cardInfo.initialised)
        return Result::CardNotInitialised;

    DataConfig dataConfig
    {
        .timeOut = DataTimeOut,
        .dataLength = blocksNumber * cardInfo.blockSize,
        .blockSize = BlockSize::Size_512B,
        .direction = TransferDir::ToCard,
        .mode = TransferMode::BlockMode,
        .dpsm = false   // Don't start the transferring without command
    };
    WRITE_REG (sdmmc->DCTRL, 0);
    m_configurator.dataTransferConfig(dataConfig);
    // For small volume cards, the addressing is by-byte:
    if (cardInfo.capacityType ==  CapacityType::StandartCapacity)
        blockAddr *= 512;

    // Data transfer command:
    SET_BIT (sdmmc->CMD, SDMMC_CMD_CMDTRANS);
    Command cmd
    {
        .Argument = blockAddr,
        .CmdIndex = (blocksNumber > 1) ? Commands::WriteMultiBlock : Commands::WriteSingleBlock,
        .Response = ResponseSize::ShortResponse,
        .WaitForInterrupt = false,
        .CPSM = true
    };
    Result result;
    if (!m_cmdSender.sendCommand(cmd, ResponseType::R1, result))
    {
        SET_BIT (sdmmc->ICR, StaticFlags);
        return result;
    }

    // Finish flags: FIFO Underrun, CRC error, Timeout, End  Of Transfer:
    uint32_t finishState = SDMMC_STA_TXUNDERR | SDMMC_STA_DCRCFAIL | SDMMC_STA_DTIMEOUT | SDMMC_STA_DATAEND;
    auto source32 = reinterpret_cast<const uint32_t*>(source);
    DwtTimer::TimeoutData param = driver::DwtTimer::getInstance().timeoutInit(timeoutMs);
    while(!READ_BIT (sdmmc->STA, finishState))
    {
        // FIFO half-empty (at least 8*32-bit words can be writed):
        if (READ_BIT (sdmmc->STA, SDMMC_STA_TXFIFOHE))
        {
            for (int i = 0; i < 8; i++)
            {
                sdmmc->FIFO = *source32;
                source32++;
            }
        }
        if (DwtTimer::getInstance().checkTimeout(param))
            return Result::ProgramTimeout;
    }

    CLEAR_BIT (sdmmc->CMD, SDMMC_CMD_CMDTRANS);  // Data transfer command reset

    // Stop command for multiblock writing (if completed successfully):
    if (READ_BIT (sdmmc->STA, SDMMC_STA_DATAEND) && (blocksNumber > 1))
    {
        cmd.Argument = 0,
        cmd.CmdIndex = Commands::StopTransfer,
        cmd.Response = ResponseSize::ShortResponse,
        cmd.WaitForInterrupt = false;
        cmd.CPSM = true;
        if (!m_cmdSender.sendCommand(cmd, ResponseType::R1, result))
        {
            SET_BIT (sdmmc->ICR, StaticFlags);
            return result;
        }
    }

    // Check result:
    if (READ_BIT (sdmmc->STA, SDMMC_STA_TXUNDERR))
    {
        SET_BIT (sdmmc->ICR, StaticFlags);
        return Result::SysFifoFail;
    }
    else if (READ_BIT (sdmmc->STA, SDMMC_STA_DCRCFAIL))
    {
        SET_BIT (sdmmc->ICR, StaticFlags);
        return Result::SysDataCrcFail;
    }
    else if (READ_BIT (sdmmc->STA, SDMMC_STA_DTIMEOUT))
    {
        SET_BIT (sdmmc->ICR, StaticFlags);
        return Result::SysDataTimeout;
    }
    SET_BIT (sdmmc->ICR, StaticDataFlags);
    return Result::Ok;
}

Result SdCard::readViaIDMA (uint8_t* dest, uint32_t blockAddr, uint32_t blocksNumber)
{
    return idmaSlot.read(dest, blockAddr, blocksNumber);    // TODO: initialization check inside!
}

Result SdCard::writeViaIDMA (const uint8_t* source, uint32_t blockAddr, uint32_t blocksNumber)
{
    return idmaSlot.write (source, blockAddr, blocksNumber);    // TODO: initialization check inside!
}

Result SdCard::eraseBlocks (uint32_t blockAddr, uint32_t blocksNumber)
{
    const auto& cardInfo = m_configurator.getCardInfo();
    if (blocksNumber == 0 || ((blockAddr + blocksNumber) > cardInfo.blocksNumber))
        return Result::ArgError;

    if (!cardInfo.initialised)
        return Result::CardNotInitialised;

    if (!cardInfo.erasingSupport)
        return Result::OptionNotSupported;

    Result result;

    // Set first erased block address:
    Command cmd;
    cmd.Argument = blockAddr;
    cmd.CmdIndex = Commands::EraseBlockStart;
    cmd.Response = ResponseSize::ShortResponse;
    cmd.CPSM = true;
    if (!m_cmdSender.sendCommand(cmd, ResponseType::R1, result))
    {
        SET_BIT (sdmmc->ICR, StaticFlags);
        return result;
    }

    // Set last erased block address:
    cmd.Argument = blockAddr + (blocksNumber - 1);
    cmd.CmdIndex = Commands::EraseBlockEnd;
    cmd.Response = ResponseSize::ShortResponse;
    cmd.WaitForInterrupt = false;
    cmd.CPSM = true;
    if (!m_cmdSender.sendCommand(cmd, ResponseType::R1, result))
    {
        SET_BIT (sdmmc->ICR, StaticFlags);
        return result;
    }

    // Erasing command
    cmd.Argument = 1;   // Trim Erase
    cmd.CmdIndex = Commands::Erase;
    cmd.Response = ResponseSize::ShortResponse;
    cmd.WaitForInterrupt = false;
    cmd.CPSM = true;
    if (!m_cmdSender.sendCommand(cmd, ResponseType::R1, result))
    {
        SET_BIT (sdmmc->ICR, StaticFlags);
        return result;
    }
    // TODO: If this method is going to be used, add the waiting for completion and timeout here!
    return Result::Ok;
}

uint32_t SdCard::getBlocksNumber()
{
    return m_configurator.getCardInfo().blocksNumber;
}

uint16_t SdCard::getBlockLen()
{
    return m_configurator.getCardInfo().blockSize;
}

void SdCard::resetIdma ()
{
    idmaSlot.reset ();
}


}   // namespace driver
