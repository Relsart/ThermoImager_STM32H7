#include "Idma.h"
#include "Sdmmc.h"
#include "driver/nvic/NvicManager.h"
#include "driver/SysTimer.h"

namespace driver {
namespace sdmmc {

IdmaSlot::IdmaSlot(SdCard& maintainer, CommandSender& cmdSender) : 
    m_maintainer(maintainer),
    m_cmdSender(cmdSender),
    sdmmc(maintainer.sdmmc)
{
    Nvic* nvic = NvicManager::getInstance().getSdmmcNvic(sdmmc);
    if (nvic)
    {
        IRQn_Type irq;
        if (NvicManager::getInstance().getIrqNumber(reinterpret_cast<uint32_t>(sdmmc), irq))
            nvic->init (irq, 3, *this);
    }
}

void IdmaSlot::run (Irq, uint32_t)
{
    // Successfully finished transaction interruption:
    if (READ_BIT (sdmmc->STA, SDMMC_STA_DATAEND | SDMMC_STA_IDMABTC) != 0)
    {
        SET_BIT (sdmmc->ICR, SDMMC_ICR_DATAENDC | SDMMC_ICR_IDMABTCC); // Сброс флагов прерываний
        remainBlocks--;
        if (remainBlocks == 0)  // All blocks have tranmitted
        {
            currBlockAddr = 0;
            currData = nullptr;
            CLEAR_BIT (sdmmc->MASK, irqSendMask);           // Disable interruptions
            CLEAR_BIT (sdmmc->CMD, SDMMC_CMD_CMDTRANS);     // Reset transfer data comand
            CLEAR_BIT (sdmmc->IDMACTRL, SDMMC_IDMA_IDMAEN); // IDMA Disable
            state = IdmaState::FinishOk;
        }
        else    // Next data block transmission
        {
            currBlockAddr++;
            currData += sdBlockSize;
            Action act;

            if (state == IdmaState::DataSending)
            {
                //writeBlock (currData, currBlockAddr);
                act = Action::Write;
            }
            else if (state == IdmaState::DataReading)
            {
                //readBlock (currData, currBlockAddr);
                act = Action::Read;
            }
            transferBlock (currData, currBlockAddr, act);
        }
    }

    // Failed transaction interruption:
    if (READ_BIT (sdmmc->STA, SDMMC_STA_DATAEND | SDMMC_STA_IDMATE) != 0)
    {
        SET_BIT (sdmmc->ICR, SDMMC_ICR_DCRCFAILC | SDMMC_MASK_DTIMEOUTIE | SDMMC_MASK_TXUNDERRIE);
        state = IdmaState::Error;
        Log (lmSDMMC, LogLevel::Error) << "SDMMC IDMA transfer error!";
    }
}

void IdmaSlot::readBlock (uint8_t* dest, uint32_t blockAddr)
{
    Result result;
    sdmmc->DCTRL = 0;
    const auto& cardInfo = m_maintainer.m_configurator.getCardInfo();
    if (cardInfo.capacityType == CapacityType::StandartCapacity)
        blockAddr *= 512;   // Для карт малого объема адресация побайтовая:

    // Конфигурация конечного автомата данных:
    DataConfig dataConfig
    {
        .timeOut = m_maintainer.DataTimeOut,
        .dataLength = cardInfo.blockSize,
        .blockSize = BlockSize::Size_512B,
        .direction = TransferDir::ToHost,
        .mode = TransferMode::BlockMode,
        .dpsm = false   // Не начинать передачу без команды передачи!
    };
    m_maintainer.m_configurator.dataTransferConfig(dataConfig);

    SET_BIT (m_maintainer.sdmmc->CMD, SDMMC_CMD_CMDTRANS);        // Команда на передачу данных
    sdmmc->IDMABASE0 = reinterpret_cast<uint32_t> (dest); // Адрес данных для IDMA
    SET_BIT (sdmmc->IDMACTRL, SDMMC_IDMA_IDMAEN);           // Включить IDMA

    Command cmd // Команда на запись данных:
    {
        .Argument = blockAddr,
        .CmdIndex = Commands::ReadSingleBlock,
        .Response = ResponseSize::ShortResponse,
        .WaitForInterrupt = false,
        .CPSM = true
    };
    if (!m_cmdSender.sendCommand(cmd, ResponseType::R1, result))
    {
        //SET_BIT(sdmmc->ICR, m_maintainer.StaticFlags);
        Log (lmSDMMC, LogLevel::Error) << "SDMMC IDMA command error!";
        return;
    }
    SET_BIT (sdmmc->MASK, irqReadMask); // Разрешить прерывания по завершению и ошибкам передачи
}

void IdmaSlot::writeBlock (const uint8_t* source, uint32_t blockAddr)
{
    const auto& cardInfo = m_maintainer.m_configurator.getCardInfo();
    sdmmc->DCTRL = 0;
    if (cardInfo.capacityType == CapacityType::StandartCapacity)
        blockAddr *= 512;   // Для карт малого объема адресация побайтовая:

    // Конфигурация конечного автомата данных:
    DataConfig dataConfig
    {
        .timeOut = m_maintainer.DataTimeOut,
        .dataLength = cardInfo.blockSize,
        .blockSize = BlockSize::Size_512B,
        .direction = TransferDir::ToCard,
        .mode = TransferMode::BlockMode,
        .dpsm = false   // Не начинать передачу без команды передачи!
    };
    m_maintainer.m_configurator.dataTransferConfig(dataConfig);
    
    SET_BIT (m_maintainer.sdmmc->CMD, SDMMC_CMD_CMDTRANS);        // Команда на передачу данных
    sdmmc->IDMABASE0 = reinterpret_cast<uint32_t> (source); // Адрес данных для IDMA
    SET_BIT (sdmmc->IDMACTRL, SDMMC_IDMA_IDMAEN);           // Включить IDMA
    
    Command cmd // Команда на запись данных:
    {
        .Argument = blockAddr,
        .CmdIndex = Commands::WriteSingleBlock,
        .Response = ResponseSize::ShortResponse,
        .WaitForInterrupt = false,
        .CPSM = true
    };
    Result result;
    if (!m_cmdSender.sendCommand(cmd, ResponseType::R1, result))
    {
        //SET_BIT(sdmmc->ICR, m_maintainer.StaticFlags);
        Log (lmSDMMC, LogLevel::Error) << "SDMMC IDMA command error!";
        return;
    }
    SET_BIT (sdmmc->MASK, irqSendMask); // Разрешить прерывания по завершению и ошибкам передачи
}

void IdmaSlot::transferBlock (uint8_t* data, uint32_t blockAddr, Action act)
{
    sdmmc->DCTRL = 0;
    const auto& cardInfo = m_maintainer.m_configurator.getCardInfo();
    if (cardInfo.capacityType == CapacityType::StandartCapacity)
        blockAddr *= 512;   // Для карт малого объема адресация побайтовая:

    // Конфигурация конечного автомата данных:
    DataConfig dataConfig
    {
        .timeOut = m_maintainer.DataTimeOut,
        .dataLength = cardInfo.blockSize,
        .blockSize = BlockSize::Size_512B,
        .direction = ((act == Action::Write) ? TransferDir::ToCard : TransferDir::ToHost),
        .mode = TransferMode::BlockMode,
        .dpsm = false   // Не начинать передачу без команды передачи!
    };
    m_maintainer.m_configurator.dataTransferConfig(dataConfig);
    
    SET_BIT (m_maintainer.sdmmc->CMD, SDMMC_CMD_CMDTRANS);        // Команда на передачу данных
    sdmmc->IDMABASE0 = reinterpret_cast<uint32_t> (data);   // Адрес данных для IDMA
    SET_BIT (sdmmc->IDMACTRL, SDMMC_IDMA_IDMAEN);           // Включить IDMA
    
    Command cmd // Команда на запись данных:
    {
        .Argument = blockAddr,
        .CmdIndex = ((act == Action::Write) ? Commands::WriteSingleBlock : Commands::ReadSingleBlock),
        .Response = ResponseSize::ShortResponse,
        .WaitForInterrupt = false,
        .CPSM = true
    };
    Result result;
    if (!m_cmdSender.sendCommand(cmd, ResponseType::R1, result))
    {
        Log (lmSDMMC, LogLevel::Error) << "SDMMC IDMA command error!";
        return;
    }
  
    if (act == Action::Write)
        SET_BIT (sdmmc->MASK, irqSendMask); // Разрешить прерывания по завершению и ошибкам передачи
    else
        SET_BIT (sdmmc->MASK, irqReadMask);
}

Result IdmaSlot::write (const uint8_t* source, uint32_t blockAddr, uint32_t blocksNumber)
{
    const auto& cardInfo = m_maintainer.m_configurator.getCardInfo();
    if (!source || (blocksNumber == 0) || ((blockAddr + blocksNumber) > cardInfo.blocksNumber))
        return Result::ArgError;

    // Wait untill stream IDMA will be free:
    uint64_t startTime = driver::getMsTicks ();
    do
    {   if ((getMsTicks () - startTime) > timeout)
        {
            Log (lmSDMMC, LogLevel::Warn) << "SDMMC IDMA stream is busy!";
            return Result::IdmaBusy;   // Timeout!
        }
    } while ((state != IdmaState::Idle) && (state != IdmaState::FinishOk));
 
    // Start writing:
    state = IdmaState::DataSending;
    remainBlocks = blocksNumber;
    currBlockAddr = blockAddr;
    currData = const_cast<uint8_t*>(source);

    transferBlock (currData, currBlockAddr, Action::Write);
    //writeBlock (currData, currBlockAddr);
    return Result::Ok;
}

Result IdmaSlot::read (uint8_t* dest, uint32_t blockAddr, uint32_t blocksNumber)
{
    const auto& cardInfo = m_maintainer.m_configurator.getCardInfo();
    if (!dest || (blocksNumber == 0) || ((blockAddr + blocksNumber) > cardInfo.blocksNumber))
        return Result::ArgError;

    // Wait untill stream IDMA will be free:
    uint64_t startTime = driver::getMsTicks ();
    do
    {   if ((getMsTicks () - startTime) > timeout)
        {
            Log (lmSDMMC, LogLevel::Warn) << "SDMMC IDMA stream is busy!";
            return Result::IdmaBusy;   // Timeout!
        }
    } while ((state != IdmaState::Idle) && (state != IdmaState::FinishOk));

    // Start reading:
    state = IdmaState::DataReading;
    remainBlocks = blocksNumber;
    currBlockAddr = blockAddr;
    currData = const_cast<uint8_t*>(dest);

    transferBlock (currData, currBlockAddr, Action::Read);
    //readBlock (currData, currBlockAddr);
    return Result::Ok;
}

void IdmaSlot::reset ()
{
    remainBlocks = 0;
    currBlockAddr = 0;
    currData = nullptr;
    CLEAR_BIT (sdmmc->MASK, irqSendMask | irqReadMask);
    state = IdmaState::Idle;
}

IdmaSlot::IdmaState IdmaSlot::getState()
{
    return state;
}

}   // namespace sdmmc
}   // namespace driver
